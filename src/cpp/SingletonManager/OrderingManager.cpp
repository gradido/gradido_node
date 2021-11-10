#include "OrderingManager.h"
#include "LoggerManager.h"
#include <stdexcept>
#include "../controller/Group.h"


OrderingManager::OrderingManager() 
:mPairedTransactions(1000 * 1000 * 60 * MAGIC_NUMBER_CROSS_GROUP_TRANSACTION_CACHE_TIMEOUT_MINUTES)
{
}

OrderingManager::~OrderingManager()
{
    mMilestoneTaskObserverMutex.lock();
    for (auto it = mMilestoneTaskObserver.begin(); it != mMilestoneTaskObserver.end(); it++) {
        delete it->second;
    }
    mMilestoneTaskObserver.clear();
    mMilestoneTaskObserverMutex.unlock();

    mMilestonesWithTransactionsMutex.lock();
    for (auto it = mMilestonesWithTransactions.begin(); it != mMilestonesWithTransactions.end(); it++) {
        delete it->second;
    }
    mMilestonesWithTransactions.clear();

    mPairedTransactionMutex.lock();
    // print it out on shutdown to see if all transactions are always deleted or
    // if we need a clear process to check from time to time if there unneeded paired transactions are left over
    if (mPairedTransactions.size()) {
        std::clog << "OrderingManager::~OrderingManager" << " left over paired transactions: " << std::to_string(mPairedTransactions.size()) << std::endl;
    }
    mPairedTransactionMutex.unlock();

    mMilestonesWithTransactionsMutex.unlock();
}

OrderingManager* OrderingManager::getInstance()
{
    static OrderingManager theOne;
    return &theOne;
}

void OrderingManager::pushMilestoneTaskObserver(int32_t milestoneId, int64_t milestoneTimestamp)
{
    Poco::ScopedLock<Poco::FastMutex> _lock(mMilestoneTaskObserverMutex);
    auto it = mMilestoneTaskObserver.find(milestoneId);
    if (it == mMilestoneTaskObserver.end()) {
        mMilestoneTaskObserver.insert({ milestoneId, new int32_t(1) });

		mMilestonesWithTransactionsMutex.lock();
		mMilestonesWithTransactions.insert({ milestoneId, new MilestoneTransactions(milestoneId, milestoneTimestamp )});
		mMilestonesWithTransactionsMutex.unlock();
    }
    else {
        (*it->second)++;
    }
}
void OrderingManager::popMilestoneTaskObserver(int32_t milestoneId)
{
    Poco::ScopedLock<Poco::FastMutex> _lock(mMilestoneTaskObserverMutex);
    auto it = mMilestoneTaskObserver.find(milestoneId);
    if (it != mMilestoneTaskObserver.end()) {
        (*it->second)--;
		if (*it->second <= 0) {
            delete it->second;
            mMilestoneTaskObserver.erase(it);
            // call finishedMilestone in task because it could need some time
            Poco::AutoPtr<FinishMilestoneTask> task = new FinishMilestoneTask(milestoneId);
            task->scheduleTask(task);
		}
    }
}

