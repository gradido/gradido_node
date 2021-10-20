#ifndef __GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR
#define __GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR

#include "Poco/Thread.h"
#include "Poco/Semaphore.h"
#include "Poco/Logger.h"
#include "../lib/MultithreadQueue.h"
#include "IotaWrapper.h"

#include <unordered_map>
#include <map>
#include <assert.h>

namespace iota {
    /*!
     * @author: einhornimmond
     * 
     * @date: 20.10.2021
     * 
     * @brief: get messages and milestones from listener and check if messages where confirmed
     * Get iota milestones and message containing gradido transactions from listener.
     * For every new message check if they already exist in one of the confirmed milestones
     * For every new milestone check if already pending messages are included in them and therefore confirmed
     * Put confirmed messages into blockchain
     */

    class MessageValidator: public Poco::Runnable
    {
    public:
        MessageValidator();
        ~MessageValidator();

        void pushMessageId(const char* messageId, MessageType type);
        void run();

    protected:
        Poco::Thread mThread;
        Poco::Semaphore mSemaphore;
        Poco::Mutex mWorkMutex;

        // put messages as json here which aren't found in milestones
        Poco::Logger& mDroppedMessages;

        struct Milestone 
        {
            uint64_t timestamp;
            std::vector<MessageId> messageIds;
        };
        std::map<uint32_t, Milestone*> mMilestones;
        Poco::Mutex mMilestonesMutex;

        //! message ids from confirmed milestones
        std::unordered_map<MessageId, uint32_t> mConfirmedMessages;
        Poco::Mutex mConfirmedMessagesMutex;

        UniLib::lib::MultithreadQueue<MessageId> mMessageIds[MESSAGE_TYPE_MAX];

    };
}



#endif //__GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR