#include "FileBased.h"
#include "FileBasedProvider.h"
#include "NodeTransactionEntry.h"
#include "../cache/Exceptions.h"
#include "../model/files/Block.h"
#include "../model/files/BlockIndex.h"
#include "../ServerGlobals.h"
#include "../SystemExceptions.h"
#include "../task/NotifyClient.h"

#include "gradido_blockchain/const.h"
#include "gradido_blockchain/blockchain/FilterBuilder.h"
#include "gradido_blockchain/interaction/calculateAccountBalance/Context.h"
#include "gradido_blockchain/interaction/validate/Context.h"
#include "gradido_blockchain/lib/Profiler.h"

#include "loguru/loguru.hpp"

#include <set>

using namespace cache;

namespace gradido {
	using namespace interaction;
	namespace blockchain {
		FileBased::FileBased(Private, std::string_view communityId, std::string_view folder)
			: Abstract(communityId),
			mExitCalled(false),
			mFolderPath(folder),
			mTaskObserver(std::make_shared<TaskObserver>()),
			mIotaMessageListener(new iota::MessageListener(communityId)),
			mPublicKeysIndex(std::make_shared<Dictionary>(std::string(folder).append("/pubkeys.index"))),
			mBlockchainState(std::string(folder).append("/.state")),
			mDeferredTransfersCache(std::string(folder).append("/deferredTransferCache")),
			mMessageIdsCache(std::string(folder).append("/messageIdCache")),
			mCachedBlocks(ServerGlobals::g_CacheTimeout),
			mTransactionHashCache(communityId)
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

			if (!mMessageIdsCache.init()) {
				mMessageIdsCache.reset();
				if (!mMessageIdsCache.init()) {
					throw ClassNotInitalizedException("cannot initalize message id cache", "cache::MessageId");
				}
				// load last 20 message ids into cache
				FilterBuilder filterBuilder;
				auto transactions = findAll(filterBuilder.setPagination({20}).setSearchDirection(SearchDirection::DESC).build());
				for (auto& transaction : transactions) {
					mMessageIdsCache.add(transaction->getConfirmedTransaction()->getMessageId(), transaction->getTransactionNr());
				}
			}

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
			mMessageIdsCache.exit();
		}
		bool FileBased::addGradidoTransaction(data::ConstGradidoTransactionPtr gradidoTransaction, memory::ConstBlockPtr messageId, Timepoint confirmedAt)
		{
			if (!gradidoTransaction) {
				throw GradidoNullPointerException("missing transaction", "GradidoTransactionPtr", __FUNCTION__);
			}
			if (!messageId) {
				throw GradidoNullPointerException("missing messageId", "memory::ConstBlockPtr", __FUNCTION__);
			}
			std::lock_guard _lock(mWorkMutex);
			if (mExitCalled) { return false;}
			if (isTransactionAlreadyExist(gradidoTransaction)) {
				LOG_F(WARNING, "transaction skipped because it already exist");
				return false;
			}
			auto lastTransaction = findOne(Filter::LAST_TRANSACTION);
			uint64_t id = 1;
			if (lastTransaction) {
				id = lastTransaction->getTransactionNr() + 1;
			}
			calculateAccountBalance::Context accountBalanceCalculator(*this);
			auto accountBalance = accountBalanceCalculator.run(gradidoTransaction, confirmedAt, id);
			auto confirmedTransaction = std::make_shared<data::ConfirmedTransaction>(
				id,
				gradidoTransaction,
				confirmedAt,
				GRADIDO_CONFIRMED_TRANSACTION_V3_3_VERSION_STRING,
				messageId,
				accountBalance.toString(),
				lastTransaction->getConfirmedTransaction()
			);
			validate::Type validationLevel = validate::Type::SINGLE;
			if (lastTransaction) {
				validationLevel = validationLevel | validate::Type::PREVIOUS;
			}
			auto transactionBody = gradidoTransaction->getTransactionBody();
			if (transactionBody->getType() != data::CrossGroupType::LOCAL) {
				validationLevel = validationLevel | validate::Type::PAIRED;
			}
			if (transactionBody->isCreation()) {
				validationLevel = validationLevel | validate::Type::MONTH_RANGE;
			}
			validate::Context validator(*confirmedTransaction);
			validator.setSenderPreviousConfirmedTransaction(lastTransaction->getConfirmedTransaction());
			validator.run(validationLevel, getCommunityId(), getProvider());

			auto blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			auto block = getBlock(blockNr);
			if (!block) {
				throw GradidoNullPointerException("couldn't create new block", "cache::Block", __FUNCTION__);
			}
			auto nodeTransactionEntry = std::make_shared<NodeTransactionEntry>(confirmedTransaction, getptr());
			if (!block->pushTransaction(nodeTransactionEntry)) {
				// block was already stopped, so we can  stop here also 
				LOG_F(WARNING, "couldn't push transaction: %llu to block: %d", id, blockNr);
				return false;
			}
			mBlockchainState.updateState(DefaultStateKeys::LAST_TRANSACTION_ID, id);
			mBlockchainState.updateState(DefaultStateKeys::LAST_ADDRESS_INDEX, mPublicKeysIndex->getLastIndex());
			mTransactionHashCache.push(nodeTransactionEntry);
			mMessageIdsCache.add(confirmedTransaction->getMessageId(), id);
			if (mCommunityServer) {
				task::TaskPtr notifyClientTask = std::make_shared<task::NotifyClient>(mCommunityServer, confirmedTransaction);
				notifyClientTask->scheduleTask(notifyClientTask);
			}
			return false;
		}
		TransactionEntries FileBased::findAll(const Filter& filter/* = Filter::ALL_TRANSACTIONS */) const
		{
			TransactionEntries result;
			bool stopped = false;
			iterateBlocks(filter, [&](const cache::Block& block) -> bool {
				auto transactionNrs = block.getBlockIndex()->findTransactions(filter, *mPublicKeysIndex);
				for (auto transactionNr : transactionNrs) {
					auto transaction = block.getTransaction(transactionNr);
					auto filterResult = filter.matches(transaction, FilterCriteria::FILTER_FUNCTION, mCommunityId);
					if ((filterResult & FilterResult::USE) == FilterResult::USE) {
						result.push_back(transaction);
					}
					if ((filterResult & FilterResult::STOP) == FilterResult::STOP) {
						stopped = true;
						break;
					}
				}
				return !stopped;
			});
			return result;
		}

