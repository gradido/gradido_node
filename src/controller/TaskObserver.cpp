#include "TaskObserver.h"
#include "../task/WriteTransactionsToBlockTask.h"

#include <cstring>


TaskObserver::TaskObserver()
{

}

TaskObserver::~TaskObserver()
{

}

bool TaskObserver::addBlockWriteTask(Poco::AutoPtr<WriteTransactionsToBlockTask> blockWriteTask)
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	auto result = mBlockWriteTasks.insert(BlockWriteMapPair(blockWriteTask.get(), blockWriteTask));
	if (!result.second) {
		return false;
	}
	auto transactionNrs = blockWriteTask->getTransactionNrs();
	for (auto it = transactionNrs.begin(); it != transactionNrs.end(); it++) {
		mTransactionsForTasks.insert(TransactionsForTasksPair(*it, blockWriteTask));
	}
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
	mBlockWriteTasks.erase(entry);
	auto transactionNrs = blockWriteTask->getTransactionNrs();
	for (auto it = transactionNrs.begin(); it != transactionNrs.end(); it++) {
		//mTransactionsForTasks.insert(TransactionsForTasksPair(*it, blockWriteTask));
		mTransactionsForTasks.erase(*it);
	}
	return true;

}

bool TaskObserver::isTransactionPending(uint64_t transactionNr)
{
	Poco::FastMutex::ScopedLock lock(mFastMutex);
	if (mTransactionsForTasks.find(transactionNr) != mTransactionsForTasks.end()) {
		return true;
	}
	return false;
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
