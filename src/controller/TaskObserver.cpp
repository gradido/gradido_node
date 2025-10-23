#include "TaskObserver.h"
#include "../task/WriteTransactionsToBlockTask.h"
#include "../blockchain/FileBased.h" // IWYU pragma: keep


#include <cstring>

#include "loguru/loguru.hpp"

using namespace gradido::blockchain;


TaskObserver::TaskObserver()
{

}

TaskObserver::~TaskObserver()
{

}

bool TaskObserver::addBlockWriteTask(std::shared_ptr<task::WriteTransactionsToBlockTask> blockWriteTask)
{
	std::lock_guard _lock(mFastMutex);
	auto result = mBlockWriteTasks.insert(BlockWriteMapPair(blockWriteTask.get(), blockWriteTask));
	if (!result.second) {
		return false;
	}
	auto transactions = blockWriteTask->getTransactionEntriesList();
	mTransactionsFromPendingTasks.insert(transactions->begin(), transactions->end());
	
	return true;
}

bool TaskObserver::removeTask(task::Task* task)
{
	auto ressourceType = task->getResourceType();
	if (strcmp("WriteTransactionsToBlockTask", ressourceType) == 0) {
		auto blockWriteTask = (task::WriteTransactionsToBlockTask*)task;
		return removeBlockWriteTask(blockWriteTask);
	}
	return false;
}

bool TaskObserver::removeBlockWriteTask(task::WriteTransactionsToBlockTask* blockWriteTask)
{
	std::lock_guard _lock(mFastMutex);
	auto entry = mBlockWriteTasks.find(blockWriteTask);
	if (entry == mBlockWriteTasks.end()) {
		return false;
	}
	auto transactions = blockWriteTask->getTransactionEntriesList();
	for(auto it = transactions->begin(); it != transactions->end(); it++) {
		mTransactionsFromPendingTasks.erase(it->first);
	}

	mBlockWriteTasks.erase(entry);
	return true;

}

bool TaskObserver::isTransactionPending(uint64_t transactionNr) const
{
	std::lock_guard _lock(mFastMutex);
	if (mTransactionsFromPendingTasks.find(transactionNr) != mTransactionsFromPendingTasks.end()) {
		return true;
	}
	return false;
}

std::shared_ptr<NodeTransactionEntry> TaskObserver::getTransaction(uint64_t transactionNr)
{
	std::lock_guard _lock(mFastMutex);
	for(auto it = mBlockWriteTasks.begin(); it != mBlockWriteTasks.end(); it++) {
		auto transactionEntry = it->second->getTransaction(transactionNr);
		if(transactionEntry) {
			return transactionEntry;
		}
	}
	LOG_F(1, "cannot find transaction: %lu in write task", transactionNr);
	auto it = mTransactionsFromPendingTasks.find(transactionNr);
	if (it != mTransactionsFromPendingTasks.end()) {
		return it->second;
	}
	LOG_F(1, "cannot find transaction: %lu in map", transactionNr);
	return nullptr;
}

// *********************** Finish command **************************************
int TaskObserverFinishCommand::taskFinished(task::Task* task)
{ 
	mBlockchain->getTaskObserver().removeTask(task); 
	return 0; 
}