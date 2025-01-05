#include "Block.h"
#include "../ServerGlobals.h"

#include "../blockchain/FileBasedProvider.h"
#include "../blockchain/FileBased.h"
#include "../blockchain/NodeTransactionEntry.h"
#include "../cache/Block.h"
#include "../controller/TaskObserver.h"
#include "../model/files/Block.h"
#include "../model/files/FileExceptions.h"

#include "../SingletonManager/CacheManager.h"

#include "gradido_blockchain/Application.h"
#include "gradido_blockchain/interaction/toJson/Context.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/lib/Profiler.h"

#include "loguru/loguru.hpp"

#include <chrono>
#include <thread>

using namespace gradido::blockchain;
using namespace gradido::interaction;

namespace cache {
	Block::Block(uint32_t blockNr, std::shared_ptr<const gradido::blockchain::FileBased> blockchain)
		: mBlockNr(blockNr),
		mSerializedTransactions(ServerGlobals::g_CacheTimeout),
		mBlockIndex(std::make_shared<BlockIndex>(blockchain->getFolderPath(), blockNr)),
		mBlockFile(std::make_shared<model::files::Block>(blockchain->getFolderPath(), blockNr)),
		mBlockchain(blockchain),
		mExitCalled(false)
	{
		CacheManager::getInstance()->getFuzzyTimer()->addTimer("cache::" + mBlockFile->getBlockPath(), this, ServerGlobals::g_TimeoutCheck, -1);
	}

	Block::~Block()
	{
		exit();
		//! attention, gap between mutex lock of exit and deconstruct
		std::lock_guard lock(mFastMutex);
		auto result = CacheManager::getInstance()->getFuzzyTimer()->removeTimer("controller::" + mBlockFile->getBlockPath());
		if (result != 1 && result != -1) {
			LOG_F(ERROR, "error removing timer, result: %d", result);
		}
		mSerializedTransactions.clear();
	}

