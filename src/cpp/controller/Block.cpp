#include "Block.h"
#include "../ServerGlobals.h"

#include "TaskObserver.h"


namespace controller {
	Block::Block(uint32_t blockNr, uint64_t firstTransactionIndex, Poco::Path groupFolderPath, TaskObserver* taskObserver)
		: mBlockNr(blockNr), mFirstTransactionIndex(firstTransactionIndex), mSerializedTransactions(ServerGlobals::g_CacheTimeout),
		  mBlockIndex(new controller::BlockIndex(groupFolderPath, blockNr)),
		  mBlockFile(new model::files::Block(groupFolderPath, blockNr)), mTaskObserver(taskObserver)
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