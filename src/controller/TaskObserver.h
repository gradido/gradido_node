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

#include "gradido_blockchain/lib/DRHashList.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"
#include "../task/Task.h"

#include <vector>
#include <map>
#include <mutex>

enum class TaskObserverType {
	WRITE_BLOCK,
	COUNT,
	INVALID
};

/*! 
 * @author Dario Rekowski
 * @date 2020-03-12
 * @brief Container for keeping track of running tasks
 *
 * to check if a specific transaction was already insert and waiting to be written
 */

class WriteTransactionsToBlockTask;

namespace gradido {
	namespace blockchain {
		class NodeTransactionEntry;
		class FileBased;
	}
}

class TaskObserver : public MultithreadContainer
{
public:
	TaskObserver();
	~TaskObserver();
	
	//! \brief adding WriteTransactionsToBlockTask to map and read transactions
	//! \return false if tasks already exist
	bool addBlockWriteTask(std::shared_ptr<WriteTransactionsToBlockTask> blockWriteTask);

	//! \brief remove WriteTransactionsToBlockTask from map and transactions
	//! \return false if entry not found, else return true
	bool removeBlockWriteTask(WriteTransactionsToBlockTask* blockWriteTask);

	//! \brief remove Tasks
	//! \return return false if task not found or unknown type, else true
	bool removeTask(task::Task* task);
	
	//! \brief check if one of the pending WriteTransactionsToBlockTask contain this transaction
	//! \return true if transaction is pending and false if not
	bool isTransactionPending(uint64_t transactionNr) const;

	//! \brief check if one of the pending WriteTransactionsToBlockTask contain this transaction
	//! \return the transaction in question
	std::shared_ptr<gradido::blockchain::NodeTransactionEntry> getTransaction(uint64_t transactionNr);

	inline size_t getPendingTasksCount() const { std::lock_guard _lock(mFastMutex); return mBlockWriteTasks.size(); }

protected:
	mutable std::mutex mFastMutex;
	typedef std::pair<WriteTransactionsToBlockTask*, std::shared_ptr<WriteTransactionsToBlockTask>> BlockWriteMapPair;
	std::map<WriteTransactionsToBlockTask*, std::shared_ptr<WriteTransactionsToBlockTask>> mBlockWriteTasks;
	typedef std::pair<uint64_t, std::shared_ptr<WriteTransactionsToBlockTask>> TransactionsForTasksPair;
	std::map<uint64_t, std::shared_ptr<gradido::blockchain::NodeTransactionEntry>> mTransactionsFromPendingTasks;
	
};

class TaskObserverFinishCommand : public task::Command
{
public:
	TaskObserverFinishCommand(std::shared_ptr<gradido::blockchain::FileBased> blockchain) : mBlockchain(blockchain) {}

	int taskFinished(task::Task* task);

protected:
	std::shared_ptr<gradido::blockchain::FileBased> mBlockchain;
};

#endif //DR_GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_SINGLETON_TASK_OBSERVER_H