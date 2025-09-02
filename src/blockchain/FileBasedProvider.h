#ifndef __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H
#define __GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H

#include "gradido_blockchain/blockchain/AbstractProvider.h"
#include "gradido_blockchain/lib/StringViewCompare.h"
#include "FileBased.h"
#include "../cache/GroupIndex.h"

#define GRADIDO_NODE_MAGIC_NUMBER_COMMUNITY_ID_INDEX_CACHE_SIZE_MBYTE 1

namespace hiero {
	class TopicId;
}

namespace client {
	namespace grpc {
		class Client;
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

			std::shared_ptr<Abstract> findBlockchain(std::string_view communityId);
			//! \return true if successfully else return false
			bool init(
				const std::string& communityConfigFile,
				std::vector<std::shared_ptr<client::grpc::Client>>&& hieroClients,
				uint8_t hieroClientsPerCommunity = 3
			);
			void exit();

			//! expensive,  reload config file from disk and add new blockchains, recreate community listener from all
			//! \return count of added blockchain
			int reloadConfig();

			// access community id index
			inline uint32_t getCommunityIdIndex(const std::string& communityId);
			inline uint32_t getCommunityIdIndex(std::string_view communityId);
			inline const std::string getCommunityIdString(uint32_t index);
		protected:

			std::map<std::string, std::shared_ptr<FileBased>, StringViewCompare> mBlockchainsPerGroup;
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
				const std::string&  alias,
				bool resetIndices
			);
			void updateListenerCommunity(const std::string& communityId, const std::string& alias, std::shared_ptr<FileBased> blockchain);

			cache::GroupIndex* mGroupIndex;
			cache::Dictionary  mCommunityIdIndex;
			std::vector<std::shared_ptr<client::grpc::Client>> mHieroClients;
			uint8_t mHieroClientsPerCommunity;
			bool mInitalized;
		};

		uint32_t FileBasedProvider::getCommunityIdIndex(const std::string& communityId)
		{
			return mCommunityIdIndex.getIndexForString(communityId);
		}
		uint32_t FileBasedProvider::getCommunityIdIndex(std::string_view communityId)
		{
			return getCommunityIdIndex(std::string(communityId));
		}
		const std::string FileBasedProvider::getCommunityIdString(uint32_t index)
		{
			return mCommunityIdIndex.getStringForIndex(index);
		}
	}
}


#endif //__GRADIDO_NODE_BLOCKCHAIN_FILE_BASED_PROVIDER_H