		std::vector<uint64_t> FileBased::findAllFast(const Filter& filter) const
		{
			std::vector<uint64_t> result;
			iterateBlocks(filter, [&](const cache::Block& block) -> bool {
				auto transactionNrs = block.getBlockIndex()->findTransactions(filter, *mPublicKeysIndex);
				result.insert(result.end(), transactionNrs.begin(), transactionNrs.end());
				return true;
			});
			return result;
		}

		size_t FileBased::findAllResultCount(const Filter& filter) const
		{
			size_t count = 0;
			iterateBlocks(filter, [&](const cache::Block& block) -> bool {
				count += block.getBlockIndex()->countTransactions(filter, *mPublicKeysIndex);
				return true;
			});
			return count;
		}

		TransactionEntries FileBased::findTimeoutedDeferredTransfersInRange(
			memory::ConstBlockPtr senderPublicKey,
			TimepointInterval timepointInterval,
			uint64_t maxTransactionNr
		) const {
			return {};
		}

		//! find all transfers which redeem a deferred transfer in date range
		//! \param senderPublicKey sender public key of sending account of deferred transaction
		//! \return list with transaction pairs, first is deferred transfer, second is redeeming transfer
		std::list<DeferredRedeemedTransferPair> FileBased::findRedeemedDeferredTransfersInRange(
			memory::ConstBlockPtr senderPublicKey,
			TimepointInterval timepointInterval,
			uint64_t maxTransactionNr
		) const {
			return {};
		}

		std::shared_ptr<const TransactionEntry> FileBased::getTransactionForId(uint64_t transactionId) const
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
		std::shared_ptr<const TransactionEntry> FileBased::findByMessageId(
			memory::ConstBlockPtr messageId,
			const Filter& filter/* = Filter::ALL_TRANSACTIONS */
		) const
		{
			auto transactionNr = mMessageIdsCache.has(messageId);
			if (transactionNr) {
				return getTransactionForId(transactionNr);
			}
			return Abstract::findByMessageId(messageId, filter);
		}
		AbstractProvider* FileBased::getProvider() const
		{
			return FileBasedProvider::getInstance();
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

		void FileBased::iterateBlocks(const Filter& filter, std::function<bool(const cache::Block&)> func) const
		{
			bool orderDesc = filter.searchDirection == SearchDirection::DESC;
			int blockNr = 1;
			if (orderDesc) {
				blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			}
			auto block = getBlock(blockNr);
			while (blockNr >= 1 && block->getBlockIndex()->getTransactionsCount()) 
			{
				if (!func(*block)) {
					break;
				}
				if (orderDesc) {
					blockNr--;
				}
				else {
					blockNr++;
				}
				block = getBlock(blockNr);
			}
		}

		std::shared_ptr<cache::Block> FileBased::getBlock(uint32_t blockNr) const 
		{
			auto block = mCachedBlocks.get(blockNr);
			if (!block) {
				auto block = std::make_shared<cache::Block>(blockNr, getptr());
				// return false if block not exist and will be created
				if (!block->init()) {
					if (blockNr > mBlockchainState.readInt32State(DefaultStateKeys::LAST_BLOCK_NR, 1)) {
						mBlockchainState.updateState(DefaultStateKeys::LAST_BLOCK_NR, blockNr);
					}
				}
				mCachedBlocks.add(blockNr, block);
				return block;
			}
			return block.value();
		}
	}
}