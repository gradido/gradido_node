#include "gradido_blockchain/AppContext.h"
#include "FileBased.h"
#include "../client/hiero/ConsensusClient.h"
#include "FileBasedProvider.h"
#include "NodeTransactionEntry.h"
#include "../cache/Exceptions.h"
#include "../hiero/MessageListenerQuery.h"
#include "../model/files/Block.h"
#include "../model/files/BlockIndex.h"
#include "../ServerGlobals.h"
#include "../SystemExceptions.h"
#include "../task/NotifyClient.h"
#include "../task/SyncTopicOnStartup.h"
#include "../client/hiero/MirrorClient.h"

#include "gradido_blockchain/const.h"
#include "gradido_blockchain/blockchain/batch/signaturesVerify.h"
#include "gradido_blockchain/blockchain/batch/ThreadingPolicy.h"
#include "gradido_blockchain/blockchain/Filter.h"
#include "gradido_blockchain/blockchain/FilterBuilder.h"
#include "gradido_blockchain/data/adapter/publicKey.h"
#include "gradido_blockchain/data/compact/ConfirmedGradidoTx.h"
#include "gradido_blockchain/data/compact/PublicKeyIndex.h"
#include "gradido_blockchain/data/Timestamp.h"
#include "gradido_blockchain/interaction/confirmTransaction/Context.h"
#include "gradido_blockchain/interaction/validate/Context.h"
#include "gradido_blockchain/serialization/toJsonString.h"
#include "gradido_blockchain/lib/Profiler.h"

#include "loguru/loguru.hpp"

#include <set>
#include <chrono>

using namespace cache;
using std::string, std::string_view, std::vector;
using std::shared_ptr, std::make_shared;
using client::hiero::ConsensusClient;
using controller::SimpleOrderingManager;
using serialization::toJsonString;

namespace gradido {
	using data::adapter::toPublicKey;
	using data::AddressType, data::Timestamp, data::LedgerAnchor;
	using data::compact::ConstConfirmedTxPtr, data::compact::ConfirmedTxs, data::compact::PublicKeyIndex;

	using namespace interaction;
	namespace blockchain {
		using batch::ThreadingPolicy, batch::verifySignatures;

		FileBased::FileBased(
			Private,
			const string& communityId,
			const hiero::TopicId& topicId,
			string_view alias,
			string_view folder,
			vector<shared_ptr<ConsensusClient>>&& hieroClients)
			: Abstract(g_appContext->getOrAddCommunityIdIndex(communityId)),
			mExitCalled(false),
			mHieroTopicId(topicId),
			mAlias(alias),
			mFolderPath(folder),
			mCommunityId(communityId),
			mTaskObserver(std::make_shared<TaskObserver>()),
			mOrderingManager(std::make_shared<SimpleOrderingManager>(communityId)),
			// mIotaMessageListener(new iota::MessageListener(communityId, alias)),
			mPublicKeysIndex((string(folder).append("/pubkeysCache"))),
			mBlockchainState(string(folder).append("/.state")),
			mLedgerAnchorCache(string(folder).append("/messageIdCache")),
			mTransactionTriggerEventsCache(std::string(folder).append("/transactionTriggerEventCache")),
			mCachedBlocks(ServerGlobals::g_CacheTimeout),
			mTransactionHashCache(communityId),
			mHieroClients(std::move(hieroClients))
		{
			assert(mHieroTopicId.empty() || mHieroClients.size());
		}

