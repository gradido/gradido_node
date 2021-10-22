#ifndef __GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR
#define __GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR

#include "Poco/Thread.h"
#include "Poco/Condition.h"
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
    //! MAGIC NUMBER: how many milestones in list allowed before deleting the oldest, count of older milestones which are loaded on startup
    //! TODO: try it out to guess best number
    //! TODO: Put it into Config
    #define MILESTONES_MAX_CACHED 10

    class MessageValidator: public Poco::Runnable
    {
    public:
        MessageValidator();
        ~MessageValidator();

        void pushMessageId(const iota::MessageId& messageId, MessageType type);
        void run();

    protected:
        Poco::Thread mThread;
        Poco::Condition mCondition;
        Poco::FastMutex mWorkMutex;
        bool mExitCalled;
        Poco::FastMutex mExitMutex;

        int mCountErrorsFetchingMilestone;

        // put messages as json here which aren't found in milestones
        Poco::Logger& mDroppedMessages;

        
        std::map<uint32_t, Milestone> mMilestones;
        Poco::FastMutex mMilestonesMutex;

        //! message ids from confirmed milestones
        std::unordered_map<MessageId, uint32_t> mConfirmedMessages;
        Poco::FastMutex mConfirmedMessagesMutex;

        std::unordered_map<MessageId, MessageId> mUnconfirmedMessages;
        Poco::FastMutex mUnconfirmedMessagesMutex;

        UniLib::lib::MultithreadQueue<MessageId> mPendingMilestones;

    };
}



#endif //__GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR