#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_H

#include "BlockIndex.h"

#include "../controller/TaskObserver.h"
#include "../lib/FuzzyTimer.h"
#include "../task/WriteTransactionsToBlockTask.h"

#include "gradido_blockchain/lib/AccessExpireCache.h"
#include "gradido_blockchain/blockchain/TransactionEntry.h"

#include <map>

namespace model {
	namespace files {
		class Block;
	}
}

namespace gradido {
	namespace blockchain {
		class FileBased;
		class NodeTransactionEntry;
	}
}

// TODO: move into config
#define GRADIDO_NODE_CACHE_BLOCK_MAX_FILE_SIZE_BYTE 128 * 1024 * 1024

namespace cache {
	class Group;

	/*! 
	 * @author Dario Rekowski
	 * @date 2020-02-06
	 * @brief interface for adding and getting transactions from specific block
	 */

	class Block : public TimerCallback
	{
	public:
		Block(uint32_t blockNr, std::shared_ptr<gradido::blockchain::FileBased> blockchain);
		~Block();

		//! \return false if block file has errors
		bool init();
		void exit();

		//! \brief put new transaction to cache and file system
		bool pushTransaction(std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transaction);
		
		//! \brief load transaction from cache or file system
		std::shared_ptr<gradido::blockchain::NodeTransactionEntry> getTransaction(uint64_t transactionNr);

		inline std::shared_ptr<BlockIndex> getBlockIndex() { return mBlockIndex; }

		bool hasSpaceLeft();

		//! \brief called from timeout manager for scheduling WriteTransactionsToBlockTask 
		TimerReturn callFromTimer();
		const char* getResourceType() const { return "controller::Block"; };
		
	protected:
		//! \brief add transaction from Block File, called by Block File, adding to cache and index
		void addTransaction(memory::ConstBlockPtr serializedTransaction, int32_t fileCursor);
		
		std::mutex mFastMutex;
		uint32_t mBlockNr;		

		AccessExpireCache<uint64_t, std::shared_ptr<gradido::blockchain::NodeTransactionEntry>> mSerializedTransactions;

		std::shared_ptr<BlockIndex> mBlockIndex;
		std::shared_ptr<model::files::Block> mBlockFile;
		std::shared_ptr<WriteTransactionsToBlockTask> mTransactionWriteTask;
		std::shared_ptr<gradido::blockchain::FileBased> mBlockchain;
		bool mExitCalled;
	};
}

#endif