#ifndef __GRADIDO_NODE_CONTROLLER_BLOCK_H
#define __GRADIDO_NODE_CONTROLLER_BLOCK_H

#include <map>
#include "ControllerBase.h"
#include "BlockIndex.h"

#include "../model/files/Block.h"
#include "../model/NodeTransactionEntry.h"

#include "../lib/FuzzyTimer.h"

#include "../task/WriteTransactionsToBlockTask.h"

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

	class Block : public ControllerBase, public TimerCallback
	{
		friend model::files::Block;
	public:
		Block(uint32_t blockNr, Poco::Path groupFolderPath, TaskObserver* taskObserver, const std::string& groupAlias);
		~Block();

		//! \return false if block file has errors
		bool init(Poco::SharedPtr<controller::AddressIndex> addressIndex);
		void exit();

		//! \brief put new transaction to cache and file system
		bool pushTransaction(Poco::SharedPtr<model::NodeTransactionEntry> transaction);
		
		//! \brief load transaction from cache or file system
		Poco::SharedPtr<model::TransactionEntry> getTransaction(uint64_t transactionNr);

		//! \brief called from timeout manager for scheduling WriteTransactionsToBlockTask 
		void checkTimeout(Poco::Timer& timer);

		inline Poco::SharedPtr<BlockIndex> getBlockIndex() { return mBlockIndex; }


		inline bool hasSpaceLeft() { return mBlockFile->getCurrentFileSize() + 32 < 128 * 1024 * 1024; }

		TimerReturn callFromTimer();
		const char* getResourceType() const { return "controller::Block"; };

			
	protected:
		//! \brief add transaction from Block File, called by Block File, adding to cache and index
		void addTransaction(std::unique_ptr<std::string> serializedTransaction, int32_t fileCursor);

		//Poco::Timer mTimer;
		
		uint32_t mBlockNr;		

		TaskObserver *mTaskObserver;
		Poco::AccessExpireCache<uint64_t, model::TransactionEntry> mSerializedTransactions;

		Poco::SharedPtr<BlockIndex> mBlockIndex;
		Poco::AutoPtr<model::files::Block> mBlockFile;
		Poco::AutoPtr<WriteTransactionsToBlockTask> mTransactionWriteTask;
		std::string mGroupAlias;
		bool mExitCalled;
	};
}

#endif