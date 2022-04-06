#include "OrderingManager.h"
#include "LoggerManager.h"
#include "GlobalStateManager.h"
#include "GroupManager.h"
#include <stdexcept>
#include "../controller/Group.h"
#include "../controller/ControllerExceptions.h"

#include "Poco/DateTimeFormatter.h"

OrderingManager::OrderingManager()
    : task::Thread("order")
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
    mMilestonesWithTransactionsMutex.unlock();
}

OrderingManager* OrderingManager::getInstance()
{
    static OrderingManager theOne;
    return &theOne;
}

void OrderingManager::pushMilestoneTaskObserver(int32_t milestoneId)
{
    //printf("[OrderingManager::pushMilestoneTaskObserver] push: %d\n", milestoneId);
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
    //printf("[OrderingManager::popMilestoneTaskObserver] pop: %d\n", milestoneId);
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
    auto gm = GroupManager::getInstance();
    while (true) {
        
        // we can only work on the lowest known milestone if no task with this milestone is running
        // the milestone must be processed in order to keep transactions in order and this is essential!
        // if node is starting up, wait until all existing messages are loaded from iota to prevent missing out one
        while (mMessageValidator.getFirstRunCount() > 0) {
            Poco::Thread::sleep(100);
        }
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
            mt->transactions.sort([](const GradidoTransactionWithGroup& a, const GradidoTransactionWithGroup& b) {
                return a.transaction->getTransactionBody()->getCreatedSeconds() < b.transaction->getTransactionBody()->getCreatedSeconds();
                });

            printf("[OrderingManager::finishedMilestone] milestone %d, transactions: %d\n", milestoneId, mt->transactions.size());

            for (auto itTransaction = mt->transactions.begin(); itTransaction != mt->transactions.end(); itTransaction++) {
                auto transaction = itTransaction->transaction.get();
                auto type = transaction->getTransactionBody()->getTransactionType();
                auto seconds = transaction->getTransactionBody()->getCreatedSeconds();
                printf("transaction type: %d, created: %d\n", type, seconds);
              
                // TODO: check if it is really necessary
                auto transactionCopy = std::make_unique<model::gradido::GradidoTransaction>(transaction->getSerialized().get());
                // put transaction to blockchain
                auto group = gm->findGroup(itTransaction->groupAlias);
                if (group.isNull()) {
                    throw controller::GroupNotFoundException("couldn't find group", itTransaction->groupAlias);
                }
                try {
                    bool result = group->addTransaction(std::move(itTransaction->transaction), itTransaction->messageId, mt->milestoneTimestamp);
                }
                catch (GradidoBlockchainException& ex) {
                    Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
                    auto communityServer = group->getListeningCommunityServer();
                    if (communityServer) {
                        communityServer->notificateFailedTransaction(transactionCopy.get(), ex.what(), *itTransaction->messageId->convertToHex().get());
                    }
                    errorLog.information("[OrderingManager] transaction not added: %s", ex.getFullString());
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


int OrderingManager::pushTransaction(
    std::unique_ptr<model::gradido::GradidoTransaction> transaction, 
    int32_t milestoneId, uint64_t timestamp, 
    const std::string& groupAlias, MemoryBin* messageId)
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
    it->second->transactions.push_back(GradidoTransactionWithGroup(std::move(transaction), groupAlias, messageId));
    it->second->mutex.unlock();

    return 0;
}
