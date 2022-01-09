#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
#define __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER

/*!
 * @author: einhornimmond
 *
 * @date: 20.10.2021
 *
 * @brief: Get transactions unordered form iota but with information to put them into order into blockchain
 * Additional check for cross-group transactions
 */

#include "../model/transactions/GradidoTransaction.h"
#include "../iota/MessageValidator.h"
#include "../iota/MessageListener.h"
#include "../task/Thread.h"
#include <map>
#include "Poco/ExpireCache.h"
#include "Poco/AccessExpireCache.h"

//! MAGIC NUMBER: how many minutes every mPairedTransactions entry should be stored in cache
//! depends how long the node server needs to process cross group transactions
//! if number is to small, transaction cannot be validated and can be get lost
//! TODO: Find a better approach to prevent loosing transactions through timeout
#define MAGIC_NUMBER_CROSS_GROUP_TRANSACTION_CACHE_TIMEOUT_MINUTES 10
//! MAGIC NUMBER: how long we know about a milestone from iota at least before we process it and his transaction
//! to prevent loosing transaction which came after through a delay or to less delay in the race between threads
//! important if iota or the node server is running to slow because to many requests
//! TODO: Make value higher if a delay was detected
#define MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER_MILLI_SECONDS 900

//! MAGIC NUMBER: if current milestone is this diff or more away from last milestone from network,
//! we take extra care and wait MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER_MILLI_SECONDS before we proceed
//! maybe we are starting up this node and then we need more time
#define MAGIC_NUMBER_MILESTONE_ID_EXTRA_BUFFER_WAIT 10

class FinishMilestoneTask;
class OrderingManager : public UniLib::lib::Thread
{
    friend class FinishMilestoneTask;
public:
    ~OrderingManager();
    static OrderingManager* getInstance();

    void pushMilestoneTaskObserver(int32_t milestoneId);
    void popMilestoneTaskObserver(int32_t milestoneId);

    int pushTransaction(Poco::AutoPtr<model::GradidoTransaction> transaction, int32_t milestoneId, uint64_t timestamp);

    void pushPairedTransaction(Poco::AutoPtr<model::GradidoTransaction> transaction);
    //! \param outbound if true return outbound transaction, if false return inbound transaction
    Poco::AutoPtr<model::GradidoTransaction> findPairedTransaction(Poco::Timestamp pairedTransactionId, bool outbound);
    void checkExternGroupForPairedTransactions(const std::string& groupAlias);

    inline iota::MessageValidator* getIotaMessageValidator() { return &mMessageValidator; }

    

protected:
    OrderingManager();
   
	// called from thread
    int ThreadFunction();

    struct MilestoneTransactions
    {
        MilestoneTransactions(int32_t _milestoneId, int64_t _milestoneTimestamp)
            : milestoneId(_milestoneId), milestoneTimestamp(_milestoneTimestamp) {}

        int32_t milestoneId;
        int64_t milestoneTimestamp;
        Poco::Timestamp entryCreationTime;
        std::list<Poco::AutoPtr<model::GradidoTransaction>> transactions;
        Poco::FastMutex mutex;
    };

    struct CrossGroupTransactionPair
    {
        void setTransaction(Poco::AutoPtr<model::GradidoTransaction> transaction) {
            auto transfer = transaction->getTransactionBody()->getTransfer();
            if (transfer->isInbound()) {
                mInboundTransaction = transaction;
            }
            else if (transfer->isOutbound()) {
                mOutboundTransaction = transaction;
            }
        }
        Poco::AutoPtr<model::GradidoTransaction> mOutboundTransaction;
        Poco::AutoPtr<model::GradidoTransaction> mInboundTransaction;
    };

    iota::MessageValidator mMessageValidator;

    std::map<int32_t, int32_t> mMilestoneTaskObserver;
    Poco::FastMutex mMilestoneTaskObserverMutex;

    std::map<int32_t, MilestoneTransactions*> mMilestonesWithTransactions;
    Poco::FastMutex mMilestonesWithTransactionsMutex;

    // all paired transactions for validation which other group belong to this node
    Poco::ExpireCache<int64_t, CrossGroupTransactionPair> mPairedTransactions;
    Poco::FastMutex mPairedTransactionMutex;
    // message listener for groups on which this node normally isn't listening but which are needed for validate cross group transactions
    Poco::AccessExpireCache<std::string, iota::MessageListener> mUnlistenedPairGroups;
    Poco::FastMutex mUnlistenedPairGroupsMutex;

    Poco::FastMutex mFinishMilestoneTaskMutex;
};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
