#include "FileBased.h"
#include "FileBasedProvider.h"
#include "NodeTransactionEntry.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/const.h"

#include "loguru/loguru.hpp"

#include <set>

using namespace controller;

namespace gradido {
	namespace blockchain {
		FileBased::FileBased(std::string_view communityId, std::string_view folder)
			: Abstract(communityId),
			mExitCalled(false),
			mFolderPath(folder),
			mIotaMessageListener(new iota::MessageListener(communityId)),
			mAddressIndex(folder, this),
			mBlockchainState(std::string(folder).append(".state")),
			mDeferredTransfersCache(std::string(folder).append("/deferredTransferCache"))
		{
		}

		FileBased::~FileBased()
		{
			if (mIotaMessageListener) {
				delete mIotaMessageListener;
				mIotaMessageListener = nullptr;
			}
		}
		bool FileBased::init()
		{
			assert(!mExitCalled);
			std::lock_guard _lock(mWorkMutex);
			bool resetDeferredTransfersCache = false;
			if (!mAddressIndex.init()) {
				// remove index files for regenration
				LOG_F(WARNING, "something went wrong with the address index file");
				// mCachedBlocks.clear();
				mAddressIndex.reset();
				resetDeferredTransfersCache = true;
			}
			if (!resetDeferredTransfersCache && !mDeferredTransfersCache.init()) {
				resetDeferredTransfersCache = true;
			}
			if (resetDeferredTransfersCache) {
				LOG_F(WARNING, "something went wrong with deferred transfer cache level db");
				mDeferredTransfersCache.reset();
				rescanForDeferredTransfers();
			}
			if (!mBlockchainState.init()) {
				LOG_F(WARNING, "something went wrong with the state level db");
				mBlockchainState.reset();
				loadStateFromBlockCache();
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
			while (auto pendingTasksCount = mTaskObserver.getPendingTasksCount()) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
				if (mTaskObserver.getPendingTasksCount() >= pendingTasksCount && timeUsed.seconds() > 10) {
					break;
				}
			}
			mDeferredTransfersCache.exit();
			mBlockchainState.exit();
			mAddressIndex.exit();
		}
		bool FileBased::addGradidoTransaction(data::ConstGradidoTransactionPtr gradidoTransaction, memory::ConstBlockPtr messageId, Timepoint confirmedAt)
		{

		}
		TransactionEntries FileBased::findAll(const Filter& filter/* = Filter::ALL_TRANSACTIONS */) const
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
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_ADDRESS_INDEX, mAddressIndex.getLastIndex());
			auto lastTransaction = findOne(Filter::LAST_TRANSACTION);
			int32_t lastTransactionNr = 0;
			if (lastTransaction) {
				lastTransactionNr = lastTransaction->getTransactionNr();
			}
			mBlockchainState.updateState(cache::DefaultStateKeys::LAST_TRANSACTION_ID, lastTransactionNr);			
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
					auto recipientAddressIndex = mAddressIndex.getOrAddIndexForAddress(recipient->copyAsString());
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
	}
}