/*!
*
* \author: einhornimmond
*
* \date: 10.01.20
*
* \brief: observe tasks, for example passwort creation or transactions
*/

#ifndef DR_GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_SINGLETON_TASK_OBSERVER_H
#define DR_GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_SINGLETON_TASK_OBSERVER_H

#include "Poco/Mutex.h"
#include "Poco/AutoPtr.h"
#include "Poco/SharedPtr.h"

#include "gradido_blockchain/lib/DRHashList.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"
#include "../task/Task.h"


#include <vector>
#include <map>
#include <mutex>

enum TaskObserverType {
	TASK_OBSERVER_WRITE_BLOCK,
	TASK_OBSERVER_COUNT,
	TASK_OBSERVER_INVALID
};

/*! 
 * @author Dario Rekowski
 * @date 2020-03-12
 * @brief Container for keeping track of running tasks
 *
 * to check if a specific transaction was already insert and waiting to be written
 */

class WriteTransactionsToBlockTask;

namespace model {
	class NodeTransactionEntry;
}

class TaskObserver : public MultithreadContainer
{
public:
	TaskObserver();
	~TaskObserver();
	
	//! \brief adding WriteTransactionsToBlockTask to map and read transactions
	//! \return false if tasks already exist
	bool addBlockWriteTask(Poco::AutoPtr<WriteTransactionsToBlockTask> blockWriteTask);

	//! \brief remove WriteTransactionsToBlockTask from map and transactions
	//! \return false if entry not found, else return true
	bool removeBlockWriteTask(WriteTransactionsToBlockTask* blockWriteTask);

	//! \brief remove Tasks
	//! \return return false if task not found or unknown type, else true
	bool removeTask(task::Task* task);
	
	//! \brief check if one of the pending WriteTransactionsToBlockTask contain this transaction
	//! \return true if transaction is pending and false if not
	bool isTransactionPending(uint64_t transactionNr);

	//! \brief check if one of the pending WriteTransactionsToBlockTask contain this transaction
	//! \return the transaction in question
	Poco::SharedPtr<model::NodeTransactionEntry> getTransaction(uint64_t transactionNr);

	inline size_t getPendingTasksCount() const { std::lock_guard _lock(mFastMutex); return mBlockWriteTasks.size(); }

	static const char* TaskObserverTypeToString(TaskObserverType type);
	static TaskObserverType StringToTaskObserverType(const std::string& typeString);

protected:
	mutable std::mutex mFastMutex;
	typedef std::pair<WriteTransactionsToBlockTask*, Poco::AutoPtr<WriteTransactionsToBlockTask>> BlockWriteMapPair;
	std::map<WriteTransactionsToBlockTask*, Poco::AutoPtr<WriteTransactionsToBlockTask>> mBlockWriteTasks;
	typedef std::pair<uint64_t, Poco::AutoPtr<WriteTransactionsToBlockTask>> TransactionsForTasksPair;
	std::map<uint64_t, Poco::SharedPtr<model::NodeTransactionEntry>> mTransactionsFromPendingTasks;
	
};

class TaskObserverFinishCommand : public task::Command
{
public:
	TaskObserverFinishCommand(TaskObserver* taskObserver) : mTaskObserver(taskObserver) {}

	int taskFinished(task::Task* task) { mTaskObserver->removeTask(task); return 0; }

protected:
	TaskObserver* mTaskObserver;
};

#endif //DR_GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_SINGLETON_TASK_OBSERVER_H