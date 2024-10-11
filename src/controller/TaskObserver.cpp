#include "TaskObserver.h"
#include "../task/WriteTransactionsToBlockTask.h"
#include "../blockchain/FileBased.h"
#include <cstring>

using namespace gradido::blockchain;


TaskObserver::TaskObserver()
{

}

TaskObserver::~TaskObserver()
{

}

bool TaskObserver::addBlockWriteTask(std::shared_ptr<WriteTransactionsToBlockTask> blockWriteTask)
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
		auto blockWriteTask = (WriteTransactionsToBlockTask*)task;
		return removeBlockWriteTask(blockWriteTask);
	}
	return false;
}

bool TaskObserver::removeBlockWriteTask(WriteTransactionsToBlockTask* blockWriteTask)
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
	std::clog << "cannot find transaction " << transactionNr << " in write task" << std::endl;
	auto it = mTransactionsFromPendingTasks.find(transactionNr);
	if (it != mTransactionsFromPendingTasks.end()) {
		return it->second;
	}
	std::clog << "cannot find transaction " << transactionNr << " in map" << std::endl;
	return nullptr;
}

// *********************** Finish command **************************************
int TaskObserverFinishCommand::taskFinished(task::Task* task)
{ 
	mBlockchain->getTaskObserver().removeTask(task); 
	return 0; 
}