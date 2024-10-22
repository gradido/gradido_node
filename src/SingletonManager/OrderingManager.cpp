#include "OrderingManager.h"
#include "GlobalStateManager.h"
#include "../blockchain/FileBased.h"
#include "../blockchain/FileBasedProvider.h"
#include "../blockchain/Exceptions.h"
#include "gradido_blockchain/interaction/toJson/Context.h"
#include "loguru/loguru.hpp"

#include <stdexcept>

using namespace gradido;
using namespace blockchain;
using namespace interaction;

OrderingManager::OrderingManager()
    : task::Thread("order")
{
    mMessageValidator.init();
}

OrderingManager::~OrderingManager()
{
    {
        std::lock_guard _lock(mMilestoneTaskObserverMutex);
        mMilestoneTaskObserver.clear();
    }
    {
        std::lock_guard _lock(mMilestonesWithTransactionsMutex);
        for (auto it = mMilestonesWithTransactions.begin(); it != mMilestonesWithTransactions.end(); it++) {
            delete it->second;
        }
        mMilestonesWithTransactions.clear();
    }
}

OrderingManager* OrderingManager::getInstance()
{
    static OrderingManager theOne;
    return &theOne;
}

void OrderingManager::pushMilestoneTaskObserver(int32_t milestoneId)
{
    //printf("[OrderingManager::pushMilestoneTaskObserver] push: %d\n", milestoneId);
    std::lock_guard _lock(mMilestoneTaskObserverMutex);
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
    std::lock_guard _lock(mMilestoneTaskObserverMutex);
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
    auto blockchainProvider = FileBasedProvider::getInstance();
    while (true) {        
        // we can only work on the lowest known milestone if no task with this milestone is running
        // the milestone must be processed in order to keep transactions in order and this is essential!
        // if node is starting up, wait until all existing messages are loaded from iota to prevent missing out one
        while (mMessageValidator.getFirstRunCount() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

		// make sure MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER time was passed since milestone was created 
        // for more information see MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER
		MilestoneTransactions* mt = workSetIt->second;
		Timepoint now;
		Duration sleepDuration = MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER - (now - mt->entryCreationTime);

        if (sleepDuration > std::chrono::milliseconds(0) && sleepDuration < MAGIC_NUMBER_MILESTONE_EXTRA_BUFFER) {
            std::this_thread::sleep_for(sleepDuration);
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
                return a.transaction->getTransactionBody()->getCreatedAt() < b.transaction->getTransactionBody()->getCreatedAt();
                });

            LOG_F(INFO, "milestone %d, transactions: %d", milestoneId, mt->transactions.size());

            for (auto itTransaction = mt->transactions.begin(); itTransaction != mt->transactions.end(); itTransaction++) {
                auto transaction = itTransaction->transaction.get();
                auto type = transaction->getTransactionBody()->getTransactionType();
                auto seconds = transaction->getTransactionBody()->getCreatedAt();

                // TODO: check if it is really necessary
                // auto transactionCopy = std::make_unique<gradido::data::GradidoTransaction>(transaction->getSerialized().get());
                // put transaction to blockchain
                auto blockchain = blockchainProvider->findBlockchain(itTransaction->communityId);
                if (blockchain) {
                    throw CommunityNotFoundExceptions("couldn't find group", itTransaction->communityId);
                }
                try {
                    bool result = blockchain->addGradidoTransaction(itTransaction->transaction, itTransaction->messageId, mt->milestoneTimestamp);
                }
                catch (GradidoBlockchainException& ex) {
                    auto fileBasedBlockchain = std::dynamic_pointer_cast<FileBased>(blockchain);
                    auto communityServer = fileBasedBlockchain->getListeningCommunityServer();
                    if (communityServer) {
                        communityServer->notificateFailedTransaction(*itTransaction->transaction, ex.what(), itTransaction->messageId->convertToHex());
                    }
                    LOG_F(INFO, "transaction not added: %s", ex.getFullString());
                    try {
                        toJson::Context toJson(*itTransaction->transaction);
                        LOG_F(INFO, "transaction: %s", toJson.run(true).data());
                    }
                    catch (GradidoBlockchainException& ex) {
                        LOG_F(ERROR, "gradido blockchain exception on parsing transaction: %s", ex.getFullString().data());
                    } catch(std::exception& ex) {
                        LOG_F(ERROR, "exception on parsing transaction: %s", ex.what());
                    }
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
    std::shared_ptr<const gradido::data::GradidoTransaction> transaction,
    int32_t milestoneId, Timepoint timestamp,
    std::string_view communityId, std::shared_ptr<const memory::Block> messageId)
{
    std::lock_guard _lock(mMilestonesWithTransactionsMutex);
    auto it = mMilestonesWithTransactions.find(milestoneId);
    if (it == mMilestonesWithTransactions.end()) {
        auto insertResult = mMilestonesWithTransactions.insert({ milestoneId, new MilestoneTransactions(milestoneId, timestamp) });
        if (!insertResult.second) {
            throw std::runtime_error("cannot insert new milestone transactions");
        }
        it = insertResult.first;
    }
    {
        std::lock_guard _lock(it->second->mutex);
        it->second->transactions.push_back(GradidoTransactionWithGroup(transaction, communityId, messageId));
    } 

    return 0;
}
