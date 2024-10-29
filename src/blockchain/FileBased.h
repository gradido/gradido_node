#ifndef __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_H
#define __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_H

#include "../cache/Block.h"
#include "../cache/Dictionary.h"
#include "../cache/DeferredTransfer.h"
#include "../cache/MessageId.h"
#include "../cache/State.h"
#include "../cache/TransactionHash.h"
#include "../client/Base.h"
#include "../controller/TaskObserver.h"
#include "../iota/MessageListener.h"

#include "gradido_blockchain/blockchain/Abstract.h"
#include "gradido_blockchain/lib/AccessExpireCache.h"

//! how many transactions will be readed from disk on blockchain startup and put into cache for preventing doublettes
//! iota stores up to 1000 transactions
#define GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE 1500

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
		class FileBased : public Abstract, public std::enable_shared_from_this<FileBased>
		{
			struct Private { explicit Private() = default; };
		public:
			// Constructor is only usable by this class
			FileBased(Private, std::string_view communityId, std::string_view alias, std::string_view folder);
			// make sure that all shared_ptr from FileBased Blockchain know each other
			static inline std::shared_ptr<FileBased> create(std::string_view communityId, std::string_view alias, std::string_view folder);
			inline std::shared_ptr<FileBased> getptr();
			inline std::shared_ptr<const FileBased> getptr() const;

			virtual ~FileBased();

			//! load blockchain from files and check if index and states seems ok
			//! if address index or block index is corrupt, remove address index and block index files, they will rebuild automatic on get block calls
			//! if state leveldb file is corrupt, reconstruct values from block cache
			//! \param resetBlockIndices will be set to true if community id index was corrupted which invalidate block index
			//! \return true on success, else false
			bool init(bool resetBlockIndices);

			// clean up group, stopp all running processes
			void exit();

			// TODO: make a interaction from it
			//! validate and generate confirmed transaction
			//! throw if gradido transaction isn't valid
			//! \return false if transaction already exist
			virtual bool addGradidoTransaction(data::ConstGradidoTransactionPtr gradidoTransaction, memory::ConstBlockPtr messageId, Timepoint confirmedAt);

			//! main search function, do all the work, reference from other functions
			virtual TransactionEntries findAll(const Filter& filter) const;

			//! use only index for searching, ignore filter function
			//! \return vector with transaction nrs
			std::vector<uint64_t> findAllFast(const Filter& filter) const;

			//! count results for a specific filter, using only the index, ignore filter function 
			size_t findAllResultCount(const Filter& filter) const;

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

			virtual std::shared_ptr<const TransactionEntry> getTransactionForId(uint64_t transactionId) const;
			virtual std::shared_ptr<const TransactionEntry> findByMessageId(
				memory::ConstBlockPtr messageId,
				const Filter& filter = Filter::ALL_TRANSACTIONS
			) const;
			virtual AbstractProvider* getProvider() const;

			inline void setListeningCommunityServer(std::shared_ptr<client::Base> client);
			inline std::shared_ptr<client::Base> getListeningCommunityServer() const;

			inline uint32_t getOrAddIndexForPublicKey(memory::ConstBlockPtr publicKey) const {
				return mPublicKeysIndex->getOrAddIndexForString(publicKey->copyAsString());
			}
			inline const std::string& getFolderPath() const { return mFolderPath; }
			inline const std::string& getAlias() const { return mAlias; }
			inline TaskObserver& getTaskObserver() const { return *mTaskObserver; }

			inline bool isTransactionAlreadyExist(const gradido::data::GradidoTransaction& transaction) const {
				return mTransactionHashCache.has(transaction);
			}

			// check transactions in deferred transfer cache for timeout and remove all timeouted deferred transfer transactions
			void cleanupDeferredTransferCache();

		protected:
			//! if state leveldb was invalid, recover values from block cache
			void loadStateFromBlockCache();
			//! scan blockchain for deferred transfer transaction which are not redeemed completly
			void rescanForDeferredTransfers();

			//! \param func if function return false, stop iteration
			void iterateBlocks(const Filter& filter, std::function<bool(const cache::Block&)> func) const;

			cache::Block& getBlock(uint32_t blockNr) const;

			mutable std::recursive_mutex mWorkMutex;
			bool mExitCalled;
			std::string mFolderPath;
			std::string mAlias;

			//! observe write to file tasks from block, mayber later more
			mutable std::shared_ptr<TaskObserver> mTaskObserver;
			//! connect via mqtt to iota server and get new messages
			iota::MessageListener* mIotaMessageListener;

			//! contain indices for every public key address, used overall for optimisation
			mutable std::shared_ptr<cache::Dictionary> mPublicKeysIndex;
			// level db to store state values like last transaction
			mutable cache::State mBlockchainState;

			mutable cache::DeferredTransfer mDeferredTransfersCache;

			mutable cache::MessageId mMessageIdsCache;

			mutable AccessExpireCache<uint32_t, std::shared_ptr<cache::Block>> mCachedBlocks;

			cache::TransactionHash mTransactionHashCache;

			//! Community Server listening on new blocks for his group
			//! TODO: replace with more abstract but simple event system and/or mqtt
			std::shared_ptr<client::Base> mCommunityServer;
		};

		std::shared_ptr<FileBased> FileBased::create(std::string_view communityId, std::string_view alias, std::string_view folder) 
		{
			return std::make_shared<FileBased>(Private(), communityId, alias, folder);
		}

		std::shared_ptr<FileBased> FileBased::getptr()
		{
			return shared_from_this();
		}

		std::shared_ptr<const FileBased> FileBased::getptr() const
		{
			return shared_from_this();
		}

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