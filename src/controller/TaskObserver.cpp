#include "TaskObserver.h"
#include "../task/WriteTransactionsToBlockTask.h"
#include <cstring>


TaskObserver::TaskObserver()
{

}

TaskObserver::~TaskObserver()
{

}

bool TaskObserver::addBlockWriteTask(std::shared_ptr<WriteTransactionsToBlockTask> blockWriteTask)
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
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
	Poco::FastMutex::ScopedLock lock(mFastMutex);
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

bool TaskObserver::isTransactionPending(uint64_t transactionNr)
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	if (mTransactionsFromPendingTasks.find(transactionNr) != mTransactionsFromPendingTasks.end()) {
		return true;
	}
	return false;
}

std::shared_ptr<model::NodeTransactionEntry> TaskObserver::getTransaction(uint64_t transactionNr)
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	for(auto it = mBlockWriteTasks.begin(); it != mBlockWriteTasks.end(); it++) {
		auto transactionEntry = it->second->getTransaction(transactionNr);
		if(!transactionEntry.isNull()) {
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

const char* TaskObserver::TaskObserverTypeToString(TaskObserverType type)
{
	switch (type) {
	case TASK_OBSERVER_WRITE_BLOCK: return "write block";
	case TASK_OBSERVER_COUNT: return "COUNT";
	case TASK_OBSERVER_INVALID: return "INVALID";
	default: return "UNKNOWN";
	}
}

TaskObserverType TaskObserver::StringToTaskObserverType(const std::string& typeString)
{
	if ("write block" == typeString) {
		return TASK_OBSERVER_WRITE_BLOCK;
	}
	return TASK_OBSERVER_INVALID;
}

// *********************** Finish command **************************************
