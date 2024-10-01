#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_H

#include "BlockIndex.h"

#include "../controller/TaskObserver.h"
#include "../model/files/Block.h"
#include "../blockchain/NodeTransactionEntry.h"

#include "../lib/FuzzyTimer.h"

#include "../task/WriteTransactionsToBlockTask.h"

#include "gradido_blockchain/lib/AccessExpireCache.h"

#include <map>


namespace cache {
	class Group;

	/*! 
	 * @author Dario Rekowski
	 * @date 2020-02-06
	 * @brief interface for adding and getting transactions from specific block
	 */

	class Block : public TimerCallback
	{
		friend model::files::Block;
	public:
		Block(uint32_t blockNr, std::string_view groupFolderPath, TaskObserver& taskObserver, const std::string& groupAlias);
		~Block();

		//! \return false if block file has errors
		bool init(const cache::Dictionary publicKeyIndex);
		void exit();
		void reset();

		//! \brief put new transaction to cache and file system
		bool pushTransaction(std::shared_ptr<model::NodeTransactionEntry> transaction);
		
		//! \brief load transaction from cache or file system
		std::shared_ptr<gradido::blockchain::TransactionEntry> getTransaction(uint64_t transactionNr);

		inline const BlockIndex& getBlockIndex() { return mBlockIndex; }

		inline bool hasSpaceLeft() { return mBlockFile->getCurrentFileSize() + 32 < 128 * 1024 * 1024; }

		//! \brief called from timeout manager for scheduling WriteTransactionsToBlockTask 
		TimerReturn callFromTimer();
		const char* getResourceType() const { return "controller::Block"; };
			
	protected:
		//! \brief add transaction from Block File, called by Block File, adding to cache and index
		void addTransaction(std::unique_ptr<std::string> serializedTransaction, int32_t fileCursor);
		
		std::mutex mFastMutex;
		uint32_t mBlockNr;		

		// need to stay in group or blockchain, because Block can be removed from cache, while write task is still pending
		// and reloaded again before write task was finished
		TaskObserver& mTaskObserver;
		AccessExpireCache<uint64_t, gradido::blockchain::TransactionEntry> mSerializedTransactions;

		BlockIndex mBlockIndex;
		std::shared_ptr<model::files::Block> mBlockFile;
		std::shared_ptr<WriteTransactionsToBlockTask> mTransactionWriteTask;
		std::string mGroupAlias;
		bool mExitCalled;
	};
}

#endif