	bool Block::init()
	{
		std::lock_guard lock(mFastMutex);
		if (!mBlockIndex->loadFromFile()) {
			// check if Block exist
			if (mBlockFile->getCurrentFileSize()) {
				Profiler timeUsed;
				auto rebuildBlockIndexTask = mBlockFile->rebuildBlockIndex(mBlockchain);
				if (!rebuildBlockIndexTask) {
					throw GradidoNullPointerException("missing rebuild block index task", "RebuildBlockIndexTask", __FUNCTION__);
				}
				rebuildBlockIndexTask->scheduleTask(rebuildBlockIndexTask);
				int sumWaited = 0;
				while (!rebuildBlockIndexTask->isPendingQueueEmpty() && sumWaited < 1000) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					sumWaited += 100;
				}
				if (!rebuildBlockIndexTask->isPendingQueueEmpty()) {
					LOG_F(FATAL, "rebuildBlockIndex Task isn't finished after waiting a whole second");
					Application::terminate();
				}
				auto transactionEntries = rebuildBlockIndexTask->getTransactionEntries();
				mBlockIndex->reset();
				std::for_each(transactionEntries.begin(), transactionEntries.end(),
					[&](const std::shared_ptr<NodeTransactionEntry>& transactionEntry) {
						mBlockIndex->addIndicesForTransaction(transactionEntry);
					}
				);
				LOG_F(INFO, "time for rebuilding block index for block %s: %s", mBlockFile->getBlockPath().data(), timeUsed.string().data());
			}
			else {
				return false;
			}
		}
		return true;
	}

	void Block::exit()
	{
		std::lock_guard lock(mFastMutex);
		mExitCalled = true;
		if (mTransactionWriteTask) {
			mTransactionWriteTask->run();
			mTransactionWriteTask = nullptr;
		}
	}

	//bool Block::pushTransaction(const std::string& serializedTransaction, uint64_t transactionNr)
	bool Block::pushTransaction(std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transaction)
	{
		std::lock_guard lock(mFastMutex);
		if (mExitCalled) return false;

		if (!mTransactionWriteTask) {
			// std::shared_ptr<model::files::Block> blockFile, std::shared_ptr<cache::BlockIndex> blockIndex
			mTransactionWriteTask = std::make_shared<WriteTransactionsToBlockTask>(mBlockFile, mBlockIndex);
		}
		mTransactionWriteTask->addSerializedTransaction(transaction);
		mSerializedTransactions.add(transaction->getTransactionNr(), transaction);
		return true;

	}

	void Block::addTransaction(memory::ConstBlockPtr serializedTransaction, int32_t fileCursor) const
	{
		auto transactionEntry = std::make_shared<NodeTransactionEntry>(serializedTransaction, mBlockchain, fileCursor);
		if (mExitCalled) return;
		mSerializedTransactions.add(transactionEntry->getTransactionNr(), transactionEntry);
	}

	std::shared_ptr<const gradido::blockchain::NodeTransactionEntry> Block::getTransaction(uint64_t transactionNr) const
	{
		assert(transactionNr);
		std::lock_guard lock(mFastMutex);

		auto transactionEntry = mSerializedTransactions.get(transactionNr);
		if (!transactionEntry) {
			// maybe it was already deleted from cache but not written to file yet, especially happen while debugging
			// happen also if cache timeout is shorter than file write timeout
			bool writeTransactionTaskExist = false;
			bool writeTransactionTaskIsObserved = false;
			std::shared_ptr<NodeTransactionEntry> transaction;
			if (mTransactionWriteTask) {
				transaction = mTransactionWriteTask->getTransaction(transactionNr);
				writeTransactionTaskExist = true;
				if (transaction) return transaction;
			}
			auto& taskObserver = mBlockchain->getTaskObserver();
			if (taskObserver.isTransactionPending(transactionNr)) {
				transaction = taskObserver.getTransaction(transactionNr);
				writeTransactionTaskIsObserved = true;
				if (transaction) return transaction;
			}
			// read from file system
			int32_t fileCursor = 0;
			if (!mBlockIndex->getFileCursorForTransactionNr(transactionNr, fileCursor)) {
				LOG_F(INFO, "writeTransactionTaskExist: %d, writeTransactionTaskIsObserved: %d",
					(int)writeTransactionTaskExist, 
					(int)writeTransactionTaskIsObserved
				);
				throw GradidoBlockchainTransactionNotFoundException("transaction not found in cache, in write task or file").setTransactionId(transactionNr);
			}
			if (!fileCursor && transactionNr < mBlockIndex->getMaxTransactionNr()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				if (!mBlockIndex->getFileCursorForTransactionNr(transactionNr, fileCursor)) {
					throw GradidoBlockchainTransactionNotFoundException("transaction not found in cache, in write task or file after sleep").setTransactionId(transactionNr);
				}
			}
			try {
				auto blockLine = mBlockFile->readLine(fileCursor);
				addTransaction(blockLine, fileCursor);
			}
			catch (model::files::EndReachingException& ex) {
				LOG_F(ERROR, ex.getFullString().data());
				throw GradidoBlockchainTransactionNotFoundException("transaction not found in file").setTransactionId(transactionNr);
			}

			transactionEntry = mSerializedTransactions.get(transactionNr);
			if (!transactionEntry) {
				LOG_F(ERROR, "fileCursor: %d", fileCursor);
				auto blockLine = mBlockFile->readLine(fileCursor);
				deserialize::Context deserializer(blockLine, deserialize::Type::CONFIRMED_TRANSACTION);
				deserializer.run();
				if (deserializer.isConfirmedTransaction()) {
					toJson::Context jsonSerializer(*deserializer.getConfirmedTransaction());
					LOG_F(ERROR, "block: %s", jsonSerializer.run(true).data());
				}
				else {
					LOG_F(ERROR, "block line isn't valid confirmed transaction");
				}
				throw GradidoBlockchainTransactionNotFoundException("transaction not found after reading from block file")
					.setTransactionId(transactionNr);
			}
		}
		if(!transactionEntry) {
			throw GradidoBlockchainTransactionNotFoundException("transaction is still null, after all checks")
				.setTransactionId(transactionNr);
		}
		return transactionEntry.value();
	}

	bool Block::hasSpaceLeft() {
		return mBlockFile->getCurrentFileSize() + 32 < GRADIDO_NODE_CACHE_BLOCK_MAX_FILE_SIZE_BYTE;
	}

	TimerReturn Block::callFromTimer()
	{
		std::unique_lock lock(mFastMutex, std::defer_lock);
		// if called from timer, while deconstruct was called, prevent dead lock
		if (!lock.try_lock()) return TimerReturn::GO_ON;
		if (mExitCalled) {
			return TimerReturn::REMOVE_ME;
		}

		if (mTransactionWriteTask) {
			Timepoint now;
			if (now - mTransactionWriteTask->getCreationDate() > ServerGlobals::g_WriteToDiskTimeout) 
			{
				auto copyTask = mTransactionWriteTask;
				mBlockchain->getTaskObserver().addBlockWriteTask(copyTask);
				mTransactionWriteTask = nullptr;

				copyTask->setFinishCommand(new TaskObserverFinishCommand(mBlockchain));
				copyTask->scheduleTask(copyTask);
			}
		}
		return TimerReturn::GO_ON;
	}

}
