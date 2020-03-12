#ifndef __GRADIDO_NODE_TASK_WRITE_TRANSACTIONS_TO_BLOCK_TASK_H
#define __GRADIDO_NODE_TASK_WRITE_TRANSACTIONS_TO_BLOCK_TASK_H

/*!
 * @author Dario Rekowski
 * @date 2020-03-11
 * @brief Task for collecting transactions and write it to block chain after timeout
 */

#include "CPUTask.h"
#include "../controller/Block.h"
#include "../lib/MultithreadQueue.h"
#include "../lib/ErrorList.h"

#include "Poco/Timestamp.h"

/*! 
 * @author Dario Rekowski
 * @date 202-03-11
 * @brief Task for collecting new transactions for block and write down to disk after timeout
 */

class WriteTransactionsToBlockTask : public UniLib::controller::CPUTask, public ErrorList
{
public:
	WriteTransactionsToBlockTask(Poco::AutoPtr<model::files::Block> blockFile);
	~WriteTransactionsToBlockTask();

	const char* getResourceType() const { return "WriteTransactionsToBlockTask"; };
	int run();

	//! \brief return creation date of object
	//! 
	//! no mutex lock, value doesn't change, set in WriteTransactionsToBlockTask()
	inline Poco::Timestamp getCreationDate() { return mCreationDate; }

	inline void addSerializedTransaction(Poco::SharedPtr<controller::TransactionEntry> transaction) { 
		Poco::FastMutex::ScopedLock lock(mFastMutex);	 
		mTransactions.push_back(transaction);
	}

protected:
	Poco::AutoPtr<model::files::Block> mBlockFile;
	Poco::Timestamp mCreationDate;
	Poco::FastMutex mFastMutex;
	std::list<Poco::SharedPtr<controller::TransactionEntry>> mTransactions;
};

#endif 