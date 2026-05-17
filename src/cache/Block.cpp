#include "Block.h"
#include "../ServerGlobals.h"

#include "../blockchain/FileBasedProvider.h"
#include "../blockchain/FileBased.h"
#include "../blockchain/NodeTransactionEntry.h"
#include "../cache/Block.h"
#include "../controller/TaskObserver.h"
#include "../model/files/Block.h"
#include "../model/files/FileExceptions.h"
#include "../task/RebuildBlockIndexTask.h"

#include "../SingletonManager/CacheManager.h"

#include "gradido_blockchain_core/memory.h"
#include "gradido_blockchain_core/data/wire/confirmed_transaction.h"
#include "gradido_blockchain_core/data/wire/transaction_body.h"
#include "gradido_blockchain/Application.h"
#include "gradido_blockchain/AppContext.h"
#include "gradido_blockchain/data/compact/ConfirmedGradidoTx.h"
#include "gradido_blockchain/data/TransactionType.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/serialization/toJsonString.h"
#include "gradido_blockchain/lib/MonotonicTimer.h"

#include "loguru/loguru.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

using gradido::AppContext;
using namespace gradido::blockchain;
using gradido::data::TransactionType, gradido::data::compact::ConfirmedGradidoTx;
using namespace gradido::interaction;
using std::shared_ptr, std::make_shared, std::lock_guard;
using task::RebuildBlockIndexTask;


namespace cache {
	Block::Block(uint32_t blockNr, std::shared_ptr<const gradido::blockchain::FileBased> blockchain)
		: mBlockNr(blockNr),
		mSerializedTransactions(ServerGlobals::g_CacheTimeout),
		mConfirmedTxByNr(ServerGlobals::g_CacheTimeout),
		mBlockIndex(std::make_shared<BlockIndex>(blockchain->getFolderPath(), blockNr, blockchain->getCommunityIdIndex())),
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

	bool Block::init(std::stop_token stop /* = std::stop_token() */)
	{
		lock_guard lock(mFastMutex);
		// todo: add data for address index in file, until then rebuild block index on each program start
		// if (!mBlockIndex->loadFromFile(publicKeyDictionary))
		{
			// check if Block exist
			if (mBlockFile->getCurrentFileSize())
			{
				MonotonicTimer timeUsed;
				mBlockIndex->reset();
				auto rebuildBlockIndexTask = make_shared<task::RebuildBlockIndexTask>(
					mBlockIndex,
					mBlockchain->getCommunityIdIndex()
				);
				rebuildBlockIndexTask->scheduleTask(rebuildBlockIndexTask);
				mBlockFile->readBuffered(rebuildBlockIndexTask->getAlloc(), rebuildBlockIndexTask.get(), stop);

				int sumWaited = 0;
				while (!rebuildBlockIndexTask->isTaskFinished() && sumWaited < GRADIDO_NODE_CACHE_BLOCK_MAX_WAIT_TIME_FOR_BLOCK_INDEX_REBUILD_MILLISECONDS) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					sumWaited += 100;
				}
				if (!rebuildBlockIndexTask->isTaskFinished()) {
					LOG_F(FATAL, "rebuildBlockIndex Task isn't finished after waiting a whole minute");
					Application::terminate();
				}
				LOG_F(INFO, "rebuilding block index for block %s in %s", mBlockFile->getBlockPath().data(), timeUsed.string().data());
				mBlockIndex->writeIntoFile();
			}
			else {
				return false;
			}
		}
		/*else {
			// hot fix: init address index
			// if block index was loaded from file, we load all transactions which change something in address index
			// TODO: persistent storage for address index
		}
		*/
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
	bool Block::pushTransaction(
		std::shared_ptr<gradido::blockchain::NodeTransactionEntry> transaction,
		IMutableDictionary<PublicKey>& publicKeyDictionary,
		AppContext& appContext
	)
	{
		lock_guard lock(mFastMutex);
		if (mExitCalled) return false;

		if (!mTransactionWriteTask) {
			// std::shared_ptr<model::files::Block> blockFile, std::shared_ptr<cache::BlockIndex> blockIndex
			mTransactionWriteTask = std::make_shared<task::WriteTransactionsToBlockTask>(mBlockFile, mBlockIndex);
		}
		mTransactionWriteTask->addSerializedTransaction(transaction, publicKeyDictionary);
		mSerializedTransactions.add(transaction->getTransactionNr(), transaction);
		addCompactTransaction(transaction, appContext);
		return true;

	}

