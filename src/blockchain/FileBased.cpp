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
#include "gradido_blockchain/blockchain/FilterBuilder.h"
#include "gradido_blockchain/interaction/calculateAccountBalance/Context.h"
#include "gradido_blockchain/interaction/confirmTransaction/Context.h"
#include "gradido_blockchain/interaction/validate/Context.h"
#include "gradido_blockchain/lib/Profiler.h"

#include "loguru/loguru.hpp"

#include <set>
#include <chrono>

using namespace cache;

namespace gradido {
	using namespace interaction;
	namespace blockchain {
		FileBased::FileBased(
			Private,
			std::string_view communityId,
			const hiero::TopicId& topicId,
			std::string_view alias,
			std::string_view folder,
			std::vector<std::shared_ptr<client::hiero::ConsensusClient>>&& hieroClients)
			: Abstract(communityId),
			mExitCalled(false),
			mHieroTopicId(topicId),
			mAlias(alias),
			mFolderPath(folder),			
			mTaskObserver(std::make_shared<TaskObserver>()),
			mOrderingManager(std::make_shared<controller::SimpleOrderingManager>(communityId)),
			// mIotaMessageListener(new iota::MessageListener(communityId, alias)),
			mPublicKeysIndex(std::make_shared<Dictionary>(std::string(folder).append("/pubkeysCache"))),
			mBlockchainState(std::string(folder).append("/.state")),
			mMessageIdsCache(std::string(folder).append("/messageIdCache")),
			mTransactionTriggerEventsCache(std::string(folder).append("/transactionTriggerEventCache")),
			mCachedBlocks(ServerGlobals::g_CacheTimeout),
			mTransactionHashCache(communityId),
			mHieroClients(std::move(hieroClients))
		{
			assert(mHieroClients.size());
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
			if (!mPublicKeysIndex->init(GRADIDO_NODE_MAGIC_NUMBER_PUBLIC_KEYS_INDEX_CACHE_MEGA_BTYES * 1024 * 1024)) {
				// remove index files for regenration
				LOG_F(WARNING, "reset the public key index file");
				// mCachedBlocks.clear();
				mPublicKeysIndex->reset();
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

			if (!mTransactionTriggerEventsCache.init(GRADIDO_NODE_MAGIC_NUMBER_TRANSACTION_TRIGGER_EVENTS_CACHE_MEGA_BTYES * 1024 * 1024)) {
				Profiler timeUsed;
				rescanForTransactionTriggerEvents();
				LOG_F(INFO, "rescan blockchain for transaction trigger events, time: %s", timeUsed.string().data());
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
					validator.run(validationLevel, getptr());
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
			return true;
		}

		std::shared_ptr<task::SyncTopicOnStartup> FileBased::initOnline()
		{
			return std::make_shared<task::SyncTopicOnStartup>(
				mBlockchainState.readInt64State(cache::DefaultStateKeys::LAST_HIERO_TOPIC_SEQUENCE_NUMBER, 0),
				hiero::TopicId(mBlockchainState.readState(cache::DefaultStateKeys::LAST_HIERO_TOPIC_ID, mHieroTopicId.toString())),
				getptr()
			);
		}

		void FileBased::startListening(data::Timestamp lastTransactionConfirmedAt)
		{			
			if (mHieroMessageListener) {
				LOG_F(WARNING, "called again, while listener where already existing");
			}
			data::Timestamp listenFrom = { lastTransactionConfirmedAt.getSeconds(), lastTransactionConfirmedAt.getNanos() + 1 };
			auto now = std::chrono::system_clock::now();
			// TODO: restart after connection was closed because of timeout
			auto endTime = now + std::chrono::duration(std::chrono::years(10));
			mHieroMessageListener = std::make_shared<hiero::MessageListenerQuery>(
				mHieroTopicId, 
				mCommunityId, 
				hiero::ConsensusTopicQuery( mHieroTopicId, listenFrom, endTime )
			);
			ServerGlobals::g_HieroMirrorNode->subscribeTopic(mHieroMessageListener);
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_HIERO_TOPIC_ID, mHieroTopicId.toString());
		}

		void FileBased::exit()
		{
			std::lock_guard _lock(mWorkMutex);
			mExitCalled = true;
			/*if (mIotaMessageListener) {
				delete mIotaMessageListener;
				mIotaMessageListener = nullptr;
			}*/
			mHieroMessageListener->cancelConnection();

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
			mPublicKeysIndex->exit();
			mMessageIdsCache.exit();
			mTransactionTriggerEventsCache.exit();
		}
		bool FileBased::createAndAddConfirmedTransaction(
			data::ConstGradidoTransactionPtr gradidoTransaction,
			memory::ConstBlockPtr messageId,
			data::Timestamp confirmedAt
		) {
			if (!gradidoTransaction) {
				throw GradidoNullPointerException("missing transaction", "GradidoTransactionPtr", __FUNCTION__);
			}
			if (!messageId) {
				throw GradidoNullPointerException("missing messageId", "memory::ConstBlockPtr", __FUNCTION__);
			}
			std::lock_guard _lock(mWorkMutex);
			if (mExitCalled) { return false;}
			confirmTransaction::Context confirmTransactionContext(getptr());
			auto role = confirmTransactionContext.createRole(gradidoTransaction, messageId, confirmedAt);
			auto confirmedTransaction = confirmTransactionContext.run(role);
			// will occure if transaction already exist
			if (!confirmedTransaction) {
				return false;
			}
			auto blockNr = mBlockchainState.readInt32State(cache::DefaultStateKeys::LAST_BLOCK_NR, 1);
			auto& block = getBlock(blockNr);
			auto nodeTransactionEntry = std::make_shared<NodeTransactionEntry>(confirmedTransaction, getptr());
			if (!block.pushTransaction(nodeTransactionEntry)) {
				// block was already stopped, so we can  stop here also 
				LOG_F(WARNING, "couldn't push transaction: %llu to block: %d", confirmedTransaction->getId(), blockNr);
				return false;
			}
			role->runPastAddToBlockchain(confirmedTransaction, getptr());
			mBlockchainState.updateState(DefaultStateKeys::LAST_TRANSACTION_ID, confirmedTransaction->getId());
			mBlockchainState.updateState(DefaultStateKeys::LAST_ADDRESS_INDEX, mPublicKeysIndex->getLastIndex());
			mTransactionHashCache.push(*nodeTransactionEntry->getConfirmedTransaction());
			mMessageIdsCache.add(confirmedTransaction->getMessageId(), confirmedTransaction->getId());
			// add public keys to index
			auto involvedAddresses = confirmedTransaction->getInvolvedAddresses();
			for (const auto& address : involvedAddresses) {
				mPublicKeysIndex->getOrAddIndexForString(address->copyAsString());
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

		std::vector<std::shared_ptr<const data::TransactionTriggerEvent>> FileBased::findTransactionTriggerEventsInRange(TimepointInterval range)
		{
			std::lock_guard _lock(mWorkMutex);
			return mTransactionTriggerEventsCache.findTransactionTriggerEventsInRange(range);
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