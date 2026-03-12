#ifndef __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_H
#define __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_H

#include "../cache/Block.h"
#include "../cache/LedgerAnchor.h"
#include "../cache/State.h"
#include "../cache/TransactionHash.h"
#include "../cache/TransactionTriggerEvent.h"
#include "../client/Base.h"
#include "../controller/TaskObserver.h"
#include "../controller/SimpleOrderingManager.h"
#include "../lib/PersistentDictionary.h"

#include "gradido_blockchain/blockchain/Abstract.h"
#include "gradido_blockchain/crypto/ByteArray.h"
#include "gradido_blockchain/data/hiero/TopicId.h"
#include "gradido_blockchain/lib/AccessExpireCache.h"

//! how many transactions will be readed from disk on blockchain startup and put into cache for preventing doublettes
//! iota stores up to 1000 transactions
#define GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE 1500
//! should be enough to keep all 3 state values in memory all the time
#define GRADIDO_NODE_MAGIC_NUMBER_BLOCKCHAIN_STATE_CACHE_SIZE_BYTES 192
//! TODO: Test and Profile different values, or create dynamic algorithmus
#define GRADIDO_NODE_MAGIC_NUMBER_IOTA_MESSAGE_ID_CACHE_MEGA_BYTES 10
#define GRADIDO_NODE_MAGIC_NUMBER_PUBLIC_KEYS_INDEX_CACHE_MEGA_BTYES 10
#define GRADIDO_NODE_MAGIC_NUMBER_COMMUNITY_INDEX_CACHE_BYTES 400
#define GRADIDO_NODE_MAGIC_NUMBER_TRANSACTION_TRIGGER_EVENTS_CACHE_MEGA_BTYES 1

#include <mutex>
#include <stop_token>

namespace client {
	namespace hiero {
		class ConsensusClient;
	}
}

namespace hiero {
	class MessageListenerQuery;
}

namespace controller {
	class SimpleOrderingManager;
}

