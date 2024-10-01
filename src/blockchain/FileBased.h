#ifndef __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_H
#define __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_H

#include "../cache/Dictionary.h"
#include "../cache/DeferredTransfer.h"
#include "../cache/State.h"


#include "gradido_blockchain/blockchain/Abstract.h"
#include "../client/Base.h"
#include "../controller/TaskObserver.h"
#include "../iota/MessageListener.h"

#include <mutex>

namespace gradido {
	namespace blockchain {
		/*
		* @author einhornimmond
		* @date 18.08.2025
		* @brief File based blockchain, inspired from bitcoin
		* Transaction Data a stored as serialized protobufObject ConfirmedTransaction
		* Additional there will be a index file listen the file start positions
		* per transaction id and further data for filterning bevor need to load whole transaction
		*/
		class FileBased : public Abstract
		{
		public:
			FileBased(std::string_view communityId, std::string_view folder);
			virtual ~FileBased();

			//! load blockchain from files and check if index and states seems ok
			//! if address index or block index is corrupt, remove address index and block index files, they will rebuild automatic on get block calls
			//! if state leveldb file is corrupt, reconstruct values from block cache
			//! \param resetBlockIndices will be set to true if community id index was corrupted which invalidate block index
			//! \return true on success, else false
			bool init(bool resetBlockIndices);

			// clean up group, stopp all running processes
			void exit();

			//! validate and generate confirmed transaction
			//! throw if gradido transaction isn't valid
			//! \return false if transaction already exist
			virtual bool addGradidoTransaction(data::ConstGradidoTransactionPtr gradidoTransaction, memory::ConstBlockPtr messageId, Timepoint confirmedAt);

			// main search function, do all the work, reference from other functions
			virtual TransactionEntries findAll(const Filter& filter = Filter::ALL_TRANSACTIONS) const;

			//! find all deferred transfers which have the timeout in date range between start and end, have senderPublicKey and are not redeemed,
			//! therefore boocked back to sender
			//! find all deferred transfers which have the timeout in date range between start and end, have senderPublicKey and are not redeemed,
			//! therefore boocked back to sender
			virtual TransactionEntries findTimeoutedDeferredTransfersInRange(
				memory::ConstBlockPtr senderPublicKey,
				TimepointInterval timepointInterval,
				uint64_t maxTransactionNr
			) const;

			//! find all transfers which redeem a deferred transfer in date range
			//! \param senderPublicKey sender public key of sending account of deferred transaction
			//! \return list with transaction pairs, first is deferred transfer, second is redeeming transfer
			virtual std::list<DeferredRedeemedTransferPair> findRedeemedDeferredTransfersInRange(
				memory::ConstBlockPtr senderPublicKey,
				TimepointInterval timepointInterval,
				uint64_t maxTransactionNr
			) const;

			virtual std::shared_ptr<TransactionEntry> getTransactionForId(uint64_t transactionId) const;
			virtual AbstractProvider* getProvider() const;

			inline void setListeningCommunityServer(std::shared_ptr<client::Base> client);
			inline std::shared_ptr<client::Base> getListeningCommunityServer() const;

		protected:
			virtual void pushTransactionEntry(std::shared_ptr<TransactionEntry> transactionEntry);
			//! if state leveldb was invalid, recover values from block cache
			void loadStateFromBlockCache();
			//! scan blockchain for deferred transfer transaction which are not redeemed completly
			void rescanForDeferredTransfers();

			mutable std::recursive_mutex mWorkMutex;
			bool mExitCalled;
			std::string mFolderPath;


			//! observe write to file tasks from block, mayber later more
			TaskObserver mTaskObserver;
			//! connect via mqtt to iota server and get new messages
			iota::MessageListener* mIotaMessageListener;

			cache::Dictionary mPublicKeysIndex;
			// level db to store state values like last transaction
			cache::State mBlockchainState;

			cache::DeferredTransfer mDeferredTransfersCache;

			//! Community Server listening on new blocks for his group
			//! TODO: replace with more abstract but simple event system
			std::shared_ptr<client::Base> mCommunityServer;
		};

		void FileBased::setListeningCommunityServer(std::shared_ptr<client::Base> client)
		{
			std::lock_guard _lock(mWorkMutex);
			mCommunityServer = client;
		}

		std::shared_ptr<client::Base> FileBased::getListeningCommunityServer() const 
		{
			std::lock_guard<std::recursive_mutex> _lock(mWorkMutex); 
			return mCommunityServer;
		}
	}
}


#endif //__GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_H