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
		FileBased::FileBased(Private, std::string_view communityId, std::string_view alias, std::string_view folder)
			: Abstract(communityId),
			mExitCalled(false),
			mFolderPath(folder),
			mAlias(alias),
			mTaskObserver(std::make_shared<TaskObserver>()),
			mIotaMessageListener(new iota::MessageListener(communityId, alias)),
			mPublicKeysIndex(std::make_shared<Dictionary>(std::string(folder).append("/pubkeysCache"))),
			mBlockchainState(std::string(folder).append("/.state")),
			mDeferredTransfersCache(std::string(folder).append("/deferredTransferCache"), communityId),
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
			if (!mPublicKeysIndex->init(GRADIDO_NODE_MAGIC_NUMBER_PUBLIC_KEYS_INDEX_CACHE_MEGA_BTYES)) {
				// remove index files for regenration
				LOG_F(WARNING, "reset the public key index file");
				// mCachedBlocks.clear();
				mPublicKeysIndex->reset();
				resetDeferredTransfersCache = true;
				resetBlockIndices = true;
			}
			if (!resetDeferredTransfersCache && !mDeferredTransfersCache.init(GRADIDO_NODE_MAGIC_NUMBER_DEFERRED_TRANSFERS_CACHE_MEGA_BTYES)) {
				resetDeferredTransfersCache = true;
			}
			
			if (resetBlockIndices) {
				model::files::BlockIndex::removeAllBlockIndexFiles(mFolderPath);
			}
			if (!mBlockchainState.init(GRADIDO_NODE_MAGIC_NUMBER_BLOCKCHAIN_STATE_CACHE_SIZE_BYTES) || resetBlockIndices) {
				LOG_F(WARNING, "reset block state");
				mBlockchainState.reset();
				loadStateFromBlockCache();
			}
			// read basic states into memory
			mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_ADDRESS_INDEX, 0);
			mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 0);
			mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_TRANSACTION_ID, 0);

			if (!mMessageIdsCache.init(GRADIDO_NODE_MAGIC_NUMBER_IOTA_MESSAGE_ID_CACHE_MEGA_BYTES * 1024 * 1024)) {
				mMessageIdsCache.reset();
				if (!mMessageIdsCache.init(GRADIDO_NODE_MAGIC_NUMBER_IOTA_MESSAGE_ID_CACHE_MEGA_BYTES * 1024 * 1024)) {
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
			// load first GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE transaction into cache and validate the transaction to check file integrity
			Profiler timeUsed;
			FilterBuilder builder;
			int count = 0;
			findAll(builder
				.setSearchDirection(SearchDirection::DESC)
				.setPagination({ GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE })
				.setFilterFunction([this, &count](const TransactionEntry& transactionEntry) -> FilterResult {
					auto previousTransactionNr = transactionEntry.getTransactionNr() - 1;
					data::ConstConfirmedTransactionPtr previousConfirmedTransaction;
					auto transactionBody = transactionEntry.getTransactionBody();
					validate::Context validator(*transactionEntry.getConfirmedTransaction());
					if (previousTransactionNr >= 1) {
						auto entry = getTransactionForId(previousTransactionNr);
						if (entry) {
							previousConfirmedTransaction = entry->getConfirmedTransaction();
						}
					}
					validate::Type validationLevel = validate::Type::SINGLE | validate::Type::ACCOUNT;
					if (transactionBody->getType() != data::CrossGroupType::LOCAL) {
						validationLevel = validationLevel | validate::Type::PAIRED;
					}
					if (previousConfirmedTransaction) {
						validationLevel = validationLevel | validate::Type::PREVIOUS;
						validator.setSenderPreviousConfirmedTransaction(previousConfirmedTransaction);
					}										
					validator.run(validationLevel, getCommunityId(), getProvider());
					mTransactionHashCache.push(*transactionEntry.getConfirmedTransaction());
					count++;
					return FilterResult::DISMISS;
				})
				.build()
			);
			LOG_F(INFO, "time used for loading and validating last: %d/%d transactions: %s",
				count,
				GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE,
				timeUsed.string().data()
			);

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
			if (isTransactionAlreadyExist(*gradidoTransaction)) {
				LOG_F(WARNING, "transaction skipped because it already exist");
				return false;
			}
			auto lastTransaction = findOne(Filter::LAST_TRANSACTION);
			uint64_t id = 1;
			if (lastTransaction) {
				id = lastTransaction->getTransactionNr() + 1;
			}			
			
			auto& lastBlock = getBlock(mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1));
			if (id <= lastBlock.getBlockIndex().getTransactionsCount()) {
				// don't find correct last transaction even if it should exist
				lastTransaction = findOne(Filter::LAST_TRANSACTION);
			}
			
			auto transactionBody = gradidoTransaction->getTransactionBody();
			// check for deferred transfer and if found, add to deferred transfer cache
			memory::ConstBlockPtr transferSenderPublicKey = nullptr;
			if (transactionBody->isDeferredTransfer()) {
				auto deferredTransferRecipientPublicKeyIndex = mPublicKeysIndex->getOrAddIndexForString(
					transactionBody
					->getDeferredTransfer()
					->getRecipientPublicKey()
					->copyAsString()
				);
				mDeferredTransfersCache.addTransactionNrForAddressIndex(deferredTransferRecipientPublicKeyIndex, id);
				transferSenderPublicKey = transactionBody
					->getDeferredTransfer()
					->getSenderPublicKey()
					;
			}
			else if (transactionBody->isTransfer()) {
				transferSenderPublicKey =
					transactionBody
					->getTransfer()
					->getSender()
					.getPubkey()
				;
			}
			uint32_t transferSenderPublicKeyIndex = 0;
			if (transferSenderPublicKey) {
				transferSenderPublicKeyIndex = mPublicKeysIndex->getIndexForString(transferSenderPublicKey->copyAsString());
				// if gradido source is a deferred transfer transaction
				if (mDeferredTransfersCache.isDeferredTransfer(transferSenderPublicKeyIndex)) {
					mDeferredTransfersCache.addTransactionNrForAddressIndex(transferSenderPublicKeyIndex, id);
				}
			}

			calculateAccountBalance::Context accountBalanceCalculator(*this);
			auto accountBalance = accountBalanceCalculator.run(gradidoTransaction, confirmedAt, id);
			gradido::data::ConstConfirmedTransactionPtr lastConfirmedTransaction;
			if (lastTransaction) {
				lastConfirmedTransaction = lastTransaction->getConfirmedTransaction();
			}
			auto confirmedTransaction = std::make_shared<data::ConfirmedTransaction>(
				id,
				gradidoTransaction,
				confirmedAt,
				GRADIDO_CONFIRMED_TRANSACTION_V3_3_VERSION_STRING,
				messageId,
				accountBalance.toString(),
				lastConfirmedTransaction
			);
			validate::Type validationLevel = validate::Type::SINGLE | validate::Type::ACCOUNT;
			if (lastTransaction) {
				validationLevel = validationLevel | validate::Type::PREVIOUS;
			}
			
			if (transactionBody->getType() != data::CrossGroupType::LOCAL) {
				validationLevel = validationLevel | validate::Type::PAIRED;
			}
			if (transactionBody->isCreation()) {
				validationLevel = validationLevel | validate::Type::MONTH_RANGE;
			}

			validate::Context validator(*confirmedTransaction);
			validator.setSenderPreviousConfirmedTransaction(lastConfirmedTransaction);
			validator.run(validationLevel, getCommunityId(), getProvider());

			auto blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			auto& block = getBlock(blockNr);
			auto nodeTransactionEntry = std::make_shared<NodeTransactionEntry>(confirmedTransaction, getptr());
			if (!block.pushTransaction(nodeTransactionEntry)) {
				// block was already stopped, so we can  stop here also 
				LOG_F(WARNING, "couldn't push transaction: %llu to block: %d", id, blockNr);
				return false;
			}
			mBlockchainState.updateState(DefaultStateKeys::LAST_TRANSACTION_ID, id);
			mBlockchainState.updateState(DefaultStateKeys::LAST_ADDRESS_INDEX, mPublicKeysIndex->getLastIndex());
			mTransactionHashCache.push(*nodeTransactionEntry->getConfirmedTransaction());
			mMessageIdsCache.add(confirmedTransaction->getMessageId(), id);
			// add public keys to index
			auto involvedAddresses = transactionBody->getInvolvedAddresses();
			for (const auto& address : involvedAddresses) {
				mPublicKeysIndex->getOrAddIndexForString(address->copyAsString());
			}

			// check if a deferred transfer was completly redeemed
			if (transferSenderPublicKeyIndex && mDeferredTransfersCache.isDeferredTransfer(transferSenderPublicKeyIndex)) {										
				auto balance = accountBalanceCalculator.run(transferSenderPublicKey, confirmedAt, id);
				if (GradidoUnit::zero() == balance) {
					mDeferredTransfersCache.removeAddressIndex(transferSenderPublicKeyIndex);
				}
			}
			if (mCommunityServer) {
				task::TaskPtr notifyClientTask = std::make_shared<task::NotifyClient>(mCommunityServer, confirmedTransaction);
				notifyClientTask->scheduleTask(notifyClientTask);
			}
			return true;
		}

		TransactionEntries FileBased::findAll(const Filter& filter/* = Filter::ALL_TRANSACTIONS */) const
		{
			TransactionEntries result;
			bool stopped = false;
			iterateBlocks(filter, [&](const cache::Block& block) -> bool {
				auto transactionNrs = block.getBlockIndex().findTransactions(filter, *mPublicKeysIndex);
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
				auto transactionNrs = block.getBlockIndex().findTransactions(filter, *mPublicKeysIndex);
				result.insert(result.end(), transactionNrs.begin(), transactionNrs.end());
				return true;
			});
			return result;
		}

		size_t FileBased::findAllResultCount(const Filter& filter) const
		{
			size_t count = 0;
			iterateBlocks(filter, [&](const cache::Block& block) -> bool {
				count += block.getBlockIndex().countTransactions(filter, *mPublicKeysIndex);
				return true;
			});
			return count;
		}

		//! find all deferred transfers which have the timeout in date range between start and end, have senderPublicKey and are not redeemed,
		//! therefore boocked back to sender
		TransactionEntries FileBased::findTimeoutedDeferredTransfersInRange(
			memory::ConstBlockPtr senderPublicKey,
			TimepointInterval timepointInterval,
			uint64_t maxTransactionNr
		) const {
			Filter f;
			f.involvedPublicKey = senderPublicKey;
			f.timepointInterval = TimepointInterval(timepointInterval.getStartDate() - GRADIDO_DEFERRED_TRANSFER_MAX_TIMEOUT_INTERVAL, timepointInterval.getEndDate());
			f.searchDirection = SearchDirection::ASC;
			f.transactionType = data::TransactionType::DEFERRED_TRANSFER;
			// nested findAll call
			f.filterFunction = [&](const TransactionEntry& deferredTransactionEntry) -> FilterResult {
				auto deferredTransfer = deferredTransactionEntry.getTransactionBody()->getDeferredTransfer();
				if (!deferredTransfer) {
					throw GradidoNullPointerException(
						"transaction hasn't expected Type", 
						"data::DeferredTransferTransaction", 
						"FileBased::findTimeoutedDeferredTransfersInRange"
					);
				}
				if (
					!deferredTransfer->getSenderPublicKey()->isTheSame(senderPublicKey) ||
					!timepointInterval.isInsideInterval(deferredTransfer->getTimeout())
				) {
					return FilterResult::DISMISS;
				}
				// first try cache
				auto recipientPublicKeyIndex = mPublicKeysIndex->getOrAddIndexForString(
					deferredTransfer->getRecipientPublicKey()->copyAsString()
				);
				auto transactionNrs = mDeferredTransfersCache.getTransactionNrsForAddressIndex(
					recipientPublicKeyIndex
				);
				// if found in cache, there are not completly redeemed yet
				if (transactionNrs.size()) {
					return FilterResult::USE;
				}

				Filter f2;
				f2.involvedPublicKey = deferredTransfer->getRecipientPublicKey();
				f2.searchDirection = SearchDirection::ASC;
				f2.timepointInterval = TimepointInterval(
					deferredTransactionEntry.getConfirmedTransaction()->getConfirmedAt(), 
					deferredTransfer->getTimeout()
				);
				f2.filterFunction = [&](const TransactionEntry& transaction) -> FilterResult {
					auto transactionBody = transaction.getTransactionBody();
					memory::ConstBlockPtr senderPublicKey = nullptr;
					if (transactionBody->isDeferredTransfer()) {
						senderPublicKey = transactionBody->getDeferredTransfer()->getSenderPublicKey();
					}
					else if (transactionBody->isTransfer()) {
						senderPublicKey = transactionBody->getTransfer()->getSender().getPubkey();
					}
					if (deferredTransfer->getRecipientPublicKey()->isTheSame(senderPublicKey)) {
						return FilterResult::USE | FilterResult::STOP;
					}
					return FilterResult::DISMISS;
				};
				auto redeemingTransactions = findAll(f2);
				if (redeemingTransactions.size()) {
					return FilterResult::DISMISS;
				}
				return FilterResult::USE;
			};
			
			return findAll(f);
		}

		//! find all transfers which redeem a deferred transfer in date range
		//! \param senderPublicKey sender public key of sending account of deferred transaction
		//! \return list with transaction pairs, first is deferred transfer, second is redeeming transfer
		std::list<DeferredRedeemedTransferPair> FileBased::findRedeemedDeferredTransfersInRange(
			memory::ConstBlockPtr senderPublicKey,
			TimepointInterval timepointInterval,
			uint64_t maxTransactionNr
		) const {
			std::list<DeferredRedeemedTransferPair> result;
			Filter f;
			f.involvedPublicKey = senderPublicKey;
			f.timepointInterval = timepointInterval;
			f.maxTransactionNr = maxTransactionNr;
			f.searchDirection = SearchDirection::ASC;
			f.transactionType = data::TransactionType::DEFERRED_TRANSFER;
			// nested findAll call
			f.filterFunction = [&](const TransactionEntry& deferredTransactionEntry) -> FilterResult {
				auto deferredTransfer = deferredTransactionEntry.getTransactionBody()->getDeferredTransfer();
				if (!deferredTransfer) {
					throw GradidoNullPointerException(
						"transaction hasn't expected Type", 
						"data::DeferredTransferTransaction", 
						"FileBased::findRedeemedDeferredTransfersInRange"
					);
				}
				// first try cache
				auto recipientPublicKeyIndex = mPublicKeysIndex->getOrAddIndexForString(
					deferredTransfer->getRecipientPublicKey()->copyAsString()
				);
				auto transactionNrs = mDeferredTransfersCache.getTransactionNrsForAddressIndex(
					recipientPublicKeyIndex
				);
				if (transactionNrs.size()) {
					for (auto transactionNr : transactionNrs) {
						if (transactionNr == deferredTransactionEntry.getTransactionNr()) {
							continue;
						}
						auto transaction = getTransactionForId(transactionNr);
						if (!timepointInterval.isInsideInterval(transaction->getTransactionBody()->getCreatedAt())) {
							continue;
						}
						result.push_back({
							getTransactionForId(deferredTransactionEntry.getTransactionNr()),
							transaction
						});
					}
					return FilterResult::DISMISS;
				}
				// if deferred transfer was already completly redeemed or timeouted and removed from cache, try find all
				Filter f2 = f;
				f2.involvedPublicKey = deferredTransfer->getRecipientPublicKey();
				f2.transactionType = data::TransactionType::NONE;
				f2.filterFunction = [&](const TransactionEntry& transaction) -> FilterResult{
					auto transactionBody = transaction.getTransactionBody();
					memory::ConstBlockPtr senderPublicKey = nullptr;
					if (transactionBody->isDeferredTransfer()) {
						senderPublicKey = transactionBody->getDeferredTransfer()->getSenderPublicKey();
					}
					else if (transactionBody->isTransfer()) {
						senderPublicKey = transactionBody->getTransfer()->getSender().getPubkey();
					}
					if (deferredTransfer->getRecipientPublicKey()->isTheSame(senderPublicKey)) {
						if (result.back().first->getTransactionNr() == deferredTransactionEntry.getTransactionNr()) {
							// TODO: update DeferredRedeemedTransferPair to include like in cache::DeferredTransfer multiple redeeming transactions
							throw std::runtime_error("multiple redeeming deferred transfer transaction not implemented yet!");
						}
						result.push_back({ 
							getTransactionForId(deferredTransactionEntry.getTransactionNr()), 
							getTransactionForId(transaction.getTransactionNr())
						});
					}
					return FilterResult::DISMISS;
				};
				findAll(f2);
				return FilterResult::DISMISS;
			};
			findAll(f);
			return result;
		}

		std::shared_ptr<const TransactionEntry> FileBased::getTransactionForId(uint64_t transactionId) const
		{
			std::lock_guard _lock(mWorkMutex);
			auto blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			do {
				auto& block = getBlock(blockNr);
				if (block.getBlockIndex().hasTransactionNr(transactionId)) {
					return block.getTransaction(transactionId);
				}
				blockNr--;
			} while (blockNr > 0);
			return nullptr;
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

		void FileBased::cleanupDeferredTransferCache()
		{
			mDeferredTransfersCache.removeAddressIndexes([&](uint32_t addressIndex, uint64_t transactionNr) -> bool {
				auto transaction = getTransactionForId(transactionNr);
				auto transactionBody = transaction->getTransactionBody();
				if (!transactionBody->isDeferredTransfer()) {
					throw GradidoNullPointerException("isn't a deferred transfer transaction in deferred transfer cache", "DeferredTransferTransaction", __FUNCTION__);
				}
				auto deferredTransfer = transactionBody->getDeferredTransfer();
				if (deferredTransfer->getTimeout().getAsTimepoint() <= Timepoint()) {
					return true;
				}
				return false;
			});
		}

		void FileBased::loadStateFromBlockCache()
		{
			Profiler timeUsed;
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_ADDRESS_INDEX, mPublicKeysIndex->getLastIndex());
			auto lastBlockNr = model::files::Block::findLastBlockFileInFolder(mFolderPath);
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_BLOCK_NR, lastBlockNr);
			auto& block = getBlock(lastBlockNr);
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_TRANSACTION_ID, block.getBlockIndex().getMaxTransactionNr());			
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
			auto lastBlockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			int blockNr = orderDesc ? lastBlockNr : 1;
			do {
				auto& block = getBlock(blockNr);
				if (!block.getBlockIndex().getTransactionsCount()) {
					break;
				}
				if (!func(block)) {
					break;
				}
				if (orderDesc) {
					blockNr--;
				}
				else {
					blockNr++;
				}
			} while (blockNr >= 1 && blockNr <= lastBlockNr);
		}

		cache::Block& FileBased::getBlock(uint32_t blockNr) const
		{
			assert(blockNr);
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
				return *block;
			}
			return *block.value();
		}
	}
}