#include "FileBased.h"
#include "FileBasedProvider.h"
#include "NodeTransactionEntry.h"
#include "../cache/Exceptions.h"
#include "../ServerGlobals.h"
#include "../model/files/Block.h"
#include "../model/files/BlockIndex.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/const.h"
#include "gradido_blockchain/ServerApplication.h"

#include "loguru/loguru.hpp"

#include <set>

using namespace cache;

namespace gradido {
	namespace blockchain {
		FileBased::FileBased(Private, std::string_view communityId, std::string_view folder)
			: Abstract(communityId),
			mExitCalled(false),
			mFolderPath(folder),
			mTaskObserver(std::make_shared<TaskObserver>()),
			mIotaMessageListener(new iota::MessageListener(communityId)),
			mPublicKeysIndex(std::make_shared<Dictionary>(std::string(folder).append("/pubkeys.index"))),
			mBlockchainState(std::string(folder).append(".state")),
			mDeferredTransfersCache(std::string(folder).append("/deferredTransferCache")),
			mCachedBlocks(ServerGlobals::g_CacheTimeout)
		{
		}

		FileBased::~FileBased()
		{
			if (mIotaMessageListener) {
				delete mIotaMessageListener;
				mIotaMessageListener = nullptr;
			}
		}
		bool FileBased::init(bool resetBlockIndices)
		{
			assert(!mExitCalled);
			std::lock_guard _lock(mWorkMutex);
			bool resetDeferredTransfersCache = false;
			if (!mPublicKeysIndex->init()) {
				// remove index files for regenration
				LOG_F(WARNING, "reset the public key index file");
				// mCachedBlocks.clear();
				mPublicKeysIndex->reset();
				resetDeferredTransfersCache = true;
				resetBlockIndices = true;
			}
			if (!resetDeferredTransfersCache && !mDeferredTransfersCache.init()) {
				resetDeferredTransfersCache = true;
			}
			
			if (resetBlockIndices) {
				model::files::BlockIndex::removeAllBlockIndexFiles(mFolderPath);
			}
			if (!mBlockchainState.init() || resetBlockIndices) {
				LOG_F(WARNING, "reset block state");
				mBlockchainState.reset();
				loadStateFromBlockCache();
			}
			// read basic states into memory
			mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_ADDRESS_INDEX, 0);
			mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 0);
			mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_TRANSACTION_ID, 0);

			if (resetDeferredTransfersCache) {
				LOG_F(WARNING, "reset deferred transfer cache level db");
				mDeferredTransfersCache.reset();
				rescanForDeferredTransfers();
			}

			mIotaMessageListener->run();
			return true;
		}
		void FileBased::exit()
		{
			std::lock_guard _lock(mWorkMutex);
			mExitCalled = true;
			if (mIotaMessageListener) {
				delete mIotaMessageListener;
				mIotaMessageListener = nullptr;
			}

			Profiler timeUsed;
			// wait until all Task of TaskObeserver are finished, wait a second and check if number decreased,
			// if number no longer descrease after a second and we wait more than 10 seconds total, exit loop
			while (auto pendingTasksCount = mTaskObserver->getPendingTasksCount()) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
				if (mTaskObserver->getPendingTasksCount() >= pendingTasksCount && timeUsed.seconds() > 10) {
					break;
				}
			}
			mDeferredTransfersCache.exit();
			mCachedBlocks.clear();
			mBlockchainState.exit();
			mPublicKeysIndex->exit();
		}
		bool FileBased::addGradidoTransaction(data::ConstGradidoTransactionPtr gradidoTransaction, memory::ConstBlockPtr messageId, Timepoint confirmedAt)
		{

		}
		TransactionEntries FileBased::findAll(const Filter& filter/* = Filter::ALL_TRANSACTIONS */) const
		{

		}

		std::vector<uint64_t> FileBased::findAllFast(const Filter& filter) const
		{

		}

		size_t FileBased::countResults(const Filter& filter) const
		{

		}

		TransactionEntries FileBased::findTimeoutedDeferredTransfersInRange(
			memory::ConstBlockPtr senderPublicKey,
			TimepointInterval timepointInterval,
			uint64_t maxTransactionNr
		) const {

		}

		//! find all transfers which redeem a deferred transfer in date range
		//! \param senderPublicKey sender public key of sending account of deferred transaction
		//! \return list with transaction pairs, first is deferred transfer, second is redeeming transfer
		std::list<DeferredRedeemedTransferPair> FileBased::findRedeemedDeferredTransfersInRange(
			memory::ConstBlockPtr senderPublicKey,
			TimepointInterval timepointInterval,
			uint64_t maxTransactionNr
		) const {

		}

		std::shared_ptr<TransactionEntry> FileBased::getTransactionForId(uint64_t transactionId) const
		{
			std::lock_guard _lock(mWorkMutex);
			auto blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 0);
			std::shared_ptr<cache::Block> block;
			do {
				block = getBlock(blockNr);
				blockNr--;
			} while (transactionId < block->getBlockIndex()->getMinTransactionNr() && blockNr > 0);

			return block->getTransaction(transactionId);
		}
		AbstractProvider* FileBased::getProvider() const
		{
			return FileBasedProvider::getInstance();
		}


		void FileBased::pushTransactionEntry(std::shared_ptr<TransactionEntry> transactionEntry)
		{

		}

		void FileBased::loadStateFromBlockCache()
		{
			Profiler timeUsed;
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_ADDRESS_INDEX, mPublicKeysIndex->getLastIndex());
			auto lastBlockNr = model::files::Block::findLastBlockFileInFolder(mFolderPath);
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_BLOCK_NR, lastBlockNr);
			auto block = getBlock(lastBlockNr);
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_TRANSACTION_ID, block->getBlockIndex()->getMaxTransactionNr());			
			LOG_F(INFO, "timeUsed: %s", timeUsed.string().data());
		}
		void FileBased::rescanForDeferredTransfers()
		{
			Filter f;
			Timepoint now;
			Profiler timeUsed;
			std::set<uint32_t> deferredTransfersAddressIndices;
			f.timepointInterval = TimepointInterval(now - GRADIDO_DEFERRED_TRANSFER_MAX_TIMEOUT_INTERVAL, now);
			f.searchDirection = SearchDirection::ASC;
			f.filterFunction = [&](const TransactionEntry& entry) -> FilterResult {
				if (entry.isDeferredTransfer()) {
					auto bodyBytes = entry.getTransactionBody();
					if (!bodyBytes) {
						throw GradidoNullPointerException("missing transaction body an TransactionEntry", "TransactionBody", __FUNCTION__);
					}
					auto recipient = bodyBytes->getDeferredTransfer()->getRecipientPublicKey();
					if (!recipient) {
						throw GradidoNullPointerException("missing public key on deferredTransfer", "memory::Block", __FUNCTION__);
					}
					auto recipientAddressIndex = mPublicKeysIndex->getOrAddIndexForString(recipient->copyAsString());
					deferredTransfersAddressIndices.insert(recipientAddressIndex);
					mDeferredTransfersCache.addTransactionNrForAddressIndex(recipientAddressIndex, entry.getTransactionNr());
				}
				// maybe it is a deferred transfer redeeming another deferred transfer so always look 
				// mDeferredTransfersCache will check for dublettes
				const NodeTransactionEntry* nodeEntry = dynamic_cast<const NodeTransactionEntry*>(&entry);
				const auto& addressIndices = nodeEntry->getAddressIndices();
				for (const auto& addressIndex : addressIndices) {
					if (deferredTransfersAddressIndices.find(addressIndex) != deferredTransfersAddressIndices.end()) {
						mDeferredTransfersCache.addTransactionNrForAddressIndex(addressIndex, entry.getTransactionNr());
					}
				}
				return FilterResult::DISMISS;
			};
			findAll(f);
			LOG_F(INFO, "timeUsed: %s", timeUsed.string().data());
		}

		std::shared_ptr<cache::Block> FileBased::getBlock(uint32_t blockNr) const 
		{
			auto block = mCachedBlocks.get(blockNr);
			if (!block) {
				auto block = std::make_shared<cache::Block>(blockNr, getptr());
				if (!block->init()) {
					// TODO: implement remote sync in this case
					LOG_F(FATAL, "%d invalid block file", blockNr);
					ServerApplication::terminate();
				}
				mCachedBlocks.add(blockNr, block);
				return block;
			}
			return block.value();
		}
	}
}