	void Block::addTransaction(
		memory::ConstBlockPtr serializedTransaction,
		int32_t fileCursor,
		AppContext& appContext
	) const
	{
		auto transactionEntry = make_shared<NodeTransactionEntry>(serializedTransaction, mBlockchain, fileCursor);
		if (mExitCalled) return;
		mSerializedTransactions.add(transactionEntry->getTransactionNr(), transactionEntry);
		addCompactTransaction(transactionEntry, appContext);
		// mBlockIndex->updateAddressIndex(transactionEntry, publicKeyDictionary);
	}

	std::shared_ptr<gradido::data::compact::ConfirmedGradidoTx> Block::addCompactTransaction(shared_ptr<const NodeTransactionEntry> transactionEntry, AppContext& appContext) const
	{
		// create compact version
		try {
			uint8_t buffer[1024];
			grd_memory alloc;
			grd_memory_init_arena_static(&alloc, buffer, 1024);
			grdw_confirmed_transaction tx{};
			auto communityIdIndex = mBlockchain->getCommunityIdIndex();
			transactionEntry->getConfirmedTransaction()->toGrdw(&alloc, &tx, communityIdIndex);
			auto confirmedTxPtr = make_shared<ConfirmedGradidoTx>(ConfirmedGradidoTx::fromGrdw(&tx, communityIdIndex, appContext));
			alloc.last_index = 0;
			grdw_transaction_body txBody{};
			transactionEntry->getTransactionBody()->toGrdw(&alloc, &txBody);
			confirmedTxPtr->fillFromGrdwTransactionBody(&txBody, appContext);
			mConfirmedTxByNr.add(confirmedTxPtr->txNr, confirmedTxPtr);
			return confirmedTxPtr;
		}
		catch (GradidoBlockchainException& ex) {
			LOG_F(WARNING, "%s on create compact", ex.getFullString().c_str());
		}
		return nullptr;
	}

	shared_ptr<const gradido::blockchain::NodeTransactionEntry> Block::getTransaction(uint64_t transactionNr, AppContext& appContext) const
	{
		assert(transactionNr);
		lock_guard lock(mFastMutex);

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
				addTransaction(blockLine, fileCursor, appContext);
			}
			catch (model::files::EndReachingException& ex) {
				LOG_F(ERROR, "%s", ex.getFullString().data());
				throw GradidoBlockchainTransactionNotFoundException("transaction not found in file").setTransactionId(transactionNr);
			}

			transactionEntry = mSerializedTransactions.get(transactionNr);
			if (!transactionEntry) {
				LOG_F(ERROR, "fileCursor: %d", fileCursor);
				auto blockLine = mBlockFile->readLine(fileCursor);
				deserialize::Context deserializer(blockLine, deserialize::Type::CONFIRMED_TRANSACTION);
				deserializer.run(mBlockchain->getCommunityIdIndex());
				if (deserializer.isConfirmedTransaction()) {
					LOG_F(ERROR, "block: %s", serialization::toJsonString(*deserializer.getConfirmedTransaction(), true).data());
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

	std::shared_ptr<gradido::data::compact::ConfirmedGradidoTx> Block::getCompactTransaction(
		uint64_t transactionNr,
		gradido::AppContext& appContext
	) const
	{
		auto confirmedTx = mConfirmedTxByNr.get(transactionNr);
		if (!confirmedTx) {
			// check write cache, else try to read from storage
			// return always a valid ptr or throw exception
			auto transactionEntry = getTransaction(transactionNr, appContext);

			// we use two different access expire caches, after this call succeed it is sure, that the transaction is in Serialized Transactions,
			// but it can be still missing in mConfirmedTxByNr
			confirmedTx = mConfirmedTxByNr.get(transactionNr);
			if (!confirmedTx) {
				return addCompactTransaction(transactionEntry, appContext);
			}
		}
		// should only don't work, if getTransaction failed, but this will throw an exception anyway
		return confirmedTx.value();
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
			Timepoint now = std::chrono::system_clock::now();
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
