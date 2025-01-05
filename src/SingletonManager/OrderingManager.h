#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
#define __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER

#include <chrono>

//! MAGIC NUMBER: how long we know about a milestone from iota at least before we process it and his transaction
//! to prevent loosing transaction which came after through a delay or to less delay in the race between threads
//! important if iota or the node server is running to slow because to many requests
//! TODO: Make value higher if a delay was detected
#define MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER std::chrono::milliseconds(900)

//! MAGIC NUMBER: if current milestone is this diff or more away from last milestone from network,
//! we take extra care and wait MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER time before we proceed
//! maybe we are starting up this node and then we need more time
#define MAGIC_NUMBER_MILESTONE_ID_EXTRA_BUFFER_WAIT 10

#include "gradido_blockchain/data/GradidoTransaction.h"
#include "../iota/MessageValidator.h"
#include "../iota/MessageListener.h"
#include "../task/Thread.h"

#include <map>
#include <mutex>



/*!
 * @author einhornimmond
 *
 * @date 20.10.2021
 *
 * @brief Get transactions unordered form iota but with information to put them into order into blockchain
 *
 * Additional check for cross-group transactions
 */

class FinishMilestoneTask;
class OrderingManager : public task::Thread
{
    friend class FinishMilestoneTask;
public:
    ~OrderingManager();
    static OrderingManager* getInstance();

    void pushMilestoneTaskObserver(int32_t milestoneId);
    void popMilestoneTaskObserver(int32_t milestoneId);

    int pushTransaction(
        std::shared_ptr<const gradido::data::GradidoTransaction> transaction,
        int32_t milestoneId, 
        Timepoint timestamp,
        std::string_view communityId,
        std::shared_ptr<const memory::Block> messageId
    );

    inline iota::MessageValidator* getIotaMessageValidator() { return &mMessageValidator; }

protected:
    OrderingManager();
   
	// called from thread
    int ThreadFunction();

    struct GradidoTransactionWithGroup
    {
        GradidoTransactionWithGroup(
            std::shared_ptr<const gradido::data::GradidoTransaction> _transaction,
            std::string_view _communityId,
            std::shared_ptr<const memory::Block> _messageId
        ) : transaction(_transaction), communityId(_communityId), messageId(_messageId) {}

        GradidoTransactionWithGroup(GradidoTransactionWithGroup&& move) noexcept
            : transaction(std::move(move.transaction)),
            communityId(std::move(move.communityId)),
            messageId(std::move(move.messageId))
        {
        }

        ~GradidoTransactionWithGroup() {}        

        std::shared_ptr<const gradido::data::GradidoTransaction> transaction;
        std::string communityId;
        std::shared_ptr<const memory::Block> messageId;
    };

    struct MilestoneTransactions
    {
        MilestoneTransactions(int32_t _milestoneId, Timepoint _milestoneTimestamp)
            : milestoneId(_milestoneId), milestoneTimestamp(_milestoneTimestamp) {}

        int32_t milestoneId;
        Timepoint milestoneTimestamp;
        Timepoint entryCreationTime;
        std::list<GradidoTransactionWithGroup> transactions;
        std::mutex mutex;
    };

    iota::MessageValidator mMessageValidator;

    std::map<int32_t, int32_t> mMilestoneTaskObserver;
    std::mutex mMilestoneTaskObserverMutex;

    std::map<int32_t, MilestoneTransactions*> mMilestonesWithTransactions;
    std::mutex mMilestonesWithTransactionsMutex;
    std::mutex mFinishMilestoneTaskMutex;
};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
