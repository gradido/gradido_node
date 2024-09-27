#include "FileBased.h"
#include "FileBasedProvider.h"
#include "gradido_blockchain/lib/Profiler.h"

#include "loguru/loguru.hpp"

using namespace controller;

namespace gradido {
	namespace blockchain {
		FileBased::FileBased(std::string_view communityId, std::string_view folder)
			: Abstract(communityId),
			mExitCalled(false),
			mFolderPath(folder),
			mIotaMessageListener(new iota::MessageListener(communityId)),
			mAddressIndex(folder, this),
			mBlockchainState(std::string(folder).append(".state"))
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
			std::lock_guard _lock(mWorkMutex);
			if (!mAddressIndex.init()) {
				// remove index files for regenration
				LOG_F(WARNING, "something went wrong with the index files");
				// mCachedBlocks.clear();
				mAddressIndex.reset();
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

		void FileBased::setListeningCommunityServer(std::shared_ptr<client::Base> client)
		{
			std::lock_guard _lock(mWorkMutex);
			mCommunityServer = client;
		}


		void FileBased::pushTransactionEntry(std::shared_ptr<TransactionEntry> transactionEntry)
		{

		}

		void FileBased::loadStateFromBlockCache()
		{

		}
	}
}