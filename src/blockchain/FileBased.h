#ifndef __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_H
#define __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_H

#include "../cache/Block.h"
#include "../cache/Dictionary.h"
#include "../cache/HieroTransactionId.h"
#include "../cache/State.h"
#include "../cache/TransactionHash.h"
#include "../cache/TransactionTriggerEvent.h"
#include "../client/Base.h"
#include "../controller/TaskObserver.h"
#include "../controller/SimpleOrderingManager.h"

#include "gradido_blockchain/blockchain/Abstract.h"
#include "gradido_blockchain/data/hiero/TopicId.h"
#include "gradido_blockchain/lib/AccessExpireCache.h"

//! how many transactions will be readed from disk on blockchain startup and put into cache for preventing doublettes
//! iota stores up to 1000 transactions
#define GRADIDO_NODE_MAGIC_NUMBER_STARTUP_TRANSACTIONS_CACHE_SIZE 1500
//! should be enough to keep all 3 state values in memory all the time
#define GRADIDO_NODE_MAGIC_NUMBER_BLOCKCHAIN_STATE_CACHE_SIZE_BYTES 192
//! TODO: Test and Profile different values, or create dynamic algorithmus
#define GRADIDO_NODE_MAGIC_NUMBER_IOTA_MESSAGE_ID_CACHE_MEGA_BYTES 10
#define GRADIDO_NODE_MAGIC_NUMBER_PUBLIC_KEYS_INDEX_CACHE_MEGA_BTYES 1
#define GRADIDO_NODE_MAGIC_NUMBER_TRANSACTION_TRIGGER_EVENTS_CACHE_MEGA_BTYES 1

#include <mutex>

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
	class SyncTopicOnStartup;
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
				std::string_view communityId,
				const hiero::TopicId& topicId,
				std::string_view alias, 
				std::string_view folder,
				std::vector<std::shared_ptr<client::hiero::ConsensusClient>>&& hieroClients
			);
			// make sure that all shared_ptr from FileBased Blockchain know each other
			static inline std::shared_ptr<FileBased> create(
				std::string_view communityId,
				const hiero::TopicId& topicId,
				std::string_view alias,
				std::string_view folder,
				std::vector<std::shared_ptr<client::hiero::ConsensusClient>>&& hieroClients
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
			//! prepare task for syncronize with hiero topic
			//! all SyncTopicOnStartup for all communities should be started/scheduled at the same time because there could need each other for new cross group transactions
			std::shared_ptr<task::SyncTopicOnStartup> initOnline();

			//! init 3
			//! start listening to topic, will be called from SyncTopicOnStartup at the end, will update last known TopicId 
			void startListening(data::Timestamp lastTransactionConfirmedAt);

			// clean up group, stopp all running processes
			void exit();

			// TODO: make a interaction from it
			//! validate and generate confirmed transaction
			//! throw if gradido transaction isn't valid
			//! \return false if transaction already exist
			virtual bool createAndAddConfirmedTransaction(
				data::ConstGradidoTransactionPtr gradidoTransaction,
				memory::ConstBlockPtr messageId,
				data::Timestamp confirmedAt
		 	) override;
			void updateLastKnownSequenceNumber(uint64_t newSequenceNumber);
			virtual void addTransactionTriggerEvent(std::shared_ptr<const data::TransactionTriggerEvent> transactionTriggerEvent);
			virtual void removeTransactionTriggerEvent(const data::TransactionTriggerEvent& transactionTriggerEvent);

			virtual bool isTransactionExist(data::ConstGradidoTransactionPtr gradidoTransaction) const {
				return mTransactionHashCache.has(*gradidoTransaction);
			}

			//! return events in asc order of targetDate
			virtual std::vector<std::shared_ptr<const data::TransactionTriggerEvent>> findTransactionTriggerEventsInRange(TimepointInterval range);

			//! main search function, do all the work, reference from other functions
			virtual TransactionEntries findAll(const Filter& filter) const;

			//! use only index for searching, ignore filter function
			//! \return vector with transaction nrs
			std::vector<uint64_t> findAllFast(const Filter& filter) const;

			//! count results for a specific filter, using only the index, ignore filter function 
			size_t findAllResultCount(const Filter& filter) const;

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
			inline const hiero::TopicId& getHieroTopicId() const { return mHieroTopicId; }
			inline const std::string& getFolderPath() const { return mFolderPath; }
			inline const std::string& getAlias() const { return mAlias; }
			inline TaskObserver& getTaskObserver() const { return *mTaskObserver; }
			inline std::shared_ptr<client::hiero::ConsensusClient> pickHieroClient() const { return mHieroClients[std::rand() % mHieroClients.size()]; }
			std::shared_ptr<controller::SimpleOrderingManager> getOrderingManager() { return mOrderingManager; }

		protected:
			//! if state leveldb was invalid, recover values from block cache
			void loadStateFromBlockCache();

			//! is transaction trigger event cache was invalid, rescan entire blockchain
			void rescanForTransactionTriggerEvents();

			//! \param func if function return false, stop iteration
			void iterateBlocks(const SearchDirection& searchDir, std::function<bool(const cache::Block&)> func) const;

			cache::Block& getBlock(uint32_t blockNr) const;

			mutable std::recursive_mutex mWorkMutex;
			bool mExitCalled;
			hiero::TopicId mHieroTopicId;
			std::string mAlias;
			std::string mFolderPath;

			//! observe write to file tasks from block, mayber later more
			mutable std::shared_ptr<TaskObserver> mTaskObserver;
			std::shared_ptr<controller::SimpleOrderingManager> mOrderingManager;
			//! connect via mqtt to iota server and get new messages
			//iota::MessageListener* mIotaMessageListener;
			std::shared_ptr<hiero::MessageListenerQuery> mHieroMessageListener;

			//! contain indices for every public key address, used overall for optimisation
			mutable std::shared_ptr<cache::Dictionary> mPublicKeysIndex;
			// level db to store state values like last transaction
			mutable cache::State mBlockchainState;

			mutable cache::HieroTransactionId mMessageIdsCache;

			cache::TransactionTriggerEvent mTransactionTriggerEventsCache;

			mutable AccessExpireCache<uint32_t, std::shared_ptr<cache::Block>> mCachedBlocks;

			cache::TransactionHash mTransactionHashCache;

			//! Community Server listening on new blocks for his group
			//! TODO: replace with more abstract but simple event system and/or mqtt
			std::shared_ptr<client::Base> mCommunityServer;
			std::vector<std::shared_ptr<client::hiero::ConsensusClient>> mHieroClients;
		};

		std::shared_ptr<FileBased> FileBased::create(
			std::string_view communityId,
			const hiero::TopicId& topicId,
			std::string_view alias,
			std::string_view folder,
			std::vector<std::shared_ptr<client::hiero::ConsensusClient>>&& hieroClients
		) {
			return std::make_shared<FileBased>(Private(), communityId, topicId, alias, folder, std::move(hieroClients));
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