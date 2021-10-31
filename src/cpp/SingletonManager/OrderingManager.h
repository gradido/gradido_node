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

#include "../iota/MilestoneListener.h"
#include "../iota/Message.h"
#include "../model/transactions/GradidoTransaction.h"
#include "../task/IotaMessageToTransactionTask.h"
#include <map>

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
    Poco::AutoPtr<model::GradidoTransaction> findPairedTransaction(Poco::Timestamp pairedTransactionId);
    void removePairedTransaction(Poco::Timestamp pairedTransactionId);

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
    
    iota::MilestoneListener mIotaMilestoneListener;
    std::map<int32_t, int32_t*> mMilestoneTaskObserver;
    Poco::FastMutex mMilestoneTaskObserverMutex;

    std::map<int32_t, MilestoneTransactions*> mMilestonesWithTransactions;
    Poco::FastMutex mMilestonesWithTransactionsMutex;

    // all paired outbound transactions for validation which other group belong to this node
    std::unordered_map<int64_t, Poco::AutoPtr<model::GradidoTransaction>> mPairedTransactions;
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