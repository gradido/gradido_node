#include "Block.h"
#include "../ServerGlobals.h"

#include "TaskObserver.h"

#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/LoggerManager.h"

namespace controller {
	Block::Block(uint32_t blockNr, Poco::Path groupFolderPath, TaskObserver* taskObserver, const std::string& groupAlias)
		: mTimer(0, ServerGlobals::g_TimeoutCheck),
		  mBlockNr(blockNr), mSerializedTransactions(ServerGlobals::g_CacheTimeout),
		  mBlockIndex(new controller::BlockIndex(groupFolderPath, blockNr)),
		  mBlockFile(new model::files::Block(groupFolderPath, blockNr)), mTaskObserver(taskObserver), mGroupAlias(groupAlias)
	{
		Poco::TimerCallback<Block> callback(*this, &Block::checkTimeout);
		mTimer.start(callback);

		mBlockIndex->loadFromFile();
	}

	Block::~Block()
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		mTimer.stop();
		checkTimeout(mTimer);
		mSerializedTransactions.clear();		
	}

	//bool Block::pushTransaction(const std::string& serializedTransaction, uint64_t transactionNr)
	bool Block::pushTransaction(Poco::SharedPtr<model::TransactionEntry> transaction)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
	
		if (mTransactionWriteTask.isNull()) {
			mTransactionWriteTask = new WriteTransactionsToBlockTask(mBlockFile, mBlockIndex);
		}
		mTransactionWriteTask->addSerializedTransaction(transaction);	
		mSerializedTransactions.add(transaction->getTransactionNr(), transaction);
		return true;

	}

	bool Block::addTransaction(const std::string& serializedTransaction, int32_t fileCursor)
	{
		auto gm = GroupManager::getInstance();
		auto group = gm->findGroup(mGroupAlias);
		try {
			Poco::SharedPtr<model::TransactionEntry> transactionEntry(new model::TransactionEntry(serializedTransaction, fileCursor, group));
			mSerializedTransactions.add(transactionEntry->getTransactionNr(), transactionEntry);
			mBlockIndex->addIndicesForTransaction(transactionEntry);
			return true;
		}
		catch (Poco::Exception& e) {
			LoggerManager::getInstance()->mErrorLogging.error("[Block::addTransactio] couldn't load Transaction from serialized data, fileCursor: %d", fileCursor);
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
			int32_t fileCursor = 0;
			if (!mBlockIndex->getFileCursorForTransactionNr(transactionNr, fileCursor)) {
				return -2;
			}
			std::string blockLine;
			auto readResult = mBlockFile->readLine(fileCursor, blockLine);
			if (readResult) {
				LoggerManager::getInstance()->mErrorLogging.error("[Block::getTransaction] error reading line from block file: %d, fileCursor: %d", readResult, fileCursor);
				return -3;
			}
			if (!addTransaction(blockLine, fileCursor)) {
				return -4;
			}
			transactionEntry = mSerializedTransactions.get(transactionNr);
		}
		if (transactionEntry.isNull()) {
			return -1;
		}

		serializedTransaction = transactionEntry->getSerializedTransaction();
		return 0;
	}

	void Block::checkTimeout(Poco::Timer& timer)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);

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
