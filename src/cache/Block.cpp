#include "Block.h"
#include "../ServerGlobals.h"

#include "../blockchain/FileBasedProvider.h"
#include "../controller/TaskObserver.h"
#include "../model/files/FileExceptions.h"

#include "../SingletonManager/CacheManager.h"

using namespace gradido::blockchain;

namespace cache {
	Block::Block(uint32_t blockNr, std::string_view groupFolderPath, TaskObserver& taskObserver, const std::string& groupAlias)
		: //mTimer(0, ServerGlobals::g_TimeoutCheck),
		  mBlockNr(blockNr),
		  mSerializedTransactions(ServerGlobals::g_CacheTimeout),
		  mBlockIndex(groupFolderPath, blockNr),
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
		std::lock_guard lock(mFastMutex);
		auto result = CacheManager::getInstance()->getFuzzyTimer()->removeTimer("controller::" + mBlockFile->getBlockPath());
		if (result != 1 && result != -1) {
			LOG_ERROR("[controller::~Block] error removing timer");
		}
		// deadlock, because it is triggered from expire cache?
		//mTimer.stop();
		//printf("after timer stop\n");
		//checkTimeout(mTimer);
		//printf("after call checkTimeout\n");
		mSerializedTransactions.clear();
	}

	bool Block::init(const cache::Dictionary publicKeyIndex)
	{
		std::lock_guard lock(mFastMutex);
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
					[&](const std::shared_ptr<NodeTransactionEntry>& transactionEntry)
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
		std::lock_guard lock(mFastMutex);
		mExitCalled = true;
		if (!mTransactionWriteTask.isNull()) {
			mTransactionWriteTask->run();
			mTransactionWriteTask = nullptr;
		}
	}

	void Block::reset()
	{

	}

	//bool Block::pushTransaction(const std::string& serializedTransaction, uint64_t transactionNr)
	bool Block::pushTransaction(std::shared_ptr<NodeTransactionEntry> transaction)
	{
		std::lock_guard lock(mFastMutex);
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
		auto transaction = std::make_unique<gradido::data::ConfirmedTransaction>(serializedTransaction.get());
		auto transactionEntry = std::make_shared<NodeTransactionEntry>(transaction.get(), group->getAddressIndex(), fileCursor);
		std::lock_guard lock(mFastMutex);
		if (mExitCalled) return;
		mSerializedTransactions.add(transactionEntry->getTransactionNr(), transactionEntry);
	}

	std::shared_ptr<gradido::blockchain::TransactionEntry> Block::getTransaction(uint64_t transactionNr)
	{
		assert(transactionNr);
		std::lock_guard lock(mFastMutex);

		auto transactionEntry = mSerializedTransactions.get(transactionNr);
		if (transactionEntry.isNull()) {
			// maybe it was already deleted from cache but not written to file yet, especially happen while debugging
			// happen also if cache timeout is shorter than file write timeout
			bool writeTransactionTaskExist = false;
			bool writeTransactionTaskIsObserved = false;
			std::shared_ptr<model::NodeTransactionEntry> transaction;
			if (!mTransactionWriteTask.isNull()) {
				transaction = mTransactionWriteTask->getTransaction(transactionNr);
				writeTransactionTaskExist = true;
				if (!transaction.isNull()) return transaction;
			}
			if (mTaskObserver.isTransactionPending(transactionNr)) {
				transaction = mTaskObserver.getTransaction(transactionNr);
				writeTransactionTaskIsObserved = true;
				if (!transaction.isNull()) return transaction;
			}
			// read from file system
			int32_t fileCursor = 0;
			if (!mBlockIndex->getFileCursorForTransactionNr(transactionNr, fileCursor)) {
				std::clog << "writeTransactionTaskExist: " << writeTransactionTaskExist
						  << ", writeTransactionTaskIsObserved: " << writeTransactionTaskIsObserved
						  << std::endl;
				throw GradidoBlockchainTransactionNotFoundException("transaction not found in cache, in write task or file").setTransactionId(transactionNr);
			}
			if (!fileCursor && transactionNr < mBlockIndex->getMaxTransactionNr()) {
				Poco::Thread::sleep(50);
				if (!mBlockIndex->getFileCursorForTransactionNr(transactionNr, fileCursor)) {
					throw GradidoBlockchainTransactionNotFoundException("transaction not found in cache, in write task or file after sleep").setTransactionId(transactionNr);
				}
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
			transactionEntry = mSerializedTransactions.get(transactionNr);
			if (transactionEntry.isNull()) {
				printf("fileCursor: %d\n", fileCursor);
				auto blockLine = mBlockFile->readLine(fileCursor);
				auto block = std::make_unique<gradido::data::ConfirmedTransaction>(blockLine.get());
				printf("block: %s\n", block->toJson().data());
				throw GradidoBlockchainTransactionNotFoundException("transaction not found after reading from block file")
					.setTransactionId(transactionNr);
			}
		}
		if(transactionEntry.isNull()) {
			throw GradidoBlockchainTransactionNotFoundException("transaction is still null, after all checks")
				.setTransactionId(transactionNr);
		}
		return transactionEntry;
	}

	TimerReturn Block::callFromTimer()
	{
		// if called from timer, while deconstruct was called, prevent dead lock
		if (!mFastMutex.try_lock()) return TimerReturn::GO_ON;
		if (mExitCalled) {
			mFastMutex.unlock();
			return TimerReturn::REMOVE_ME;
		}
		std::lock_guard lock(mFastMutex);

		if (!mTransactionWriteTask.isNull()) {
			Timepoint now;
			if (now - mTransactionWriteTask->getCreationDate() > ServerGlobals::g_WriteToDiskTimeout) 
			{
				auto copyTask = mTransactionWriteTask;
				mTaskObserver.addBlockWriteTask(copyTask);
				mTransactionWriteTask = nullptr;

				copyTask->setFinishCommand(new TaskObserverFinishCommand(&mTaskObserver));
				copyTask->scheduleTask(copyTask);
			}
		}
		return TimerReturn::GO_ON;
	}

}