		FileBased::~FileBased()
		{
			/*if (mIotaMessageListener) {
				delete mIotaMessageListener;
				mIotaMessageListener = nullptr;
			}
			*/
		}
		bool FileBased::init(bool resetBlockIndices)
		{
			assert(!mExitCalled);
			std::lock_guard _lock(mWorkMutex);
			if (!mPublicKeysIndex.init(GRADIDO_NODE_MAGIC_NUMBER_PUBLIC_KEYS_INDEX_CACHE_MEGA_BTYES * 1024 * 1024)) {
				// remove index files for regenration
				LOG_F(WARNING, "reset the public key index file");
				// mCachedBlocks.clear();
				mPublicKeysIndex.reset();
				resetBlockIndices = true;
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
			mBlockchainState.readInt64State(cache::DefaultStateKeys::LAST_HIERO_TOPIC_SEQUENCE_NUMBER, 0);
			mBlockchainState.readState(cache::DefaultStateKeys::LAST_HIERO_TOPIC_ID, mHieroTopicId.toString());

			return true;
		}

		bool FileBased::startValidationTransactions()
		{
			auto lastBlockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 0);
			if (!mLedgerAnchorCache.init(GRADIDO_NODE_MAGIC_NUMBER_IOTA_MESSAGE_ID_CACHE_MEGA_BYTES * 1024 * 1024)) {
				mLedgerAnchorCache.reset();
				if (!mLedgerAnchorCache.init(GRADIDO_NODE_MAGIC_NUMBER_IOTA_MESSAGE_ID_CACHE_MEGA_BYTES * 1024 * 1024)) {
					throw ClassNotInitalizedException("cannot initalize message id cache", "cache::MessageId");
				}
				// load last 20 message ids into cache
				FilterBuilder filterBuilder;
				auto transactions = findAll(filterBuilder.setPagination({ 20 }).setSearchDirection(SearchDirection::DESC).build());
				for (auto& transaction : transactions) {
					mLedgerAnchorCache.add(transaction->getConfirmedTransaction()->getLedgerAnchor(), transaction->getTransactionNr());
				}
			}

			if (!mTransactionTriggerEventsCache.init(GRADIDO_NODE_MAGIC_NUMBER_TRANSACTION_TRIGGER_EVENTS_CACHE_MEGA_BTYES * 1024 * 1024)) {
				Profiler timeUsed;
				rescanForTransactionTriggerEvents();
				LOG_F(INFO, "rescan blockchain for transaction trigger events, time: %s", timeUsed.string().data());
			}

			// if state was empty, we better validate full blockchain
			if (!lastBlockNr) {
				return validateLastTransactions(0);
			}
			else {
				return validateLastTransactions(GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE);
			}
		}

		std::shared_ptr<task::SyncTopicOnStartup> FileBased::initOnline()
		{
			auto hieroTopicIdString = mBlockchainState.readState(cache::DefaultStateKeys::LAST_HIERO_TOPIC_ID, mHieroTopicId.toString());
			if (hieroTopicIdString.size()) {
				return std::make_shared<task::SyncTopicOnStartup>(
					mBlockchainState.readInt64State(cache::DefaultStateKeys::LAST_HIERO_TOPIC_SEQUENCE_NUMBER, 0),
					hieroTopicIdString,
					getptr()
				);
			}
			LOG_F(WARNING, "init online called for community without hiero topic id");
			return nullptr;
		}

		void FileBased::startListening(data::Timestamp lastTransactionConfirmedAt)
		{
			if (mHieroMessageListener) {
				LOG_F(WARNING, "called again, while listener where already existing");
			}
			auto hieroTopicId = hiero::TopicId(mBlockchainState.readState(cache::DefaultStateKeys::LAST_HIERO_TOPIC_ID, mHieroTopicId.toString()));
			if (hieroTopicId.empty()) {
				LOG_F(WARNING, "startListening called without valid hiero topic id");
				return;
			}
			data::Timestamp listenFrom = { lastTransactionConfirmedAt.getSeconds(), lastTransactionConfirmedAt.getNanos() + 1 };
			auto now = std::chrono::system_clock::now();
			// TODO: restart after connection was closed because of timeout
			auto endTime = now + std::chrono::duration(std::chrono::years(10));
			mHieroMessageListener = std::make_shared<hiero::MessageListenerQuery>(
				hieroTopicId,
				mCommunityId,
				hiero::ConsensusTopicQuery( hieroTopicId, listenFrom, endTime )
			);
			ServerGlobals::g_HieroMirrorNode->subscribeTopic(mHieroMessageListener);
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_HIERO_TOPIC_ID, hieroTopicId.toString());
		}

