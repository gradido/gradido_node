#include "MessageValidator.h"
#include "MessageParser.h"
#include "MqttClientWrapper.h"
#include "../task/IotaMessageToTransactionTask.h"
#include "gradido_blockchain/lib/RapidjsonHelper.h"
#include "../SingletonManager/GlobalStateManager.h"
#include "../SingletonManager/OrderingManager.h"
#include "../ServerGlobals.h"

#include "sodium.h"
#include "loguru/loguru.hpp"

#include <assert.h>
#include <stdexcept>

using namespace rapidjson;

namespace iota {
    MessageValidator::MessageValidator()
    : mThread(nullptr),
      mExitCalled(false),
      mCountErrorsFetchingMilestone(0),
      mMessageListenerFirstRunCount(0),
        mWaitOnEmptyQueue(0)
    {		
    }

    MessageValidator::~MessageValidator()
    {
        MqttClientWrapper::getInstance()->unsubscribe(TopicType::MILESTONES_CONFIRMED, this);
		mExitMutex.lock();
		mExitCalled = true;
		mExitMutex.unlock();
		if (mWorkMutex.try_lock()) {
			mWorkMutex.unlock();
			mCondition.notify_one();
		}
		try {
            mThread->join();
		}
		catch (GradidoBlockchainException& ex) {
            LOG_F(ERROR, "gradido blockchain exception by shutdown MessageValidator thread: %s", ex.getFullString().data());
		}
        catch (std::exception& e) {
            LOG_F(ERROR, "exception by shutdown MessageValidator thread: %s", e.what());
        }
        LOG_F(1, "Message Validator shutdown");
        
		mConfirmedMessagesMutex.lock();
		mConfirmedMessages.clear();
		mConfirmedMessagesMutex.unlock();

        mMilestonesMutex.lock();
        LOG_F(1, "Milestones count: %llu", mMilestones.size());
        mMilestones.clear();
        mMilestonesMutex.unlock();
    }

    void MessageValidator::init()
    {
        mThread = new std::thread(&MessageValidator::run, this);
        MqttClientWrapper::getInstance()->subscribe(TopicType::MILESTONES_CONFIRMED, this);
    }

