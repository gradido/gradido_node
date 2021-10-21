#include "Block.h"
#include "../ServerGlobals.h"

#include "TaskObserver.h"

#include "../SingletonManager/GroupManager.h"




namespace controller {
	Block::Block(uint32_t blockNr, Poco::Path groupFolderPath, TaskObserver* taskObserver, const std::string& groupHash)
		: mBlockNr(blockNr), mSerializedTransactions(ServerGlobals::g_CacheTimeout),
		  mBlockIndex(new controller::BlockIndex(groupFolderPath, blockNr)),
		  mBlockFile(new model::files::Block(groupFolderPath, blockNr)), mTaskObserver(taskObserver), mGroupHash(groupHash)
	{
		TimeoutManager::getInstance()->registerTimeout(this);
	}

	Block::~Block()
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);
		TimeoutManager::getInstance()->unregisterTimeout(this);

		/*for (auto it = mSerializedTransactions.begin(); it != mSerializedTransactions.end(); it++) {
			delete it->second;
		}*/

		mSerializedTransactions.clear();

	}

	//bool Block::pushTransaction(const std::string& serializedTransaction, uint64_t transactionNr)
	bool Block::pushTransaction(Poco::SharedPtr<model::TransactionEntry> transaction)
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);

		if (mTransactionWriteTask.isNull()) {
			mTransactionWriteTask = new WriteTransactionsToBlockTask(mBlockFile, mBlockIndex);
		}
		mTransactionWriteTask->addSerializedTransaction(transaction);
		mSerializedTransactions.add(transaction->getTransactionNr(), transaction);

		return true;

	}

	bool Block::addTransaction(const std::string& serializedTransaction, uint32_t fileCursor)
	{
		auto gm = GroupManager::getInstance();
		auto group = gm->findGroup(mGroupHash);
		try {
			Poco::SharedPtr<model::TransactionEntry> transactionEntry(new model::TransactionEntry(serializedTransaction, fileCursor, group));
			mSerializedTransactions.add(transactionEntry->getTransactionNr(), transactionEntry);
			mBlockIndex->addIndicesForTransaction(transactionEntry);
			return true;
		}
		catch (Poco::Exception& e) {
			return false;
		}

		return false;
	}

	int Block::getTransaction(uint64_t transactionNr, std::string& serializedTransaction)
	{
		if (transactionNr == 0) return -1;
		auto transactionEntry = mSerializedTransactions.get(transactionNr);
		if (transactionEntry.isNull()) {
			// read from file system

		}
		if (transactionEntry.isNull()) {
			return -1;
		}

		serializedTransaction = transactionEntry->getSerializedTransaction();
		return 0;
	}

	void Block::checkTimeout()
	{
		Poco::FastMutex::ScopedLock lock(mWorkingMutex);

		if (!mTransactionWriteTask.isNull()) {
			if (Poco::Timespan(Poco::DateTime() - mTransactionWriteTask->getCreationDate()).totalSeconds() > ServerGlobals::g_WriteToDiskTimeout) {
				mTransactionWriteTask->setFinishCommand(new TaskObserverFinishCommand(mTaskObserver));
				mTransactionWriteTask->scheduleTask(mTransactionWriteTask);
				mTaskObserver->addBlockWriteTask(mTransactionWriteTask);
				mTransactionWriteTask = nullptr;
			}
		}
	}

}
