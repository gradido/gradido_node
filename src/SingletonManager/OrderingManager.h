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
#include "Poco/ExpireCache.h"
#include "Poco/AccessExpireCache.h"

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

    int pushTransaction(std::unique_ptr<gradido::data::GradidoTransaction> transaction, int32_t milestoneId, uint64_t timestamp, const std::string& groupAlias, MemoryBin* messageId);

    inline iota::MessageValidator* getIotaMessageValidator() { return &mMessageValidator; }

protected:
    OrderingManager();
   
	// called from thread
    int ThreadFunction();

    struct GradidoTransactionWithGroup
    {
        GradidoTransactionWithGroup(
            std::unique_ptr<gradido::data::GradidoTransaction> _transaction,
            const std::string& _groupAlias,
            const memory::Block& _messageId
        ) : transaction(std::move(_transaction)), groupAlias(_groupAlias), messageId(_messageId) {}

        GradidoTransactionWithGroup(GradidoTransactionWithGroup&& move) noexcept
            : transaction(std::move(move.transaction)),
            groupAlias(std::move(move.groupAlias)),
            messageId(std::move(move.messageId))
        {
        }

        ~GradidoTransactionWithGroup() {}        

        std::unique_ptr<gradido::data::GradidoTransaction> transaction;
        std::string groupAlias;
        memory::Block messageId;
    };

    struct MilestoneTransactions
    {
        MilestoneTransactions(int32_t _milestoneId, int64_t _milestoneTimestamp)
            : milestoneId(_milestoneId), milestoneTimestamp(_milestoneTimestamp) {}

        int32_t milestoneId;
        int64_t milestoneTimestamp;
        Timepoint entryCreationTime;
        std::list<GradidoTransactionWithGroup> transactions;
        Poco::FastMutex mutex;
    };

    iota::MessageValidator mMessageValidator;

    std::map<int32_t, int32_t> mMilestoneTaskObserver;
    Poco::FastMutex mMilestoneTaskObserverMutex;

    std::map<int32_t, MilestoneTransactions*> mMilestonesWithTransactions;
    Poco::FastMutex mMilestonesWithTransactionsMutex;
    Poco::FastMutex mFinishMilestoneTaskMutex;
};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