    void MessageValidator::run()
    {
        loguru::set_thread_name("MessageValidator");
        // thread loop, waiting on condition
        while(true) {
            // if at least one unconfirmed message exist, check every MAGIC_NUMBER_WAIT_ON_IOTA_CONFIRMATION_TIMEOUT_MILLI_SECONDS 
            // if it has get a milestone (was confirmed) else wait on the next unconfirmed message which will notify the condition
            std::unique_lock _lock(mWorkMutex);
            if (!mPendingMessages.empty()) {
                mCondition.wait_for(_lock, MAGIC_NUMBER_WAIT_ON_IOTA_CONFIRMATION_TIMEOUT);
            }
            else {
                mCondition.wait(_lock);
            }
            mExitMutex.lock();

            if(mExitCalled) {
                mExitMutex.unlock();
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
                }
                else {
                    ++notConfirmedCount;
                    mPendingMessages.push(messageId);
                }

                if (notConfirmedCount > mPendingMessages.size() || notConfirmedCount > MAGIC_NUMBER_TRY_MESSAGE_GET_MILESTONE) break;
            }
            mWaitOnEmptyQueue = 0;
        }
    }

    void MessageValidator::pushMilestone(int32_t id, Timepoint timestamp)
    {
        LOG_F(1, "process milestone: %d from timestamp: %s", id, DataTypeConverter::timePointToString(timestamp).data());
        std::lock_guard lock(mConfirmedMessagesMutex);
        
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
        LOG_F(1, "Message confirmed: %s in milestone: %d", messageId.toHex().data(), milestoneId);
		// pop will be called in IotaMessageToTransactionTask
		OrderingManager::getInstance()->pushMilestoneTaskObserver(milestoneId);
		
		// check if other messages for this milestone exist and the milestone loading was started
		{ // scoped lock mConfirmedMessages
            std::lock_guard lock(mConfirmedMessagesMutex);
			auto it = mConfirmedMessages.find(milestoneId);
			if (it != mConfirmedMessages.end()) {
				// found others messages which are confirmed from the same milestone
				// push to list which will be processed after the milestone loading task was finished
				it->second.push_back(messageId);
                LOG_F(1, "other messages with same milestone found, push to list");
                return;
			}
		} // scoped lock mConfirmedMessages end

		// check if milestone for this was already loaded
		{ // scoped lock milestones 
            std::lock_guard lock(mMilestonesMutex);
			auto it = mMilestones.find(milestoneId);
			if (it != mMilestones.end()) {
				std::shared_ptr<IotaMessageToTransactionTask> task(new IotaMessageToTransactionTask(milestoneId, it->second, messageId));
				task->scheduleTask(task);
                LOG_F(1, "found milestone in confirmed milestones list, schedule iota message to transaction task");
				return;
			}
		} // scoped lock milestones end

		// milestone wasn't loaded and other messages for this milestone doesn't exist, so create entry for this milestone 
        {
            std::lock_guard lock(mConfirmedMessagesMutex);
            mConfirmedMessages.insert({ milestoneId, std::list<MessageId>(1, messageId) });
            LOG_F(1, "add entry in confirmed messages map with this milestone");
        }
        // check last milestone we get via mqtt, it is not set or higher, than we need a milestone loading task
        auto lastMilestone = GlobalStateManager::getInstance()->getLastIotaMilestone();
        if (!lastMilestone || lastMilestone > milestoneId) {
            // and start loading task for it
            LOG_F(1, "Milestone: %d was already confirmed, start milestone loading task", milestoneId);
            std::shared_ptr<MilestoneLoadingTask> task(new MilestoneLoadingTask(milestoneId, this));
            task->scheduleTask(task);
        }
    }

    void MessageValidator::messageArrived(MQTTAsync_message* message, TopicType type)
    {
        //MessageParser parsedMessage(message->payload, message->payloadlen);
        //std::string messageString((const char*)message->payload, message->payloadlen);
        LOG_F(1, "message: %s", message->payload);
		Document document;
		document.Parse((const char*)message->payload);
        rapidjson_helper::checkMember(document, "index", rapidjson_helper::MemberType::INTEGER, "iota milestone/confirmed");
        rapidjson_helper::checkMember(document, "timestamp", rapidjson_helper::MemberType::INTEGER, "iota milestone/confirmed");
        auto milestoneTimestampSeconds = document["timestamp"].GetUint64();
        auto milestoneTimepoint = std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(milestoneTimestampSeconds));
        auto milestoneIndex = document["index"].GetInt();
        GlobalStateManager::getInstance()->updateLastIotaMilestone(milestoneIndex);
        pushMilestone(milestoneIndex, milestoneTimepoint);
    }


    // ************************ Milestone Loading Task **********************************
    MilestoneLoadingTask::MilestoneLoadingTask(int32_t milestoneId, MessageValidator* messageValidator)
        : CPUTask(ServerGlobals::g_IotaRequestCPUScheduler), mMilestoneId(milestoneId), mMessageValidator(messageValidator)
    {

    }

    int MilestoneLoadingTask::run()
    {
        LOG_F(1, "start loading milestone");
        auto milestoneTimestamp = ServerGlobals::g_IotaRequestHandler->getMilestoneTimestamp(mMilestoneId);
        if (milestoneTimestamp) {
            auto milestoneTimepoint = std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(milestoneTimestamp));
            mMessageValidator->pushMilestone(mMilestoneId, milestoneTimepoint);
        }
        else {
            LOG_F(ERROR, "couldn't load milestone: %d from iota", mMilestoneId);
        }
        return 0;
    }

}
