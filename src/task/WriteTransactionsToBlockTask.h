#ifndef __GRADIDO_NODE_TASK_WRITE_TRANSACTIONS_TO_BLOCK_TASK_H
#define __GRADIDO_NODE_TASK_WRITE_TRANSACTIONS_TO_BLOCK_TASK_H

/*!
 * @author Dario Rekowski
 * @date 2020-03-11
 * @brief Task for collecting transactions and write it to block chain after timeout
 */

#include "CPUTask.h"
#include "../model/NodeTransactionEntry.h"
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
	WriteTransactionsToBlockTask(std::shared_ptr<model::files::Block> blockFile, std::shared_ptr<controller::BlockIndex> blockIndex);
	~WriteTransactionsToBlockTask();

	const char* getResourceType() const { return "WriteTransactionsToBlockTask"; };
	int run();

	//! \brief return creation date of object
	//! 
	//! no mutex lock, value doesn't change, set in WriteTransactionsToBlockTask()
	inline Timepoint getCreationDate() { return mCreationDate; }

	void addSerializedTransaction(std::shared_ptr<model::NodeTransactionEntry> transaction);

	//! return transaction by nr
	std::shared_ptr<model::NodeTransactionEntry> getTransaction(uint64_t nr);

	inline const std::map<uint64_t, std::shared_ptr<model::NodeTransactionEntry>>* getTransactionEntriesList() const {
		return &mTransactions;
	}

protected:
	std::shared_ptr<model::files::Block> mBlockFile;
	std::shared_ptr<controller::BlockIndex> mBlockIndex;
	Timepoint mCreationDate;
	Poco::FastMutex mFastMutex;
	std::map<uint64_t, std::shared_ptr<model::NodeTransactionEntry>> mTransactions;
};

#endif 