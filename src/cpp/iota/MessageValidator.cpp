#include "MessageValidator.h"
#include "../task/IotaMessageToTransactionTask.h"
#include "../SingletonManager/MemoryManager.h"
#include "sodium.h"
#include "HTTPApi.h"
#include <assert.h>
#include <stdexcept>

namespace iota {
    MessageValidator::MessageValidator()
    : mDroppedMessages(Poco::Logger::get("droppedMessages")), mExitCalled(false),
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
        mMilestonesMutex.lock();
        std::clog << "Milestones count: " << std::to_string(mMilestones.size()) << std::endl;
        mMilestones.clear();
        mMilestonesMutex.unlock();

        mConfirmedMessagesMutex.lock();
        std::clog << "Confirmed Messages Count: " << std::to_string(mConfirmedMessages.size()) << std::endl;
        mConfirmedMessages.clear();
        mConfirmedMessagesMutex.unlock();

        mUnconfirmedMessagesMutex.lock();
        std::clog << "Unconfirmed Messages Count: " << std::to_string(mUnconfirmedMessages.size()) << std::endl;
        mUnconfirmedMessages.clear();
        mUnconfirmedMessagesMutex.unlock();

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

            iota::MessageId milestoneMessageId;
            int countProcessedMilestones = 0;
            while(mPendingMilestones.pop(milestoneMessageId))
            {
                // fetch one milestone from iota
                auto milestone = iotaHttp::getMilestone(milestoneMessageId);
                std::clog << "new milestone " << std::to_string(milestone.id) << ", "
                    << std::to_string(milestone.timestamp) << std::endl;
                // put into `mMilestones` map
                mMilestonesMutex.lock();
                mMilestones.insert({milestone.id, milestone});
                mMilestonesMutex.unlock();

                // check every message id if they can be found in `mUnconfirmedMessages`
                for(auto it = milestone.messageIds.begin(); it != milestone.messageIds.end(); it++)
                {
                    bool found = false;
                    { // start scoped lock
                        Poco::ScopedLock<Poco::FastMutex> _lock1(mUnconfirmedMessagesMutex);
                        auto itFound = mUnconfirmedMessages.find(*it);
                        if(itFound != mUnconfirmedMessages.end())  {
                            found = true;
                            // if found, create `IotaMessageToTransactionTask` from it
                            Poco::AutoPtr<IotaMessageToTransactionTask> task(new IotaMessageToTransactionTask(
                                milestone.id,
                                milestone.timestamp,
                                itFound->second
                            ));
                            task->scheduleTask(task);
                            mUnconfirmedMessages.erase(itFound);
                        }
                    } // end scoped lock
                    if(!found) {
                        // else put message id into `mConfirmedMessages` unordered map
                        mConfirmedMessagesMutex.lock();
                        mConfirmedMessages.insert({*it, milestone.id});
                        mConfirmedMessagesMutex.unlock();
                    }

                }
                countProcessedMilestones++;
            }
            std::clog << "[MessageValidator] processed milestones: " << std::to_string(countProcessedMilestones) << std::endl;
            // remove older milestones and there messageIDs from confirmed messages from list but how old should they be?
            { // start scoped lock
                Poco::ScopedLock<Poco::FastMutex> _lock2(mMilestonesMutex);
                Poco::ScopedLock<Poco::FastMutex> _lock3(mConfirmedMessagesMutex);
                while(mMilestones.size() > MILESTONES_MAX_CACHED) {
                    // map index is milestone id so it should be smallest = oldest
                    auto milestoneIt = mMilestones.begin();
                    for(auto it = milestoneIt->second.messageIds.begin(); it != milestoneIt->second.messageIds.end(); it++) {
                       mConfirmedMessages.erase(*it);
                    }
                    mMilestones.erase(milestoneIt);
                }
            } // end scoped lock

            mWorkMutex.unlock();
        }
    }

    void MessageValidator::pushMessageId(const iota::MessageId& messageId, MessageType type)
    {
        assert(type < MESSAGE_TYPE_MAX);

        if(MESSAGE_TYPE_MILESTONE == type) {
            mPendingMilestones.push(messageId);
            // check if Message Validator is already working, if so we don't need to signaling a new dataset
            if(mWorkMutex.tryLock()) {
                mWorkMutex.unlock();
                mCondition.signal();
            }

        } else if(MESSAGE_TYPE_TRANSACTION == type) {
            bool found = false;
            { // start scoped lock
                Poco::ScopedLock<Poco::FastMutex> _lock1(mConfirmedMessagesMutex);
                auto it = mConfirmedMessages.find(messageId);
                if(it != mConfirmedMessages.end()) {
                    found = true;
                    mMilestonesMutex.lock();
                    auto milestoneIt = mMilestones.find(it->second);
                    if(milestoneIt == mMilestones.end()) {
                        mMilestonesMutex.unlock();
                        throw new std::runtime_error("milestone not found, already deleted?");
                    }

                    Poco::AutoPtr<IotaMessageToTransactionTask> task(new IotaMessageToTransactionTask(
                        milestoneIt->first,
                        milestoneIt->second.timestamp,
                        messageId
                    ));
                    mMilestonesMutex.unlock();
                    task->scheduleTask(task);
                }
            } // end scoped lock
            if(!found) {
                mUnconfirmedMessagesMutex.lock();
                mUnconfirmedMessages.insert({messageId, messageId});
                mUnconfirmedMessagesMutex.unlock();
            }
        }
    }


}
