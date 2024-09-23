#ifndef __GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR
#define __GRADIDO_NODE_IOTA_MESSAGE_VALIDATOR

#include "Poco/Thread.h"
#include "Poco/Condition.h"
#include "Poco/Logger.h"
#include "gradido_blockchain/data/iota/MessageId.h"
#include "gradido_blockchain/lib/MultithreadQueue.h"
#include "../task/CPUTask.h"

#include "IMessageObserver.h"


#include <unordered_map>
#include <map>
#include <assert.h>

namespace iota {

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

    /*!
		@author einhornimmond

		@date 20.10.2021

		@brief get messages and milestones from listener and check if messages where confirmed

		Get iota milestones and message containing gradido transactions from MessageListener.<br>
		For every new message check if they already exist in one of the confirmed milestones<br>
		For every new milestone check if already pending messages are included in them and therefore confirmed<br>
		Put confirmed messages into blockchain via OrderingManager<br>
		TODO: Refactor to that it works even if many transaction are incoming at once and the buffer time from OrderingManager isn't enough<br>
		Wait with calling popMilestone until the whole list of unconfirmed transaction was checked<br>
		Use IotaMessageToTransactionTask for processing all IotaMessages for loaded Milestones<br>
		Use MilestoneLoadingTask for loading Milestones from Iota.<br>

		\startuml
		(*top) --> ===LOOP_START===
		--> "Wait on Pending Messages"
		note top
            get new messageIDs
            from MessageListener
        end note
        --> if "new messageIds" then 
			->[arrived] "Call Iota API:\nask for milestoneId for message"
			if "milestoneId" then
                -->[not null ]===VALID_MILESTONE===
				--> "Observe Milestone"
				note bottom
                    OrderingManager keep track
                    of all milestones 
                    for which messages are processing
                end note

				===VALID_MILESTONE=== if "messages for this milestone" then
				->[where already queued] "put message to:\nConfirmedMessage map"
				--> ===LOOP_START===
				else
                    if "milestone is" then
				    ->[finished with loading] "start IotaMessageToTransactionTask task for milestone"
				    --> ===LOOP_START===
                    else
				        --->[else] "queue first message for this milestone"
				        --> "start MilestoneLoadingTask"
                        --> ===LOOP_START===
                    endif
                endif
            else 
                ->[null]===LOOP_START===
			endif
        else
            ->[not arrived]===LOOP_START===
        endif
	    \enduml
    */

    

    class MessageValidator : public Poco::Runnable, public IMessageObserver
    {
    public:
        MessageValidator();
        ~MessageValidator();

        inline void firstRunStart() { mMessageListenerFirstRunCount++; }
        inline void firstRunFinished() { mMessageListenerFirstRunCount--; mWaitOnEmptyQueue++; }
        int getFirstRunCount() { return mMessageListenerFirstRunCount.value() + mWaitOnEmptyQueue.value(); }

        inline void pushMessageId(const iota::MessageId& messageId) { mPendingMessages.push(messageId); }
        void run();
        void pushMilestone(int32_t id, int64_t timestamp);
        void messageConfirmed(const iota::MessageId& messageId, int32_t milestoneId);

        inline void signal() { mCondition.signal(); }

        // implemented from IMessageObserver, called via mqtt from iota server
        virtual void messageArrived(MQTTAsync_message* message, TopicType type);

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

        MultithreadQueue<MessageId> mPendingMessages;
        Poco::AtomicCounter mMessageListenerFirstRunCount;
        Poco::AtomicCounter mWaitOnEmptyQueue;

    };

    // **********************  Milestone Loading Task ****************************
    class MilestoneLoadingTask : public task::CPUTask
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