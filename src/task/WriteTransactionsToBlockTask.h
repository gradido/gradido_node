#ifndef __GRADIDO_NODE_TASK_WRITE_TRANSACTIONS_TO_BLOCK_TASK_H
#define __GRADIDO_NODE_TASK_WRITE_TRANSACTIONS_TO_BLOCK_TASK_H

/*!
 * @author Dario Rekowski
 * @date 2020-03-11
 * @brief Task for collecting transactions and write it to block chain after timeout
 */

#include "CPUTask.h"
#include "../model/TransactionEntry.h"
#include "../model/files/Block.h"
#include "gradido_blockchain/lib/MultithreadQueue.h"

#include "../controller/BlockIndex.h"

#include "Poco/Timestamp.h"

/*! 
 * @author Dario Rekowski
 * @date 202-03-11
 * @brief Task for collecting new transactions for block and write down to disk after timeout
 */

class WriteTransactionsToBlockTask : public task::CPUTask
{
public:
	WriteTransactionsToBlockTask(Poco::AutoPtr<model::files::Block> blockFile, Poco::SharedPtr<controller::BlockIndex> blockIndex);
	~WriteTransactionsToBlockTask();

	const char* getResourceType() const { return "WriteTransactionsToBlockTask"; };
	int run();

	//! \brief return creation date of object
	//! 
	//! no mutex lock, value doesn't change, set in WriteTransactionsToBlockTask()
	inline Poco::Timestamp getCreationDate() { return mCreationDate; }

	inline void addSerializedTransaction(Poco::SharedPtr<model::TransactionEntry> transaction) { 
		Poco::FastMutex::ScopedLock lock(mFastMutex);	 
		mTransactions.push_back(transaction);
		mBlockIndex->addIndicesForTransaction(transaction);
	}

	//! return transaction by nr
	Poco::SharedPtr<model::TransactionEntry> getTransaction(uint64_t nr);

	//! \brief collect all transaction nrs from transactions
	std::vector<uint64_t> getTransactionNrs();

protected:
	Poco::AutoPtr<model::files::Block> mBlockFile;
	Poco::SharedPtr<controller::BlockIndex> mBlockIndex;
	Poco::Timestamp mCreationDate;
	Poco::FastMutex mFastMutex;
	std::list<Poco::SharedPtr<model::TransactionEntry>> mTransactions;
};

#endif 