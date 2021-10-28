#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_H

#include <map>
#include "ControllerBase.h"
#include "BlockIndex.h"

#include "../model/files/Block.h"
#include "../model/TransactionEntry.h"

#include "../task/WriteTransactionsToBlockTask.h"

#include "../SingletonManager/TimeoutManager.h"

#include "Poco/AccessExpireCache.h"

//#include "../SingletonManager/FileLockManager.h"
class TaskObserver;


namespace controller {
	class Group;

	/*! 
	 * @author Dario Rekowski
	 * @date 2020-02-06
	 * @brief interface for adding and getting transactions from specific block
	 */

	class Block : public ControllerBase, public ITimeout
	{
		friend model::files::Block;
	public:
		Block(uint32_t blockNr, Poco::Path groupFolderPath, TaskObserver* taskObserver, const std::string& groupAlias);
		~Block();

		//! \brief put new transaction to cache and file system
		bool pushTransaction(Poco::SharedPtr<model::TransactionEntry> transaction);
		
		//! \brief load transaction from cache or file system
		int getTransaction(uint64_t transactionNr, std::string& serializedTransaction);

		//! \brief called from timeout manager for scheduling WriteTransactionsToBlockTask 
		void checkTimeout();

		inline Poco::SharedPtr<BlockIndex> getBlockIndex() { return mBlockIndex; }


		inline bool hasSpaceLeft() { return mBlockFile->getCurrentFileSize() + 32 < 128 * 1024 * 1024; }

			
	protected:
		//! \brief add transaction from Block File, called by Block File, adding to cache and index
		bool addTransaction(const std::string& serializedTransaction, int32_t fileCursor);
		
		uint32_t mBlockNr;		

		TaskObserver *mTaskObserver;
		Poco::AccessExpireCache<uint64_t, model::TransactionEntry> mSerializedTransactions;

		Poco::SharedPtr<BlockIndex> mBlockIndex;
		Poco::AutoPtr<model::files::Block> mBlockFile;
		Poco::AutoPtr<WriteTransactionsToBlockTask> mTransactionWriteTask;
		std::string mGroupAlias;
	};
}

#endif