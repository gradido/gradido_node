#include "Block.h"
#include "../ServerGlobals.h"

#include "TaskObserver.h"

#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/CacheManager.h"

namespace controller {
	Block::Block(uint32_t blockNr, Poco::Path groupFolderPath, TaskObserver* taskObserver, const std::string& groupAlias)
		: //mTimer(0, ServerGlobals::g_TimeoutCheck),
		  mBlockNr(blockNr), mSerializedTransactions(ServerGlobals::g_CacheTimeout),
		  mBlockIndex(new controller::BlockIndex(groupFolderPath, blockNr)),
		  mBlockFile(new model::files::Block(groupFolderPath, blockNr)), mTaskObserver(taskObserver), mGroupAlias(groupAlias),
   		  mExitCalled(false)
	{
		//Poco::TimerCallback<Block> callback(*this, &Block::checkTimeout);
		//mTimer.start(callback);
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		CacheManager::getInstance()->getFuzzyTimer()->addTimer("controller::" + mBlockFile->getBlockPath(), this, ServerGlobals::g_TimeoutCheck, -1);
		mBlockIndex->loadFromFile();
	}

	Block::~Block()
	{
		//printf("[controller::~Block]\n");
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		if (CacheManager::getInstance()->getFuzzyTimer()->removeTimer("controller::" + mBlockFile->getBlockPath()) != 1) {
			printf("[controller::~Block]] error removing timer\n");
		}
		// deadlock, because it is triggered from expire cache?
		//mTimer.stop();
		//printf("after timer stop\n");
		//checkTimeout(mTimer);
		//printf("after call checkTimeout\n");
		mSerializedTransactions.clear();		
	}

	void Block::exit()
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		mExitCalled = true;
		if (!mTransactionWriteTask.isNull()) {
			mTransactionWriteTask->run();
			mTransactionWriteTask = nullptr;
		}
	}

	//bool Block::pushTransaction(const std::string& serializedTransaction, uint64_t transactionNr)
	bool Block::pushTransaction(Poco::SharedPtr<model::TransactionEntry> transaction)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		if (mExitCalled) return false;
	
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
			Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
			mSerializedTransactions.add(transactionEntry->getTransactionNr(), transactionEntry);
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
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);

		auto transactionEntry = mSerializedTransactions.get(transactionNr);
		if (transactionEntry.isNull()) {
			// read from file system
			int32_t fileCursor = 0;
			if (!mBlockIndex->getFileCursorForTransactionNr(transactionNr, fileCursor)) {
				// maybe it was already deleted from cache but not written to file yet, especially happen while debugging
				if (!mTransactionWriteTask.isNull()) {
					transactionEntry = mTransactionWriteTask->getTransaction(transactionNr);
					if (!transactionEntry.isNull()) {
						serializedTransaction = transactionEntry->getSerializedTransaction();
						return 0;
					}
				}
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
		
		if (mExitCalled) {
			timer.restart(0);
			return;
		}

		if (!mTransactionWriteTask.isNull()) {
			if (Poco::Timespan(Poco::DateTime() - mTransactionWriteTask->getCreationDate()).totalSeconds() > ServerGlobals::g_WriteToDiskTimeout) {
				mTransactionWriteTask->setFinishCommand(new TaskObserverFinishCommand(mTaskObserver));
				mTransactionWriteTask->scheduleTask(mTransactionWriteTask);
				mTaskObserver->addBlockWriteTask(mTransactionWriteTask);
				mTransactionWriteTask = nullptr;
			}
		}
	}

	UniLib::lib::TimerReturn Block::callFromTimer()
	{
		// if called from timer, while deconstruct was called, prevent dead lock
		if (!mWorkingMutex.tryLock()) UniLib::lib::GO_ON;
		if (mExitCalled) {
			mWorkingMutex.unlock();
			return UniLib::lib::REMOVE_ME;
		}
		//Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);

		if (!mTransactionWriteTask.isNull()) {
			if (Poco::Timespan(Poco::DateTime() - mTransactionWriteTask->getCreationDate()).totalSeconds() > ServerGlobals::g_WriteToDiskTimeout) {
				mTransactionWriteTask->setFinishCommand(new TaskObserverFinishCommand(mTaskObserver));
				mTransactionWriteTask->scheduleTask(mTransactionWriteTask);
				mTaskObserver->addBlockWriteTask(mTransactionWriteTask);
				mTransactionWriteTask = nullptr;
			}
		}
		mWorkingMutex.unlock();
		return UniLib::lib::GO_ON;
	}

}
