#ifndef __GRADIDO_NODE_TASK_WRITE_TRANSACTIONS_TO_BLOCK_TASK_H
#define __GRADIDO_NODE_TASK_WRITE_TRANSACTIONS_TO_BLOCK_TASK_H

/*!
 * @author Dario Rekowski
 * @date 2020-03-11
 * @brief Task for collecting transactions and write it to block chain after timeout
 */

#include "CPUTask.h"
#include "gradido_blockchain/lib/MultithreadQueue.h"

namespace cache {
	class BlockIndex;
}

namespace model {
	namespace files {
		class Block;
	}
}

namespace gradido {
	namespace blockchain {
		class NodeTransactionEntry;
	}
}

/*! 
 * @author Dario Rekowski
 * @date 202-03-11
 * @brief Task for collecting new transactions for block and write down to disk after timeout
 */

class WriteTransactionsToBlockTask : public task::CPUTask
{
public:
	WriteTransactionsToBlockTask(std::shared_ptr<model::files::Block> blockFile, std::shared_ptr<cache::BlockIndex> blockIndex);
	~WriteTransactionsToBlockTask();

	const char* getResourceType() const { return "WriteTransactionsToBlockTask"; };
	int run();

	//! \brief return creation date of object
	//! 
	//! no mutex lock, value doesn't change, set in WriteTransactionsToBlockTask()
	inline Timepoint getCreationDate() { return mCreationDate; }

	void addSerializedTransaction(std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transaction);

	//! return transaction by nr
	std::shared_ptr<gradido::blockchain::NodeTransactionEntry> getTransaction(uint64_t nr);

	inline const std::map<uint64_t, std::shared_ptr<gradido::blockchain::NodeTransactionEntry>>* getTransactionEntriesList() const {
		return &mTransactions;
	}

protected:
	std::shared_ptr<model::files::Block> mBlockFile;
	std::shared_ptr<cache::BlockIndex> mBlockIndex;
	Timepoint mCreationDate;
	std::mutex mFastMutex;
	std::map<uint64_t, std::shared_ptr<gradido::blockchain::NodeTransactionEntry>> mTransactions;
};

#endif 