#include "OrderingManager.h"
#include "LoggerManager.h"
#include <stdexcept>
#include "../controller/Group.h"


OrderingManager::OrderingManager()
    : UniLib::lib::Thread("order"), mPairedTransactions(1000 * 1000 * 60 * MAGIC_NUMBER_CROSS_GROUP_TRANSACTION_CACHE_TIMEOUT_MINUTES)
{

}

OrderingManager::~OrderingManager()
{
    mMilestoneTaskObserverMutex.lock();
    
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

void OrderingManager::pushMilestoneTaskObserver(int32_t milestoneId)
{
    printf("[OrderingManager::pushMilestoneTaskObserver] push: %d\n", milestoneId);
    Poco::ScopedLock<Poco::FastMutex> _lock(mMilestoneTaskObserverMutex);
    auto it = mMilestoneTaskObserver.find(milestoneId);
    if (it == mMilestoneTaskObserver.end()) {
        mMilestoneTaskObserver.insert({ milestoneId, 1 });
    }
    else {
        it->second++;
    }
}
void OrderingManager::popMilestoneTaskObserver(int32_t milestoneId)
{
    printf("[OrderingManager::popMilestoneTaskObserver] pop: %d\n", milestoneId);
    Poco::ScopedLock<Poco::FastMutex> _lock(mMilestoneTaskObserverMutex);
    auto it = mMilestoneTaskObserver.find(milestoneId);
    if (it != mMilestoneTaskObserver.end()) {
        it->second--;
        if (it->second <= 0) {
            mMilestoneTaskObserver.erase(it);
            condSignal();
        }
    }
}

int OrderingManager::ThreadFunction()
{
    while (true) {
        // we can only work on the lowest known milestone if no task with this milestone is running
        // the milestone must be processed in order to keep transactions in order and this is essential!

        // get first not processed milestone
        mMilestonesWithTransactionsMutex.lock();
        auto workSetIt = mMilestonesWithTransactions.begin();
        mMilestonesWithTransactionsMutex.unlock();

		// Inserting into std::map does not invalidate existing iterators.
		// Q: https://stackoverflow.com/questions/4343220/does-insertion-to-stl-map-invalidate-other-existing-iterator
        
        if (workSetIt == mMilestonesWithTransactions.end()) {
			// we haven't any milestone at all, so we can exit now,
		    // thread will be called again if condSignal was called
            return 0;
        }
        auto milestoneId = workSetIt->first;   

		// make sure MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER_MILLI_SECONDS time was passed since milestone was created 
        // for more information see MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER_MILLI_SECONDS
		MilestoneTransactions* mt = workSetIt->second;
		Poco::Timestamp now;
        // Poco Timediff has a resolution of microseconds
		int64_t sleepMilliSeconds = MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER_MILLI_SECONDS - (now - mt->entryCreationTime) / 1000;

		if (sleepMilliSeconds > 0 && sleepMilliSeconds < MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER_MILLI_SECONDS) {
			Poco::Thread::sleep(sleepMilliSeconds);
            // check if our workSet is still the first
            if (workSetIt != mMilestonesWithTransactions.begin()) {
                continue;
            }
		}

        // check if the oldest milestone which has running task came after current milestone
        // milestone task observer are started before milestone with transactions entry
        mMilestoneTaskObserverMutex.lock();
        auto oldestMilestoneTaskObserverIt = mMilestoneTaskObserver.begin();
        mMilestoneTaskObserverMutex.unlock();
        if (oldestMilestoneTaskObserverIt != mMilestoneTaskObserver.end() && oldestMilestoneTaskObserverIt->first <= milestoneId) {
            return 0;
        }        
        
		if (mt->transactions.size())
		{
            // sort transaction after creation date if more than one was processed with this milestone
			mt->transactions.sort([](Poco::AutoPtr<model::GradidoTransaction> a, Poco::AutoPtr<model::GradidoTransaction> b) {
				return a->getTransactionBody()->getCreatedSeconds() < b->getTransactionBody()->getCreatedSeconds();
			});

			printf("[OrderingManager::finishedMilestone] milestone %d, transactions: %d\n", milestoneId, mt->transactions.size());

			for (auto itTransaction = mt->transactions.begin(); itTransaction != mt->transactions.end(); itTransaction++) {
				auto type = (*itTransaction)->getTransactionBody()->getType();
				auto seconds = (*itTransaction)->getTransactionBody()->getCreatedSeconds();
				printf("transaction type: %d, created: %d\n", type, seconds);

				Poco::AutoPtr<model::GradidoTransaction> transaction = *itTransaction;
                assert(!transaction->getGroupRoot().isNull());

                // put transaction to blockchain
		   		bool result = transaction->getGroupRoot()->addTransactionFromIota(transaction, mt->milestoneId, mt->milestoneTimestamp);
				if (!result) {
					transaction->addError(new Error(__FUNCTION__, "couldn't add transaction"));
                    transaction->addError(new ParamError(__FUNCTION__, "transaction", transaction->getJson()));
				}
			}
		}
        // remove not longer needed milestone transactions entry
        mMilestonesWithTransactionsMutex.lock();
        mMilestonesWithTransactions.erase(workSetIt);
        printf("[%s] processed milestone: %d\n", __FUNCTION__, milestoneId);
        mMilestonesWithTransactionsMutex.unlock();
    }
    return 0;
}


int OrderingManager::pushTransaction(Poco::AutoPtr<model::GradidoTransaction> transaction, int32_t milestoneId, uint64_t timestamp)
{
    Poco::ScopedLock<Poco::FastMutex> _lock(mMilestonesWithTransactionsMutex);
    auto it = mMilestonesWithTransactions.find(milestoneId);
    if (it == mMilestonesWithTransactions.end()) {
        auto insertResult = mMilestonesWithTransactions.insert({ milestoneId, new MilestoneTransactions(milestoneId, timestamp) });
        if (!insertResult.second) {
            throw std::runtime_error("cannot insert new milestone transactions");
        }
        it = insertResult.first;
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

void OrderingManager::checkExternGroupForPairedTransactions(const std::string& groupAlias)
{
    mUnlistenedPairGroupsMutex.lock();
    if (!mUnlistenedPairGroups.has(groupAlias)) {
        mUnlistenedPairGroups.add(groupAlias, new iota::MessageListener(groupAlias));
    }
    else {
        // get reset the timeout for removing it from access expire cache
        mUnlistenedPairGroups.get(groupAlias);
    }
    mUnlistenedPairGroupsMutex.unlock();
}
