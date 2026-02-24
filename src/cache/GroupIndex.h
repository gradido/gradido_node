#ifndef __GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H
#define __GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H

#include "gradido_blockchain/lib/DRHash.h"
#include "../model/files/JsonFile.h"

#include <functional>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace hiero {
	class TopicId;
}

namespace cache {

	struct CommunityIndexEntry
	{
		std::string alias;
		std::string communityId;
		std::string topicId;
		std::string folderName;
		std::string newBlockUri;
		std::string blockUriType;
		uint32_t    communityIdIndex;

		DHASH makeHash() {
			return DRMakeStringHash(alias.data(), alias.size());
		}
	};

	/*!
	 * @author Dario Rekowski
	 * 
	 * @date 2020-02-06
	 * 
	 * @brief storing group folder name in memory in hash list for fast access
	 *
	 * protected by FastMutex 
	 *
	 * TODO: adding function to adding group folder pair and save changed group.index file
	 */
	class GroupIndex
	{
	public:
		//! load communities for listening from config file in json format
		GroupIndex(const std::string& jsonConfigFileName);

		~GroupIndex();

		//! \brief clear and refill hash list and doublet's vector from mConfig
		//! protected with fast mutex
		//! \return hash map entry count
		size_t update();

		//! \brief get full folder path for group public key
		//! if the hash from groupPublicKey exist multiple time, getting folder from mConfig, else from mHashList (faster)
		//! \return complete path to group folder or empty path if not group not found
		std::string getFolder(uint32_t communityIdIndex) const;
		//! throw GroupNotFoundException Exception of community don't exist in config
		const CommunityIndexEntry& getCommunityDetails(const std::string& communityId) const;
		const CommunityIndexEntry& getCommunityDetails(const hiero::TopicId& topicId) const;
		const CommunityIndexEntry& getCommunityDetails(uint32_t communityIdIndex) const;
		bool isCommunityInConfig(uint32_t communityIdIndex) const;

		// callback for each community, stop if return false
		template<typename callbackFunc>
		void iterate(callbackFunc callback) const {
			std::shared_lock _lock(mWorkMutex);
			for (const auto& it : mCommunities) {
				auto result = callback(it.second);
				if (!result) break;
			}
		}

		//! \brief collect all group aliases from unordered map (not the fastest operation from unordered map)
		//! \return vector with group aliases registered to the node server
		std::vector<std::string> listCommunitiesIds() const;	
		std::vector<uint32_t> listCommunitiesIdIndices() const;
		
	protected:
		mutable std::shared_mutex mWorkMutex;
		model::files::JsonFile mConfig;

		std::unordered_map<uint32_t, CommunityIndexEntry> mCommunities;

		//! \brief clear hash list and doublet's vector
		void clear();

	};
};

#endif //__GRADIDO_NODE_CONTROLLER_GROUP_INDEX_H