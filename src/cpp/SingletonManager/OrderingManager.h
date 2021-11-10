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
#include <map>
#include "Poco/ExpireCache.h"

//! MAGIC NUMBER: how many minutes every mPairedTransactions entry should be stored in cache
//! depends how long the node server needs to process cross group transactions
//! if number is to small, transaction cannot be validated and can be get lost
//! TODO: Find a better approach to prevent loosing transactions through timeout
#define MAGIC_NUMBER_CROSS_GROUP_TRANSACTION_CACHE_TIMEOUT_MINUTES 10

class FinishMilestoneTask;
class OrderingManager
{
    friend class FinishMilestoneTask;
public: 
    ~OrderingManager();
    static OrderingManager* getInstance();
    
    void pushMilestoneTaskObserver(int32_t milestoneId, int64_t milestoneTimestamp);
    void popMilestoneTaskObserver(int32_t milestoneId);

    int pushTransaction(Poco::AutoPtr<model::GradidoTransaction> transaction, int32_t milestoneId);

    void pushPairedTransaction(Poco::AutoPtr<model::GradidoTransaction> transaction);
    //! \param outbound if true return outbound transaction, if false return inbound transaction
    Poco::AutoPtr<model::GradidoTransaction> findPairedTransaction(Poco::Timestamp pairedTransactionId, bool outbound);

    inline iota::MessageValidator* getIotaMessageValidator() { return &mMessageValidator; }

protected:
    OrderingManager();
    void finishedMilestone(int32_t milestoneId);

    struct MilestoneTransactions 
    {
        MilestoneTransactions(int32_t _milestoneId, int64_t _milestoneTimestamp)
            : milestoneId(_milestoneId), milestoneTimestamp(_milestoneTimestamp) {}

        int32_t milestoneId;
        int64_t milestoneTimestamp;
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

    std::map<int32_t, int32_t*> mMilestoneTaskObserver;
    Poco::FastMutex mMilestoneTaskObserverMutex;

    std::map<int32_t, MilestoneTransactions*> mMilestonesWithTransactions;
    Poco::FastMutex mMilestonesWithTransactionsMutex;

    // all paired transactions for validation which other group belong to this node
    Poco::ExpireCache<int64_t, CrossGroupTransactionPair> mPairedTransactions;
    Poco::FastMutex mPairedTransactionMutex;

    Poco::FastMutex mFinishMilestoneTaskMutex;
};

class FinishMilestoneTask : public UniLib::controller::CPUTask
{
public:
    FinishMilestoneTask(int32_t milestoneId) : mMilestoneId(milestoneId) {};
    ~FinishMilestoneTask() {};

    int run() { OrderingManager::getInstance()->finishedMilestone(mMilestoneId); return 0; }
protected:
    int32_t mMilestoneId;
};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER