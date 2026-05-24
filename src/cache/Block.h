#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_H

#include "BlockIndex.h"

#include "../controller/TaskObserver.h"
#include "../lib/FuzzyTimer.h"
#include "../task/WriteTransactionsToBlockTask.h"

#include "gradido_blockchain/lib/AccessExpireCache.h"
#include "gradido_blockchain/lib/DictionaryInterface.h"
#include "gradido_blockchain/blockchain/TransactionEntry.h"

#include <map>
#include <memory>
#include <stop_token>

namespace model {
	namespace files {
		class Block;
	}
}

namespace gradido {
	class AppContext;
	namespace blockchain {
		class FileBased;
		class NodeTransactionEntry;
	}
	namespace data::compact {
		struct ConfirmedGradidoTx;
	}
}

namespace memory {
	class Block;
	using ConstBlockPtr = std::shared_ptr<const Block>;
}

// TODO: move into config
#define GRADIDO_NODE_CACHE_BLOCK_MAX_FILE_SIZE_BYTE 128 * 1024 * 1024
#define GRADIDO_NODE_CACHE_BLOCK_MAX_WAIT_TIME_FOR_BLOCK_INDEX_REBUILD_MILLISECONDS 1000 * 60

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
		Block(uint32_t blockNr, std::shared_ptr<const gradido::blockchain::FileBased> blockchain);
		~Block();

		//! \return false if block not exist
		bool init(std::stop_token stop = std::stop_token());
		void exit();

		//! \brief put new transaction to cache and file system
		bool pushTransaction(
			std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transaction,
			IMutableDictionary<gradido::data::PublicKey>& publicKeyDictionary,
			gradido::AppContext& appContext
		);
		
		//! \brief load transaction from cache or file system
		std::shared_ptr<const gradido::blockchain::NodeTransactionEntry> getTransaction(
			uint64_t transactionNr,
			gradido::AppContext& appContext
		) const;

		std::shared_ptr<gradido::data::compact::ConfirmedGradidoTx> getCompactTransaction(
			uint64_t transactionNr,
			gradido::AppContext& appContext
		) const;

		inline BlockIndex& getBlockIndex() { return *mBlockIndex; }
		inline const BlockIndex& getBlockIndex() const { return *mBlockIndex; }

		bool hasSpaceLeft();

		//! \brief called from timeout manager for scheduling WriteTransactionsToBlockTask 
		TimerReturn callFromTimer();
		const char* getResourceType() const { return "controller::Block"; };
		
	protected:
		//! \brief add transaction from Block File, called by Block File, adding to cache and index
		//! not locking mutex!
		void addTransaction(
			memory::ConstBlockPtr serializedTransaction, 
			int32_t fileCursor,
			gradido::AppContext& appContext
		) const;
		std::shared_ptr<gradido::data::compact::ConfirmedGradidoTx> addCompactTransaction(std::shared_ptr<const gradido::blockchain::NodeTransactionEntry> transactionEntry, gradido::AppContext& appContext) const;
		
		mutable std::mutex mFastMutex;
		uint32_t mBlockNr;		

		mutable AccessExpireCache<uint64_t, std::shared_ptr<gradido::blockchain::NodeTransactionEntry>> mSerializedTransactions;
		mutable AccessExpireCache<uint64_t, std::shared_ptr<gradido::data::compact::ConfirmedGradidoTx>> mConfirmedTxByNr;

		std::shared_ptr<BlockIndex> mBlockIndex;
		std::shared_ptr<model::files::Block> mBlockFile;
		std::shared_ptr<task::WriteTransactionsToBlockTask> mTransactionWriteTask;
		std::shared_ptr<const gradido::blockchain::FileBased> mBlockchain;
		bool mExitCalled;
	};
}

#endif