void OrderingManager::finishedMilestone(int32_t milestoneId)
{
    // exit task if another is already running, because he will go one as long there is something to work on
    // TODO: make own thread for this, because to keep transactions in order only one thread is allowed to run 
    // TODO: put transactions in queues per group and work on this per group base to reduce the impact of malicious transactions
    // for the whole node (cross group transactions with missing pair)
    if (!mFinishMilestoneTaskMutex.tryLock()) return;
    mFinishMilestoneTaskMutex.unlock();
    // enforce that the finishing milestone tasks must wait on each other
    Poco::ScopedLock<Poco::FastMutex> _lockRoot(mFinishMilestoneTaskMutex);

    // now every message and transaction belonging to this milestone should be finished
    { // scoped lock
        Poco::ScopedLock<Poco::FastMutex> _lock(mMilestoneTaskObserverMutex);
		for (auto it = mMilestoneTaskObserver.begin(); it != mMilestoneTaskObserver.end(); it++) {
            // if a milestone before us wasn't processed, skip
			if (it->first < milestoneId) {
                return;
			}
		}
    } // end scoped lock

    
    mMilestonesWithTransactionsMutex.lock();
    auto it = mMilestonesWithTransactions.find(milestoneId);
    mMilestonesWithTransactionsMutex.unlock();
    // end return iterator past last entry so it should be save, even if a new entry is added in another thread
    if (it == mMilestonesWithTransactions.end()) {   
        // our entry wasn't found, maybe it was already processed by another thread?
        return;
    }
    MilestoneTransactions* mt = it->second;
    if (mt->transactions.size()) 
    {
        printf("[OrderingManager::finishedMilestone] before sort:\n");
        for (auto itTransaction = mt->transactions.begin(); itTransaction != mt->transactions.end(); itTransaction++) {
            auto type = (*itTransaction)->getTransactionBody()->getType();
            auto seconds = (*itTransaction)->getTransactionBody()->getCreatedSeconds();
            printf("transaction type: %d, created: %d\n", type, seconds);
        }
        // sort after creation date
        mt->transactions.sort([](Poco::AutoPtr<model::GradidoTransaction> a, Poco::AutoPtr<model::GradidoTransaction> b) {
            return a->getTransactionBody()->getCreatedSeconds() < b->getTransactionBody()->getCreatedSeconds();
            });
            
        printf("[OrderingManager::finishedMilestone] milestone %d, transactions: %d\n", milestoneId, mt->transactions.size());
            
        printf("[OrderingManager::finishedMilestone] after sort:\n");
        for (auto itTransaction = mt->transactions.begin(); itTransaction != mt->transactions.end(); itTransaction++) {
			auto type = (*itTransaction)->getTransactionBody()->getType();
			auto seconds = (*itTransaction)->getTransactionBody()->getCreatedSeconds();
			printf("transaction type: %d, created: %d\n", type, seconds);

            Poco::AutoPtr<model::GradidoTransaction> transaction = *itTransaction;
            bool result = transaction->getGroupRoot()->addTransactionFromIota(transaction, mt->milestoneId, mt->milestoneTimestamp);
            if (!result) {
                transaction->addError(new Error("OrderingManager::finishedMilestone", "couldn't add transaction"));
            }
        }
    }

    // after finish
    { // scoped lock
        Poco::ScopedLock<Poco::FastMutex> _lock1(mMilestoneTaskObserverMutex);
        Poco::ScopedLock<Poco::FastMutex> _lock2(mMilestonesWithTransactionsMutex);
        // remove not longer needed milestone transactions entry
        // Inserting into std::map does not invalidate existing iterators.
        // Q: https://stackoverflow.com/questions/4343220/does-insertion-to-stl-map-invalidate-other-existing-iterator
        mMilestonesWithTransactions.erase(it);
        auto it1 = mMilestoneTaskObserver.find(milestoneId + 1);
        auto it2 = mMilestonesWithTransactions.find(milestoneId + 1);
		if (it1 == mMilestoneTaskObserver.end() && it2 != mMilestonesWithTransactions.end()) {
            // if a milestone after us is ready for processing (and exist), process him, maybe he was waiting of us
			return finishedMilestone(milestoneId + 1);
		}
    } // end scoped lock
}

int OrderingManager::pushTransaction(Poco::AutoPtr<model::GradidoTransaction> transaction, int32_t milestoneId)
{
    Poco::ScopedLock<Poco::FastMutex> _lock(mMilestonesWithTransactionsMutex);
    auto it = mMilestonesWithTransactions.find(milestoneId);
    if (it == mMilestonesWithTransactions.end()) {
        std::string error = "cannot find milestone with transaction for milestone with index:  ";
        error += std::to_string(milestoneId);
        throw std::runtime_error(error);
    }
    it->second->mutex.lock();
    it->second->transactions.push_back(transaction);
    it->second->mutex.unlock();

    return 0;
}

void OrderingManager::pushPairedTransaction(Poco::AutoPtr<model::GradidoTransaction> transaction)
{
    Poco::ScopedLock<Poco::FastMutex> _lock(mPairedTransactionMutex);
    assert(transaction->getTransactionBody()->isTransfer());
    auto transfer = transaction->getTransactionBody()->getTransfer();
    auto pairedTransactionId = transfer->getPairedTransactionId().raw();
    if (!mPairedTransactions.has(pairedTransactionId)) {
        mPairedTransactions.add(pairedTransactionId, new CrossGroupTransactionPair);
    }
    auto pair = mPairedTransactions.get(pairedTransactionId);    
    pair->setTransaction(transaction);
}

Poco::AutoPtr<model::GradidoTransaction> OrderingManager::findPairedTransaction(Poco::Timestamp pairedTransactionId, bool outbound)
{
    Poco::ScopedLock<Poco::FastMutex> _lock(mPairedTransactionMutex);
    auto pair = mPairedTransactions.get(pairedTransactionId.raw());
    if (!pair.isNull()) {
        if (outbound) {
            return pair->mOutboundTransaction;
        }
        else {
            return pair->mInboundTransaction;
        }
    }
    return nullptr;
}