namespace task {
	class SyncTopic;
}

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
			FileBased(
				Private,
				std::stop_token stopToken,
				const std::string& communityId,
				const hiero::TopicId& topicId,
				std::string_view alias,
				std::string_view folder,
				std::vector<std::shared_ptr<client::hiero::ConsensusClient>>&& hieroClients
			);
			// make sure that all shared_ptr from FileBased Blockchain know each other
			static inline std::shared_ptr<FileBased> create(
				std::stop_token stopToken,
				const std::string& communityId,
				const hiero::TopicId& topicId,
				std::string_view alias,
				std::string_view folder,
				std::vector<std::shared_ptr<client::hiero::ConsensusClient>>&& hieroClients
			);
			// construct without valid HieroTopic Id. Make Blockchain txs available, but don't listen for new ones from hiero/hedera
			static inline std::shared_ptr<FileBased> createWithoutHieroTopic(
				std::stop_token stopToken,
				const std::string& communityId,
				std::string_view alias,
				std::string_view folder
			);
			inline std::shared_ptr<FileBased> getptr();
			inline std::shared_ptr<const FileBased> getptr() const;

			virtual ~FileBased();

			//! init 1
			//! load blockchain from files and check if index and states seems ok
			//! if address index or block index is corrupt, remove address index and block index files, they will rebuild automatic on get block calls
			//! if state leveldb file is corrupt, reconstruct values from block cache
			//! \param resetBlockIndices will be set to true if community id index was corrupted which invalidate block index
			//! \return true on success, else false
			bool init(bool resetBlockIndices);

			//! init 2
			//! check last GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE if they are valid
			//! should be called, after all communities where initalized, because it would need other communities for cross group transaction validation
			bool startValidationTransactions();

			//! init 3
			//! prepare task for syncronize with hiero topic
			//! all SyncTopic for all communities should be started/scheduled at the same time because there could need each other for new cross group transactions
			std::shared_ptr<task::SyncTopic> getTopicSyncTask();

			//! init 4
			//! start listening to topic, will be called from SyncTopic at the end, will update last known TopicId 
			void startListening(data::Timestamp lastTransactionConfirmedAt);

			// clean up group, stopp all running processes
			void exit();

			// TODO: make a interaction from it
			//! validate and generate confirmed transaction
			//! throw if gradido transaction isn't valid
			//! \return false if transaction already exist
			virtual bool createAndAddConfirmedTransaction(
				data::ConstGradidoTransactionPtr gradidoTransaction,
				const data::LedgerAnchor& ledgerAnchor,
				data::Timestamp confirmedAt
			) override;

			virtual bool createAndAddConfirmedTransactionExtern(
				data::ConstGradidoTransactionPtr gradidoTransaction,
				const data::LedgerAnchor& ledgerAnchor,
				std::vector<data::AccountBalance> accountBalances
			) override;

			void updateLastKnownSequenceNumber(uint64_t newSequenceNumber);
			virtual void addTransactionTriggerEvent(std::shared_ptr<const data::TransactionTriggerEvent> transactionTriggerEvent) override;
			virtual void removeTransactionTriggerEvent(const data::TransactionTriggerEvent& transactionTriggerEvent) override;

			virtual bool isTransactionExist(data::ConstGradidoTransactionPtr gradidoTransaction, data::Timestamp confirmedAt) const override {
				return mTransactionHashCache.has(*gradidoTransaction);
			}

			//! return events in asc order of targetDate
			virtual std::vector<std::shared_ptr<const data::TransactionTriggerEvent>> findTransactionTriggerEventsInRange(data::Timestamp startDate, data::Timestamp endDate) override;
			virtual std::shared_ptr<const data::TransactionTriggerEvent> findNextTransactionTriggerEventInRange(data::Timestamp startDate, data::Timestamp endDate) override;

			//! main search function, do all the work, reference from other functions
			virtual TransactionEntries findAll(const Filter& filter) const override;

			virtual data::compact::ConfirmedTxs findAll(const CompactFilter& filter) const override;

			// find all optimized for counting transaction nrs, better not use the filter.function for that, because this would slow down
			virtual size_t countAll(const Filter& filter = Filter::ALL_TRANSACTIONS) const override;
			virtual size_t countAll(const CompactFilter& filter) const override;

			//! use only index for searching, ignore filter function
			//! \return vector with transaction nrs
			std::vector<uint64_t> findAllFast(const Filter& filter) const;

			virtual data::AddressType getAddressType(const Filter& filter = Filter::LAST_TRANSACTION) const override;

			virtual std::shared_ptr<const TransactionEntry> getTransactionForId(uint64_t transactionId) const override;
			virtual data::compact::ConstConfirmedTxPtr getConfirmedTxForId(uint64_t transactionId) const override;
			//! \param filter use to speed up search if infos exist to narrow down search transactions range
			virtual ConstTransactionEntryPtr findByLedgerAnchor(
				const data::LedgerAnchor& ledgerAnchor,
				const Filter& filter = Filter::ALL_TRANSACTIONS
			) const override;
			virtual AbstractProvider* getProvider() const override;

			inline void setListeningCommunityServer(std::shared_ptr<client::Base> client);
			inline std::shared_ptr<client::Base> getListeningCommunityServer() const;

			inline virtual const IDictionary<PublicKey>& getPublicKeyDictionary() const override { return mPublicKeysIndex; }
			inline virtual uint32_t getOrAddPublicKey(const PublicKey& publicKey) override {
				return mPublicKeysIndex.getOrAddIndexForData(publicKey);
			}

			inline uint32_t getOrAddIndexForPublicKey(const PublicKey& publicKey) const {
				return mPublicKeysIndex.getOrAddIndexForData(publicKey);
			}

			inline const hiero::TopicId& getHieroTopicId() const { return mHieroTopicId; }
			inline const std::string& getAlias() const { return mAlias; }
			inline const std::string& getFolderPath() const { return mFolderPath; }
			inline const std::string& getCommunityId() const { return mCommunityId; }
			inline TaskObserver& getTaskObserver() const { return *mTaskObserver; }
			inline std::shared_ptr<client::hiero::ConsensusClient> pickHieroClient() const { return mHieroClients.size() ?  mHieroClients[std::rand() % mHieroClients.size()] : nullptr; }
			std::shared_ptr<controller::SimpleOrderingManager> getOrderingManager() { return mOrderingManager; }
			std::stop_token getStopToken() const { return mStopToken; }

		protected:
			//! if state leveldb was invalid, recover values from block cache
			void loadStateFromBlockCache();

			//! is transaction trigger event cache was invalid, rescan entire blockchain
			void rescanForTransactionTriggerEvents();

			//! \param func if function return false, stop iteration
			//! TODO: make a template function without using std::function
			void iterateBlocks(const SearchDirection& searchDir, std::function<bool(const cache::Block&)> func) const;

			cache::Block& getBlock(uint32_t blockNr) const;

			//! \param countToValidate lastTransaction.nr - countToValidate = minTransaction.nr, 0 for all
			bool validateLastTransactions(uint64_t countToValidate);

			mutable std::recursive_mutex mWorkMutex;
			std::stop_token mStopToken;
			hiero::TopicId mHieroTopicId;
			std::string mAlias;
			std::string mFolderPath;
			std::string mCommunityId;

			//! observe write to file tasks from block, mayber later more
			mutable std::shared_ptr<TaskObserver> mTaskObserver;
			std::shared_ptr<controller::SimpleOrderingManager> mOrderingManager;
			//! connect via mqtt to iota server and get new messages
			//iota::MessageListener* mIotaMessageListener;
			std::shared_ptr<hiero::MessageListenerQuery> mHieroMessageListener;

			//! contain indices for every public key address, used overall for optimisation
			mutable PersistentDictionary<PublicKey, PublicKeyHash, PublicKeyEqual> mPublicKeysIndex;
			// level db to store state values like last transaction
			// TODO: speedup with atcual struct, write out into leveldb/lmdb only on changes, maybe even buffered, think on exit management
			mutable cache::State mBlockchainState;

			mutable cache::LedgerAnchor mLedgerAnchorCache;

			cache::TransactionTriggerEvent mTransactionTriggerEventsCache;

			mutable AccessExpireCache<uint32_t, std::shared_ptr<cache::Block>> mCachedBlocks;

			cache::TransactionHash mTransactionHashCache;

			//! Community Server listening on new blocks for his group
			//! TODO: replace with more abstract but simple event system and/or mqtt
			std::shared_ptr<client::Base> mCommunityServer;
			std::vector<std::shared_ptr<client::hiero::ConsensusClient>> mHieroClients;
		};

		std::shared_ptr<FileBased> FileBased::create(
			std::stop_token stopToken,
			const std::string& communityId,
			const hiero::TopicId& topicId,
			std::string_view alias,
			std::string_view folder,
			std::vector<std::shared_ptr<client::hiero::ConsensusClient>>&& hieroClients
		) {
			return std::make_shared<FileBased>(Private(), stopToken, communityId, topicId, alias, folder, std::move(hieroClients));
		}

		std::shared_ptr<FileBased> FileBased::createWithoutHieroTopic(
			std::stop_token stopToken,
			const std::string& communityId,
			std::string_view alias,
			std::string_view folder
		) {
			return std::make_shared<FileBased>(
				Private(),
				stopToken,
				communityId,
				hiero::TopicId(),
				alias,
				folder,
				std::vector<std::shared_ptr<client::hiero::ConsensusClient>>()
			);
		}

		std::shared_ptr<FileBased> FileBased::getptr()
		{
			if (mStopToken.stop_requested()) return nullptr;
			return shared_from_this();
		}

		std::shared_ptr<const FileBased> FileBased::getptr() const
		{
			if (mStopToken.stop_requested()) return nullptr;
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