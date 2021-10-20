#include "MessageValidator.h"
#include "../task/IotaMessageToTransactionTask.h"
#include "../SingletonManager/MemoryManager.h"
#include "sodium.h"
#include <assert.h>
#include <stdexcept>

namespace iota {
    MessageValidator::MessageValidator()
    : mSemaphore(0), mDroppedMessages(Poco::Logger::get("droppedMessages"))
    {

    }

    MessageValidator::~MessageValidator()
    {
        mMilestonesMutex.lock();
        for(auto it = mMilestones.begin(); it != mMilestones.end(); it++) {
            delete it->second;
        }
        mMilestones.clear();
        mMilestonesMutex.unlock();
    }

    void MessageValidator::run()
    {
        // thread loop, waiting on semaphore

            // will be triggered if a new milestone was added
            // fetch new milestone(s) from iota
            // put into `mMilestones` map
            // check every message id if they can be found in `mMessageIds[MESSAGE_ID_TRANSACTION]`
                // if found, create `IotaMessageToTransactionTask` from it
                // else put message id into `mConfirmedMessages` unordered map

            // remove older milestones from list but how old should they be?

    }

    void MessageValidator::pushMessageId(const char* messageId, MessageIdType type) 
    {
        assert(type < MESSAGE_ID_MAX);
        IotaMessageId messageIdObject;
        messageIdObject.fromByteArray(messageId);
        if(MESSAGE_ID_MILESTONE == type) {
            mMessageIds[type].push(messageIdObject);
            mSemaphore.set();
        } else if(MESSAGE_ID_TRANSACTION == type) {
            mConfirmedMessagesMutex.lock();
            auto it = mConfirmedMessages.find(messageIdObject);
            if(it != mConfirmedMessages.end()) {
                auto milestoneIt = mMilestones.find(it->second);
                if(milestoneIt == mMilestones.end()) {
                    throw new std::runtime_error("milestone not found, already deleted?");
                }

                Poco::AutoPtr<IotaMessageToTransactionTask> task(new IotaMessageToTransactionTask(
                    milestoneIt->first,
                    milestoneIt->second->timestamp,
                    messageIdObject.toMemoryBin()
                ));
                task->scheduleTask(task);

            } else {
                mMessageIds[type].push(messageIdObject);
            }
        }        
    }

    
}