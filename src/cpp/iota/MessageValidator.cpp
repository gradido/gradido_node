#include "MessageValidator.h"
#include "../task/IotaMessageToTransactionTask.h"
#include "../SingletonManager/MemoryManager.h"
#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/OrderingManager.h"
#include "../ServerGlobals.h"
#include "sodium.h"
#include "HTTPApi.h"
#include <assert.h>
#include <stdexcept>

namespace iota {
    MessageValidator::MessageValidator()
    : mExitCalled(false),
      mCountErrorsFetchingMilestone(0)
    {
        mThread.setName("messVal");
        mThread.start(*this);
    }

    MessageValidator::~MessageValidator()
    {
        mExitMutex.lock();
        mExitCalled = true;
        mExitMutex.unlock();
        if(mWorkMutex.tryLock()) {
            mWorkMutex.unlock();
            mCondition.signal();
        }
        try {
            mThread.join(100);
        } catch(Poco::Exception& e) {
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
            // will be triggered if a new milestone was added or exit was called
            mWorkMutex.lock();
            mCondition.wait(mWorkMutex);
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
                auto milestoneId = getMessageMilestoneId(messageId);
                // if messages was already confirmed from iota
                if (milestoneId) {
                    // pop will be called in IotaMessageToTransactionTask
                    OrderingManager::getInstance()->pushMilestoneTaskObserver(milestoneId);
                    notConfirmedCount = 0;
                    // check if other messages for this milestone exist and the milestone loading was started
                    { // scoped lock mConfirmedMessages
                        Poco::ScopedLock<Poco::FastMutex> lock(mConfirmedMessagesMutex);
                        auto it = mConfirmedMessages.find(milestoneId);
                        if (it != mConfirmedMessages.end()) {
                            // found others messages which are confirmed from the same milestone
                            // push to list which will be processed after the milestone loading task was finished
                            it->second.push_back(messageId);
                            continue;
                        }
                    } // scoped lock mConfirmedMessages end

                    // check if milestone for this was already loaded
                    { // scoped lock milestones 
                        Poco::ScopedLock<Poco::FastMutex> lock(mMilestonesMutex);
                        auto it = mMilestones.find(milestoneId);
                        if (it != mMilestones.end()) {
                            Poco::AutoPtr<IotaMessageToTransactionTask> task(new IotaMessageToTransactionTask(milestoneId, it->second, messageId));
                            task->scheduleTask(task);
                            continue;
                        }
                    } // scoped lock milestones end

                    // milestone wasn't loaded and other messages for this milestone doesn't exist, so create entry for this milestone and start loading task for it
                    mConfirmedMessagesMutex.lock();
                    mConfirmedMessages.insert({ milestoneId, std::list<MessageId>(1, messageId) });
                    mConfirmedMessagesMutex.unlock();
                    Poco::AutoPtr<MilestoneLoadingTask> task(new MilestoneLoadingTask(milestoneId, this));
                    task->scheduleTask(task);
                }
                else {
                    ++notConfirmedCount;
                }

                if (notConfirmedCount > MAGIC_NUMBER_TRY_MESSAGE_GET_MILESTONE) break;
            }
            mWorkMutex.unlock();
        }
    }

    void MessageValidator::pushMessageId(const MessageId& messageId)
    {
        mPendingMessages.push(messageId);
        mCondition.signal();
    }

    void MessageValidator::pushMilestone(int32_t id, int64_t timestamp)
    {
        Poco::ScopedLock<Poco::FastMutex> lock(mConfirmedMessagesMutex);
        
        auto it = mConfirmedMessages.find(id);
        if (it != mConfirmedMessages.end()) {
            for (auto itList = it->second.begin(); itList != it->second.end(); itList++) {
				Poco::AutoPtr<IotaMessageToTransactionTask> task(new IotaMessageToTransactionTask(id, timestamp, *itList));
				task->scheduleTask(task);
            }
            mConfirmedMessages.erase(it);
        }
        else {
            LoggerManager::getInstance()->mErrorLogging.error("[%s] loaded milestone %d but no messages for it where found", __FUNCTION__, id);
        }

        mMilestonesMutex.lock();
        while (mMilestones.size() > MILESTONES_MAX_CACHED) {
            mMilestones.erase(mMilestones.begin());
        }
        mMilestones.insert({ id, timestamp });
        mMilestonesMutex.unlock();        
    }


    // ************************ Milestone Loading Task **********************************
    MilestoneLoadingTask::MilestoneLoadingTask(int32_t milestoneId, MessageValidator* messageValidator)
        : CPUTask(ServerGlobals::g_IotaRequestCPUScheduler), mMilestoneId(milestoneId), mMessageValidator(messageValidator)
    {

    }

    int MilestoneLoadingTask::run()
    {
        auto milestoneTimestamp = getMilestoneTimestamp(mMilestoneId);
        if (milestoneTimestamp) {
            mMessageValidator->pushMilestone(mMilestoneId, milestoneTimestamp);
        }
        else {
            LoggerManager::getInstance()->mErrorLogging.error("%s couldn't load milestone: %d from iota", __FUNCTION__, mMilestoneId);
        }
        return 0;
    }

}
