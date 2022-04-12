#include "Block.h"
#include "../ServerGlobals.h"

#include "TaskObserver.h"
#include "../model/files/FileExceptions.h"

#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/CacheManager.h"

#include "Poco/Util/ServerApplication.h"

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
		CacheManager::getInstance()->getFuzzyTimer()->addTimer("controller::" + mBlockFile->getBlockPath(), this, ServerGlobals::g_TimeoutCheck, -1);
	}

	Block::~Block()
	{
		exit();
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

	bool Block::init(Poco::SharedPtr<controller::AddressIndex> addressIndex)
	{
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		if (!mBlockIndex->loadFromFile()) {
			// check if Block exist
			if (mBlockFile->getCurrentFileSize()) {
				auto rebuildBlockIndexTask = mBlockFile->rebuildBlockIndex(addressIndex);
				if (rebuildBlockIndexTask.isNull()) {
					return false;
				}
				int sumWaited = 0;
				while (!rebuildBlockIndexTask->isPendingQueueEmpty() && sumWaited < 1000) {
					Poco::Thread::sleep(100);
					sumWaited += 100;
				}
				if (!rebuildBlockIndexTask->isPendingQueueEmpty()) {
					LoggerManager::getInstance()->mErrorLogging.critical("[controller::Block::Block] rebuildBlockIndex Task isn't finished after waiting a whole second");
					Poco::Util::ServerApplication::terminate();
				}
				auto transactionEntries = rebuildBlockIndexTask->getTransactionEntries();
				mBlockIndex->reset();
				std::for_each(transactionEntries.begin(), transactionEntries.end(),
					[&](const Poco::SharedPtr<model::NodeTransactionEntry>& transactionEntry)
					{
						mBlockIndex->addIndicesForTransaction(transactionEntry);
					}
				);
			}
		}
		return true;
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
	bool Block::pushTransaction(Poco::SharedPtr<model::NodeTransactionEntry> transaction)
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

	void Block::addTransaction(std::unique_ptr<std::string> serializedTransaction, int32_t fileCursor)
	{
		auto gm = GroupManager::getInstance();
		auto group = gm->findGroup(mGroupAlias);
		auto transaction = std::make_unique<model::gradido::GradidoBlock>(serializedTransaction.get());
		Poco::SharedPtr<model::NodeTransactionEntry> transactionEntry(new model::NodeTransactionEntry(transaction.get(), group->getAddressIndex(), fileCursor));
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);
		mSerializedTransactions.add(transactionEntry->getTransactionNr(), transactionEntry);
	}

	Poco::SharedPtr<model::NodeTransactionEntry> Block::getTransaction(uint64_t transactionNr)
	{
		assert(transactionNr);
		Poco::ScopedLock<Poco::Mutex> lock(mWorkingMutex);

		auto transactionEntry = mSerializedTransactions.get(transactionNr);
		if (transactionEntry.isNull()) {
			// read from file system
			int32_t fileCursor = 0;
			if (!mBlockIndex->getFileCursorForTransactionNr(transactionNr, fileCursor)) {
				// maybe it was already deleted from cache but not written to file yet, especially happen while debugging
				if (!mTransactionWriteTask.isNull()) {
					return mTransactionWriteTask->getTransaction(transactionNr);
				}
				throw GradidoBlockchainTransactionNotFoundException("transaction not found in cache, in write task or file").setTransactionId(transactionNr);
			}
			try {
				auto blockLine = mBlockFile->readLine(fileCursor);
				addTransaction(std::move(blockLine), fileCursor);
			}
			catch (model::files::EndReachingException& ex) {
				LoggerManager::getInstance()->mErrorLogging.error("%s: %s", std::string(__FUNCTION__), ex.getFullString());
				throw GradidoBlockchainTransactionNotFoundException("transaction not found in file").setTransactionId(transactionNr);
			}
			catch (Poco::NullPointerException& ex) {
				printf("[Block::getTransaction] catch Poco Null Pointer Exception\n");
				throw;
			}
			return mSerializedTransactions.get(transactionNr);
		}
		else {
			return transactionEntry;
		}
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

	TimerReturn Block::callFromTimer()
	{
		// if called from timer, while deconstruct was called, prevent dead lock
		if (!mWorkingMutex.tryLock()) GO_ON;
		if (mExitCalled) {
			mWorkingMutex.unlock();
			return REMOVE_ME;
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
		return GO_ON;
	}

}