		void FileBased::exit()
		{
			std::lock_guard _lock(mWorkMutex);
			mExitCalled = true;
			/*if (mIotaMessageListener) {
				delete mIotaMessageListener;
				mIotaMessageListener = nullptr;
			}*/
			if (mHieroMessageListener) {
				mHieroMessageListener->cancelConnection();
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
			mOrderingManager->exit();
			mCachedBlocks.clear();
			mBlockchainState.exit();
			mPublicKeysIndex.exit();
			mLedgerAnchorCache.exit();
			mTransactionTriggerEventsCache.exit();
		}

		bool FileBased::createAndAddConfirmedTransaction(
			data::ConstGradidoTransactionPtr gradidoTransaction,
			const data::LedgerAnchor& ledgerAnchor,
			data::Timestamp confirmedAt
		) {
			if (!gradidoTransaction) {
				throw GradidoNullPointerException("missing transaction", "GradidoTransactionPtr", __FUNCTION__);
			}
			if (ledgerAnchor.empty()) {
				throw GradidoNullPointerException("empty ledger anchor", "gradido::data::LedgerAnchor", __FUNCTION__);
			}
			std::lock_guard _lock(mWorkMutex);
			if (mExitCalled) { return false;}
			confirmTransaction::Context confirmTransactionContext(getptr());
			auto role = confirmTransactionContext.createRole(gradidoTransaction, ledgerAnchor, confirmedAt);
			auto confirmedTransaction = confirmTransactionContext.run(role);
			// will occure if transaction already exist
			if (!confirmedTransaction) {
				return false;
			}
			auto blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			auto& block = getBlock(blockNr);
			auto nodeTransactionEntry = make_shared<NodeTransactionEntry>(confirmedTransaction, getptr());
			if (!block.pushTransaction(nodeTransactionEntry, mPublicKeysIndex, *g_appContext)) {
				// block was already stopped, so we can  stop here also
				LOG_F(WARNING, "couldn't push transaction: %lu to block: %d", confirmedTransaction->getId(), blockNr);
				return false;
			}
			role->runPastAddToBlockchain(confirmedTransaction, getptr());
			mBlockchainState.updateState(DefaultStateKeys::LAST_TRANSACTION_ID, confirmedTransaction->getId());
			mBlockchainState.updateState(DefaultStateKeys::LAST_ADDRESS_INDEX, mPublicKeysIndex.getLastIndex());
			mTransactionHashCache.push(*nodeTransactionEntry->getConfirmedTransaction());
			mLedgerAnchorCache.add(confirmedTransaction->getLedgerAnchor(), confirmedTransaction->getId());
			// add public keys to index
			auto involvedAddresses = confirmedTransaction->getInvolvedAddresses();
			for (const auto& address : involvedAddresses) {
				mPublicKeysIndex.getOrAddIndexForData(toPublicKey(address));
			}
			if (mCommunityServer) {
				task::TaskPtr notifyClientTask = std::make_shared<task::NotifyClient>(mCommunityServer, confirmedTransaction);
				notifyClientTask->scheduleTask(notifyClientTask);
			}
			return true;
		}

		bool FileBased::createAndAddConfirmedTransactionExtern(
				data::ConstGradidoTransactionPtr gradidoTransaction,
				const data::LedgerAnchor& ledgerAnchor,
				std::vector<data::AccountBalance> accountBalances
			)
		{
			if (!gradidoTransaction) {
				throw GradidoNullPointerException("missing transaction", "GradidoTransactionPtr", __FUNCTION__);
			}
			if (ledgerAnchor.empty()) {
				throw GradidoNullPointerException("empty ledger anchor", "gradido::data::LedgerAnchor", __FUNCTION__);
			}
			std::lock_guard _lock(mWorkMutex);
			if (mExitCalled) { return false; }
			confirmTransaction::Context confirmTransactionContext(getptr());
			auto role = confirmTransactionContext.createRole(
				gradidoTransaction,
				ledgerAnchor,
				gradidoTransaction->getTransactionBody()->getCreatedAt()
			);
			if (!role) {
				throw GradidoNotImplementedException("missing role for gradido transaction");
			}
			role->setAccountBalances(accountBalances);
			auto confirmedTransaction = confirmTransactionContext.run(role);
			// will occure if transaction already exist
			if (!confirmedTransaction) {
				return false;
			}
			auto blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			auto& block = getBlock(blockNr);
			auto nodeTransactionEntry = make_shared<NodeTransactionEntry>(confirmedTransaction, getptr());
			if (!block.pushTransaction(nodeTransactionEntry, mPublicKeysIndex, *g_appContext)) {
				// block was already stopped, so we can  stop here also
				LOG_F(WARNING, "couldn't push transaction: %lu to block: %d", confirmedTransaction->getId(), blockNr);
				return false;
			}
			role->runPastAddToBlockchain(confirmedTransaction, getptr());
			mBlockchainState.updateState(DefaultStateKeys::LAST_TRANSACTION_ID, confirmedTransaction->getId());
			mBlockchainState.updateState(DefaultStateKeys::LAST_ADDRESS_INDEX, mPublicKeysIndex.getLastIndex());
			mTransactionHashCache.push(*nodeTransactionEntry->getConfirmedTransaction());
			mLedgerAnchorCache.add(confirmedTransaction->getLedgerAnchor(), confirmedTransaction->getId());
			// add public keys to index
			auto involvedAddresses = confirmedTransaction->getInvolvedAddresses();
			for (const auto& address : involvedAddresses) {
				mPublicKeysIndex.getOrAddIndexForData(toPublicKey(address));
			}
			if (mCommunityServer) {
				task::TaskPtr notifyClientTask = std::make_shared<task::NotifyClient>(mCommunityServer, confirmedTransaction);
				notifyClientTask->scheduleTask(notifyClientTask);
			}
			return true;
		}

		void FileBased::updateLastKnownSequenceNumber(uint64_t newSequenceNumber)
		{
			mBlockchainState.updateState(DefaultStateKeys::LAST_HIERO_TOPIC_SEQUENCE_NUMBER, newSequenceNumber);
		}

		void FileBased::addTransactionTriggerEvent(std::shared_ptr<const data::TransactionTriggerEvent> transactionTriggerEvent)
		{
			std::lock_guard _lock(mWorkMutex);
			mTransactionTriggerEventsCache.addTransactionTriggerEvent(transactionTriggerEvent);
		}

		void FileBased::removeTransactionTriggerEvent(const data::TransactionTriggerEvent& transactionTriggerEvent)
		{
			std::lock_guard _lock(mWorkMutex);
			mTransactionTriggerEventsCache.removeTransactionTriggerEvent(transactionTriggerEvent);
		}

		std::vector<std::shared_ptr<const data::TransactionTriggerEvent>> FileBased::findTransactionTriggerEventsInRange(Timestamp startDate, Timestamp endDate)
		{
			std::lock_guard _lock(mWorkMutex);
			return mTransactionTriggerEventsCache.findTransactionTriggerEventsInRange(startDate, endDate);
		}

		std::shared_ptr<const data::TransactionTriggerEvent> FileBased::findNextTransactionTriggerEventInRange(Timestamp startDate, Timestamp endDate)
		{
			std::lock_guard _lock(mWorkMutex);
			return mTransactionTriggerEventsCache.findNextTransactionTriggerEventInRange(startDate, endDate);
		}

		TransactionEntries FileBased::findAll(const Filter& filter/* = Filter::ALL_TRANSACTIONS */) const
		{
			TransactionEntries result;
			// if pagination is used, filterCopy contain count of still to find transactions
			Filter filterCopy(filter);
			bool stopped = false;
			iterateBlocks(filter.searchDirection, [&](const cache::Block& block) -> bool {
				auto transactionNrs = block.getBlockIndex().findTransactions(filterCopy, mPublicKeysIndex);
				for (auto transactionNr : transactionNrs) {
					if (!filter.pagination.hasCapacityLeft(result.size())) {
						return false;
					}
					auto transaction = block.getTransaction(transactionNr, *g_appContext);
					auto filterResult = filter.matches(transaction, FilterCriteria::FILTER_FUNCTION | FilterCriteria::TIMEPOINT_INTERVAL);
					if ((filterResult & FilterResult::USE) == FilterResult::USE) {
						result.push_back(transaction);
					}
					if ((filterResult & FilterResult::STOP) == FilterResult::STOP) {
						stopped = true;
						break;
					}
				}
				if (filter.pagination.size) {
					filterCopy.pagination.size = filter.pagination.size - result.size();
					// we have requested result count, let's exit here
					if (filterCopy.pagination.size <= 0) {
						return false;
					}
				}
				return !stopped;
			});
			return result;
		}

		ConfirmedTxs FileBased::findAll(const CompactFilter& filter) const
		{
			ConfirmedTxs results;
			// if pagination is used, filterCopy contain count of still to find transactions
			CompactFilter filterCopy(filter);
			auto skipEntries = filter.pagination.skipEntriesCount();
			int paginationCursor = 0;
			iterateBlocks(filterCopy.searchDirection,
				[&](const cache::Block& block) -> bool
				{
					const auto& transactionIndex = block.getBlockIndex();
					if (PublicKeySearchType::BalanceChangingPublicKey == filterCopy.publicKeySearchType && filterCopy.publicKeyIndex.communityIdIndex == mCommunityIdIndex) 
					{
						filterCopy.pagination.page = 1;
						do {
							auto balanceChangingTxsInRange = transactionIndex.findTransactionsBalanceChangingForPublicKey(filterCopy);
							if (balanceChangingTxsInRange.empty()) {
								break;
							}
							for (const auto& tx : balanceChangingTxsInRange) {
								auto transaction = getConfirmedTxForId(tx);
								if (!transaction) {
									throw GradidoBlockchainTransactionNotFoundException("confirmed tx not found").setTransactionId(tx);
								}
								auto filterResult = filterCopy.matches(*transaction, FilterCriteria::TIMEPOINT_INTERVAL);
								if ((filterResult & FilterResult::USE) == FilterResult::USE) {
									if (paginationCursor >= skipEntries) {
										results.push_back(transaction);
										if (!filterCopy.pagination.hasCapacityLeft(results.size())) {
											return false;
										}
									}
									paginationCursor++;
								}
								if ((filterResult & FilterResult::STOP) == FilterResult::STOP) {
									return false;
								}
							}
							if (filterCopy.pagination.empty() || filter.pagination.size > balanceChangingTxsInRange.size()) {
								break;
							}
							filterCopy.pagination.page++;
						} while (filter.pagination.hasCapacityLeft(results.size()));
						return true;
					}

					transactionIndex.lock();
					try {
						auto startIt = transactionIndex.begin(filter);
						auto endIt = transactionIndex.end(filter);
						auto it = startIt;
						for (; it != endIt; ++it) 
						{
							auto transaction = block.getCompactTransaction(*it, *g_appContext);
							if (!transaction) {
								throw GradidoBlockchainTransactionNotFoundException("confirmed tx not found").setTransactionId(*it);
							}
							auto filterResult = filter.matches(*transaction, FilterCriteria::TIMEPOINT_INTERVAL);
							if ((filterResult & FilterResult::USE) == FilterResult::USE) {
								if (paginationCursor >= skipEntries) {
									results.push_back(transaction);
									if (!filter.pagination.hasCapacityLeft(results.size())) {
										transactionIndex.unlock();
										return false;
									}
								}
								paginationCursor++;
							}
							if ((filterResult & FilterResult::STOP) == FilterResult::STOP) {
								transactionIndex.unlock();
								return false;
							}
						}
						transactionIndex.unlock();
						return true;
					}
					catch (...) {
						transactionIndex.unlock();
						throw;
					}
				});
			return results;
		}

		size_t FileBased::countAll(const Filter& filter/* = Filter::ALL_TRANSACTIONS*/) const
		{
			size_t count = 0;
			// check if filter has fields which aren't checked by index
			if (!gradido::blockchain::TransactionsIndex::canMatchWithoutDeserialize(filter)) {
				LOG_F(
					WARNING,
					"slow count, detect fields in Filter which aren't covered by index: %s",
					toJsonString(filter).c_str()
				);
				return findAll(filter).size();
			}
			iterateBlocks(filter.searchDirection, [&](const cache::Block& block) -> bool {
				count += block.getBlockIndex().countTransactions(filter, mPublicKeysIndex);
				return true;
			});
			return count;
		}

		std::vector<uint64_t> FileBased::findAllFast(const Filter& filter) const
		{
			// check if filter has fields which aren't checked by index
			if (!gradido::blockchain::TransactionsIndex::canMatchWithoutDeserialize(filter)) {
				LOG_F(
					ERROR,
					"findAllFast call with invalid filter not covered by index: %s",
					toJsonString(filter).c_str()
				);
				return {};
			}
			std::vector<uint64_t> result;
			// if pagination is used, filterCopy contain count of still to find transactions
			Filter filterCopy(filter);
			iterateBlocks(filter.searchDirection, [&](const cache::Block& block) -> bool {
				auto transactionNrs = block.getBlockIndex().findTransactions(filterCopy, mPublicKeysIndex);
				result.insert(result.end(), transactionNrs.begin(), transactionNrs.end());
				if (filter.pagination.size) {
					filterCopy.pagination.size = filter.pagination.size - result.size();
					// we have requested result count, let's exit here
					if (filterCopy.pagination.size <= 0) return false;
				}
				return true;
			});
			return result;
		}

		data::AddressType FileBased::getAddressType(const Filter& filter/* = Filter::LAST_TRANSACTION*/) const
		{
			// return getAddressTypeSlow(filter);
			if (!filter.involvedPublicKey || filter.involvedPublicKey->isEmpty()) {
				throw GradidoNodeInvalidDataException("missing public key, please use filter with involvedPublicKey set");
			}
			auto publicKeyIndexOptional = mPublicKeysIndex.getIndexForData(toPublicKey(filter.involvedPublicKey));
			if (!publicKeyIndexOptional) {
				return AddressType::NONE;
			}
			uint32_t publicKeyUint32 = (uint32_t)publicKeyIndexOptional;
			if (publicKeyUint32 != publicKeyIndexOptional) {
				throw GradidoNodeInvalidDataException("public key index overflow");
			}
			PublicKeyIndex publicKeyIndex = { .communityIdIndex = mCommunityIdIndex, .publicKeyIndex = publicKeyUint32 };
			data::AddressType result = data::AddressType::NONE;
			iterateBlocks(filter.searchDirection, [&](const cache::Block& block) -> bool {
				auto addressTypeStateChange = block.getBlockIndex().getAddressType(publicKeyIndex);
				result = addressTypeStateChange.getValue();
				if (addressTypeStateChange.getTxId()) {
					auto tx = getTransactionForId(addressTypeStateChange.getTxId());
					if (FilterResult::USE != (filter.matches(tx, FilterCriteria::MAX) & FilterResult::USE)) {
						result = data::AddressType::NONE;
					}
					return false; //break iterateBlocks
				}
				// result
				if (data::AddressType::NONE == result) {
					return true;
				}
				return false;
			});
			return result;
		}

		std::shared_ptr<const TransactionEntry> FileBased::getTransactionForId(uint64_t transactionId) const
		{
			std::lock_guard _lock(mWorkMutex);
			auto blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			do {
				auto& block = getBlock(blockNr);
				if (block.getBlockIndex().hasTransactionNr(transactionId)) {
					return block.getTransaction(transactionId, *g_appContext);
				}
				blockNr--;
			} while (blockNr > 0);
			return nullptr;
		}

		ConstConfirmedTxPtr FileBased::getConfirmedTxForId(uint64_t transactionId) const
		{
			std::lock_guard _lock(mWorkMutex);
			auto blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			do {
				auto& block = getBlock(blockNr);
				if (block.getBlockIndex().hasTransactionNr(transactionId)) {
					return block.getCompactTransaction(transactionId, *g_appContext);
				}
				blockNr--;
			} while (blockNr > 0);
			return nullptr;
		}

		std::shared_ptr<const TransactionEntry> FileBased::findByLedgerAnchor(
			const data::LedgerAnchor& ledgerAnchor,
			const Filter& filter/* = Filter::ALL_TRANSACTIONS*/
		) const
		{
			auto transactionNr = mLedgerAnchorCache.has(ledgerAnchor);
			if (transactionNr) {
				return getTransactionForId(transactionNr);
			}
			return Abstract::findByLedgerAnchor(ledgerAnchor, filter);
		}

		AbstractProvider* FileBased::getProvider() const
		{
			return FileBasedProvider::getInstance();
		}


		void FileBased::loadStateFromBlockCache()
		{
			Profiler timeUsed;
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_ADDRESS_INDEX, mPublicKeysIndex.getLastIndex());
			auto lastBlockNr = model::files::Block::findLastBlockFileInFolder(mFolderPath);
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_BLOCK_NR, lastBlockNr);
			auto& block = getBlock(lastBlockNr);
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_TRANSACTION_ID, block.getBlockIndex().getMaxTransactionNr());
			LOG_F(INFO, "timeUsed: %s", timeUsed.string().data());
		}

		// TODO: look for a way of reusing logic from interaction::confirmTransaction, with nearly the same code in the roles
		void FileBased::rescanForTransactionTriggerEvents()
		{
			mTransactionTriggerEventsCache.reset();
			Filter f = Filter::ALL_TRANSACTIONS;
			f.searchDirection = SearchDirection::ASC;
			f.filterFunction = [this](const TransactionEntry& entry) -> FilterResult {
				if (entry.getTransactionType() == data::TransactionType::DEFERRED_TRANSFER) {
					auto confirmedTransaction = entry.getConfirmedTransaction();
					auto body = entry.getTransactionBody();
					Timepoint targetDate = confirmedTransaction->getConfirmedAt().getAsTimepoint()
						+ body->getDeferredTransfer()->getTimeoutDuration().getAsDuration();

					mTransactionTriggerEventsCache.addTransactionTriggerEvent(std::make_shared<data::TransactionTriggerEvent>(
						confirmedTransaction->getId(),
						targetDate,
						data::TransactionTriggerEventType::DEFERRED_TIMEOUT_REVERSAL
					));
				}
				else if (entry.getTransactionType() == data::TransactionType::REDEEM_DEFERRED_TRANSFER) {
					// remove timeout transaction trigger event
					auto confirmedTransaction = entry.getConfirmedTransaction();
					auto body = entry.getTransactionBody();
					assert(body->isRedeemDeferredTransfer());
					auto redeemDeferredTransfer = body->getRedeemDeferredTransfer();
					auto deferredTransferId = redeemDeferredTransfer->getDeferredTransferTransactionNr();
					auto deferredTransferEntry = getTransactionForId(deferredTransferId);
					assert(deferredTransferEntry->isDeferredTransfer());
					auto deferredTransfer = deferredTransferEntry->getTransactionBody()->getDeferredTransfer();

					auto transactionTriggerEventTargetDate =
						confirmedTransaction->getConfirmedAt().getAsTimepoint()
						+ deferredTransfer->getTimeoutDuration().getAsDuration()
						;

					mTransactionTriggerEventsCache.removeTransactionTriggerEvent(data::TransactionTriggerEvent(
						deferredTransferId,
						transactionTriggerEventTargetDate,
						data::TransactionTriggerEventType::DEFERRED_TIMEOUT_REVERSAL
					));
				}
				return FilterResult::DISMISS;
			};
			findAll(f);
		}

		void FileBased::iterateBlocks(const SearchDirection& searchDir, std::function<bool(const cache::Block&)> func) const
		{
			bool orderDesc = searchDir == SearchDirection::DESC;
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

		bool FileBased::validateLastTransactions(uint64_t countToValidate)
		{
			// load first GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE transaction into cache and validate the transaction to check file integrity
			Profiler timeUsed;
			Profiler timeSinceLastPrint;
			data::ConstConfirmedTransactionPtr previousConfirmedTransaction = nullptr;
			auto lastTransaction = findOne(Filter::LAST_TRANSACTION);
			if (!lastTransaction) {
				// seems we have nothing todo here
				LOG_F(WARNING, "startValidationTransactions called on empty blockchain");
				return true;
			}
			int count = 0;
			Filter f;
			f.searchDirection = SearchDirection::ASC;
			int countTarget = countToValidate;
			if (countToValidate) {
				f.minTransactionNr = lastTransaction->getTransactionNr() - countToValidate;
			}
			else {
				countTarget = lastTransaction->getTransactionNr();
			}

			f.filterFunction =
				[&](const TransactionEntry& transactionEntry) -> FilterResult
				{
					auto transactionBody = transactionEntry.getTransactionBody();
					validate::Context validator(*transactionEntry.getConfirmedTransaction());
					validate::Type validationLevel = validate::Type::SINGLE | validate::Type::ACCOUNT;
					if (transactionBody->getType() != data::CrossGroupType::LOCAL) {
						validationLevel = validationLevel | validate::Type::PAIRED;
					}
					if (previousConfirmedTransaction) {
						validationLevel = validationLevel | validate::Type::PREVIOUS;
						validator.setSenderPreviousConfirmedTransaction(previousConfirmedTransaction);
					}
					validator.disableVerify();
					validator.run(validationLevel, getptr());
					if (transactionEntry.getTransactionNr() > (lastTransaction->getTransactionNr() - GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE)) {
						mTransactionHashCache.push(*transactionEntry.getConfirmedTransaction());
					}
					previousConfirmedTransaction = transactionEntry.getConfirmedTransaction();
					count++;
					/*if (timeSinceLastPrint.millis() > 150) {
						printf("\r%.2f%%", ((double)count / (double)countTarget) * 100.0);
						timeSinceLastPrint.reset();
					}*/
					return FilterResult::DISMISS;
				};
			findAll(f);
			// printf("\r");
			f.filterFunction = nullptr;
			Profiler batchVerifyTime;
			auto invalidSignatures = verifySignatures(f, mCommunityId, ThreadingPolicy::ThreeQuarter);
			LOG_F(INFO, "time used for loading and validating last: %d transactions: %s (%s for batch verify)",
				count,
				timeUsed.string().c_str(),
				batchVerifyTime.string().c_str()
			);
			if (!invalidSignatures.empty()) {
				throw GradidoNodeInvalidDataException("verify from at least one transaction failed");
			}
			return true;
		}
	}
}