#ifndef __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H
#define __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H

#include "gradido_blockchain/blockchain/AbstractProvider.h"
#include "gradido_blockchain/lib/StringViewCompare.h"
#include "FileBased.h"
#include "../cache/GroupIndex.h"

#include <unordered_map>
#include <mutex>

#define GRADIDO_NODE_MAGIC_NUMBER_COMMUNITY_ID_INDEX_CACHE_SIZE_MBYTE 1

namespace hiero {
	class TopicId;
}

namespace client {
	namespace hiero {
		class ConsensusClient;
	}
}

namespace gradido {
	namespace blockchain {
		/*
		* @author einhornimmond
		* @date 18.08.2025
		* @brief singleton, hold FileBasedBlockchains per community topic id
		*/
		class FileBasedProvider : public AbstractProvider
		{
		public:
			static FileBasedProvider* getInstance();

			std::shared_ptr<Abstract> findBlockchain(uint32_t communityIdIndex) override;
			std::shared_ptr<Abstract> findBlockchain(const std::string& communityId) override;

			std::shared_ptr<Abstract> findBlockchain(hiero::TopicId& topicId);
			//! \return true if successfully else return false
			bool init(
				const std::string& communityConfigFile,
				std::vector<std::shared_ptr<client::hiero::ConsensusClient>>&& hieroClients,
				uint8_t hieroClientsPerCommunity = 3
			);
			void exit();

			//! expensive,  reload config file from disk and add new blockchains, recreate community listener from all
			//! \return count of added blockchain
			int reloadConfig();

			//! list all known communities
			inline std::vector<std::string> listCommunityIds() const;
			inline const cache::GroupIndex* getGroupIndex() const { return mGroupIndex; }
		protected:

			// check if neccessary, or community context is enough
			std::unordered_map<uint32_t, std::shared_ptr<FileBased>> mBlockchainsPerGroup;
			std::recursive_mutex mWorkMutex;

		private:
			FileBasedProvider();
			~FileBasedProvider();

			/* Explicitly disallow copying. */
			FileBasedProvider(const FileBasedProvider&) = delete;
			FileBasedProvider& operator= (const FileBasedProvider&) = delete;

			//! load or create blockchain for community, not locking woking mutex!
			std::shared_ptr<FileBased> addCommunity(
				const std::string& communityId,
				const hiero::TopicId& topicId,
				const std::string&  alias
			);
			void updateListenerCommunity(uint32_t communityIdIndex, const std::string& alias, std::shared_ptr<FileBased> blockchain);

			cache::GroupIndex* mGroupIndex;
			std::vector<std::shared_ptr<client::hiero::ConsensusClient>> mHieroClients;
			uint8_t mHieroClientsPerCommunity;
			bool mInitalized;
		};

		std::vector<std::string> FileBasedProvider::listCommunityIds() const
		{
			return mGroupIndex->listCommunitiesIds();
		}
		
	}
}


#endif //__GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H