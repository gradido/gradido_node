#ifndef __GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR
#define __GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR

#include "Poco/Thread.h"
#include "Poco/Condition.h"
#include "Poco/Logger.h"
#include "../lib/MultithreadQueue.h"
#include "../task/CPUTask.h"
#include "MessageId.h"


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
    #define MILESTONES_MAX_CACHED 50
    // MAGIC NUMBER: how many messages in queue should be checked for milestone before break loop
    // we assume that the oldest message are most likely to get confirmed as first
    // so if the oldest not confirmed we don't need to check the newer
    // but just in case we check more than one
#define MAGIC_NUMBER_TRY_MESSAGE_GET_MILESTONE 3

#define MAGIC_NUMBER_WAIT_ON_IOTA_CONFIRMATION_TIMEOUT_MILLI_SECONDS 500 

    class MessageValidator : public Poco::Runnable
    {
    public:
        MessageValidator();
        ~MessageValidator();

        inline void pushMessageId(const iota::MessageId& messageId) { mPendingMessages.push(messageId); }
        void run();
        void pushMilestone(int32_t id, int64_t timestamp);

        inline void signal() { mCondition.signal(); }

    protected:


        Poco::Thread mThread;
        Poco::Condition mCondition;
        Poco::FastMutex mWorkMutex;
        bool mExitCalled;
        Poco::FastMutex mExitMutex;

        int mCountErrorsFetchingMilestone;

        std::map<uint32_t, uint64_t> mMilestones;
        Poco::FastMutex mMilestonesMutex;

        std::unordered_map<uint32_t, std::list<MessageId>> mConfirmedMessages;
        Poco::FastMutex mConfirmedMessagesMutex;

        UniLib::lib::MultithreadQueue<MessageId> mPendingMessages;
        

    };

    // **********************  Milestone Loading Task ****************************
    class MilestoneLoadingTask : public UniLib::controller::CPUTask
    {
    public:
        MilestoneLoadingTask(int32_t milestoneId, MessageValidator* messageValidator);
		const char* getResourceType() const { return "iota::MilestoneLoadingTask"; }
		int run();

    protected:
        int32_t mMilestoneId;
        MessageValidator* mMessageValidator;
    };
}



#endif //__GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR