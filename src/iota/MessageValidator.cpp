#include "MessageValidator.h"
#include "MessageParser.h"
#include "MqttClientWrapper.h"
#include "../task/IotaMessageToTransactionTask.h"
#include "gradido_blockchain/MemoryManager.h"
#include "gradido_blockchain/lib/RapidjsonHelper.h"
#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/OrderingManager.h"
#include "../ServerGlobals.h"

#include "sodium.h"
#include <assert.h>
#include <stdexcept>

using namespace rapidjson;

namespace iota {
    MessageValidator::MessageValidator()
    : mExitCalled(false),
      mCountErrorsFetchingMilestone(0),
      mMessageListenerFirstRunCount(0),
        mWaitOnEmptyQueue(0)
    {
		mThread.setName("messVal");
		mThread.start(*this);
        MqttClientWrapper::getInstance()->subscribe(TopicType::MILESTONES_CONFIRMED, this);
    }

    MessageValidator::~MessageValidator()
    {
        MqttClientWrapper::getInstance()->unsubscribe(TopicType::MILESTONES_CONFIRMED, this);
		mExitMutex.lock();
		mExitCalled = true;
		mExitMutex.unlock();
		if (mWorkMutex.tryLock()) {
			mWorkMutex.unlock();
			mCondition.signal();
		}
		try {
			mThread.join(100);
		}
		catch (Poco::Exception& e) {
			std::clog << "error by shutdown MessageValidator thread: " << e.displayText() << std::endl;
		}
		std::clog << "Message Validator shutdown: " << std::endl;
        
		mConfirmedMessagesMutex.lock();
		mConfirmedMessages.clear();
		mConfirmedMessagesMutex.unlock();

        mMilestonesMutex.lock();
        std::clog << "Milestones count: " << std::to_string(mMilestones.size()) << std::endl;
        mMilestones.clear();
        mMilestonesMutex.unlock();
    }

    void MessageValidator::run()
    {
        // thread loop, waiting on condition
        while(true) {
            // if at least one unconfirmed message exist, check every MAGIC_NUMBER_WAIT_ON_IOTA_CONFIRMATION_TIMEOUT_MILLI_SECONDS 
            // if it has get a milestone (was confirmed) else wait on the next unconfirmed message which will notify the condition
            mWorkMutex.lock();
            if (!mPendingMessages.empty()) {
                mCondition.tryWait(mWorkMutex, MAGIC_NUMBER_WAIT_ON_IOTA_CONFIRMATION_TIMEOUT_MILLI_SECONDS);
            }
            else {
                mCondition.wait(mWorkMutex);
            }
            mExitMutex.lock();

            if(mExitCalled) {
                mExitMutex.unlock();
                mWorkMutex.unlock();
                return;
            }
            mExitMutex.unlock();            

            MessageId messageId;
            int notConfirmedCount = 0;
            while (mPendingMessages.pop(messageId))
            {
                auto milestoneId = ServerGlobals::g_IotaRequestHandler->getMessageMilestoneId(messageId.toHex());
                // if messages was already confirmed from iota
                if (milestoneId) {
                    notConfirmedCount = 0;
                    messageConfirmed(messageId, milestoneId);
					// and start loading task for it
					std::shared_ptr<MilestoneLoadingTask> task(new MilestoneLoadingTask(milestoneId, this));
					task->scheduleTask(task);
                }
                else {
                    ++notConfirmedCount;
                    mPendingMessages.push(messageId);
                }

                if (notConfirmedCount > mPendingMessages.size() || notConfirmedCount > MAGIC_NUMBER_TRY_MESSAGE_GET_MILESTONE) break;
            }
            mWaitOnEmptyQueue = 0;
            mWorkMutex.unlock();
        }
    }

    void MessageValidator::pushMilestone(int32_t id, int64_t timestamp)
    {
        Poco::ScopedLock<Poco::FastMutex> lock(mConfirmedMessagesMutex);
        
        auto it = mConfirmedMessages.find(id);
        if (it != mConfirmedMessages.end()) {
            for (auto itList = it->second.begin(); itList != it->second.end(); itList++) {
				std::shared_ptr<IotaMessageToTransactionTask> task(new IotaMessageToTransactionTask(id, timestamp, *itList));
				task->scheduleTask(task);
            }
            mConfirmedMessages.erase(it);
        }
#ifdef IOTA_WITHOUT_MQTT
        else {
            LoggerManager::getInstance()->mErrorLogging.error("[%s] loaded milestone %d but no messages for it where found", std::string(__FUNCTION__), id);
        }
#endif
        mMilestonesMutex.lock();
        while (mMilestones.size() > MILESTONES_MAX_CACHED) {
            mMilestones.erase(mMilestones.begin());
        }
        mMilestones.insert({ id, timestamp });
        mMilestonesMutex.unlock();        
    }

    void MessageValidator::messageConfirmed(const iota::MessageId& messageId, int32_t milestoneId)
    {
		// pop will be called in IotaMessageToTransactionTask
		OrderingManager::getInstance()->pushMilestoneTaskObserver(milestoneId);
		
		// check if other messages for this milestone exist and the milestone loading was started
		{ // scoped lock mConfirmedMessages
			Poco::ScopedLock<Poco::FastMutex> lock(mConfirmedMessagesMutex);
			auto it = mConfirmedMessages.find(milestoneId);
			if (it != mConfirmedMessages.end()) {
				// found others messages which are confirmed from the same milestone
				// push to list which will be processed after the milestone loading task was finished
				it->second.push_back(messageId);
                return;
			}
		} // scoped lock mConfirmedMessages end

		// check if milestone for this was already loaded
		{ // scoped lock milestones 
			Poco::ScopedLock<Poco::FastMutex> lock(mMilestonesMutex);
			auto it = mMilestones.find(milestoneId);
			if (it != mMilestones.end()) {
				std::shared_ptr<IotaMessageToTransactionTask> task(new IotaMessageToTransactionTask(milestoneId, it->second, messageId));
				task->scheduleTask(task);
				return;
			}
		} // scoped lock milestones end

		// milestone wasn't loaded and other messages for this milestone doesn't exist, so create entry for this milestone 
		mConfirmedMessagesMutex.lock();
		mConfirmedMessages.insert({ milestoneId, std::list<MessageId>(1, messageId) });
		mConfirmedMessagesMutex.unlock();
    }

    void MessageValidator::messageArrived(MQTTAsync_message* message, TopicType type)
    {
        //MessageParser parsedMessage(message->payload, message->payloadlen);
        //std::string messageString((const char*)message->payload, message->payloadlen);
		Document document;
		document.Parse((const char*)message->payload);
        rapidjson_helper::checkMember(document, "index", rapidjson_helper::MemberType::INTEGER, "iota milestone/confirmed");
        rapidjson_helper::checkMember(document, "timestamp", rapidjson_helper::MemberType::INTEGER, "iota milestone/confirmed");
        pushMilestone(document["index"].GetInt(), document["timestamp"].GetInt());
    }


    // ************************ Milestone Loading Task **********************************
    MilestoneLoadingTask::MilestoneLoadingTask(int32_t milestoneId, MessageValidator* messageValidator)
        : CPUTask(ServerGlobals::g_IotaRequestCPUScheduler), mMilestoneId(milestoneId), mMessageValidator(messageValidator)
    {

    }

    int MilestoneLoadingTask::run()
    {
        auto milestoneTimestamp = ServerGlobals::g_IotaRequestHandler->getMilestoneTimestamp(mMilestoneId);

        if (milestoneTimestamp) {
            mMessageValidator->pushMilestone(mMilestoneId, milestoneTimestamp);
        }
        else {
            LoggerManager::getInstance()->mErrorLogging.error("%s couldn't load milestone: %d from iota", __FUNCTION__, mMilestoneId);
        }
        return 0;
    }

}
