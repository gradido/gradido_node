#include "GroupIndex.h"
#include "../controller/ControllerExceptions.h"

#include "../ServerGlobals.h"
#include "gradido_blockchain/AppContext.h"
#include "gradido_blockchain/lib/RapidjsonHelper.h"
#include "gradido_blockchain/data/hiero/TopicId.h"

#include "loguru/loguru.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using gradido::g_appContext;
using std::function;
using std::string;
using std::shared_lock, std::unique_lock;
using std::vector;

namespace cache {
	GroupIndex::GroupIndex(const string& jsonConfigFileName)
		: mConfig(jsonConfigFileName)
	{

	}

	GroupIndex::~GroupIndex()
	{
		clear();
	}

	void GroupIndex::clear()
	{
		mCommunities.clear();
	}

	size_t GroupIndex::update()
	{
		unique_lock _lock(mWorkMutex);

		clear();
		try {
			auto& cfg = mConfig.load();
			if (cfg.IsArray()) {
				auto communitiesArray = cfg.GetArray();
				for (rapidjson::SizeType i = 0; i < communitiesArray.Size(); i++) {
					auto& communityEntry = communitiesArray[i];
					CommunityIndexEntry entry;
					rapidjson_helper::checkMember(communityEntry, "alias", rapidjson_helper::MemberType::STRING);
					rapidjson_helper::checkMember(communityEntry, "communityId", rapidjson_helper::MemberType::STRING);
					rapidjson_helper::checkMember(communityEntry, "folder", rapidjson_helper::MemberType::STRING);
					rapidjson_helper::checkMember(communityEntry, "hieroTopicId", rapidjson_helper::MemberType::STRING);
					entry.alias = communityEntry["alias"].GetString();
					entry.communityId = communityEntry["communityId"].GetString();
					entry.communityIdIndex = g_appContext->getOrAddCommunityIdIndex(entry.communityId);
					entry.topicId = communityEntry["hieroTopicId"].GetString();
					entry.folderName = communityEntry["folder"].GetString();
					if (communityEntry.HasMember("newBlockUri")) {
						entry.newBlockUri = communityEntry["newBlockUri"].GetString();
					}
					if (communityEntry.HasMember("blockUriType")) {
						entry.blockUriType = communityEntry["blockUriType"].GetString();
					}
					mCommunities.insert({ entry.communityIdIndex, entry });
				}
			}
			else {
				throw RapidjsonInvalidMemberException("expected array as root node in commuities config", "", "array");
			}
		}
		catch (GradidoBlockchainException& ex) {
			LOG_F(ERROR, "%s", ex.getFullString().data());
			LOG_F(WARNING, "start without communities");
		}
		
		return mCommunities.size();
	}

	string GroupIndex::getFolder(uint32_t communityIdIndex) const
	{
		shared_lock _lock(mWorkMutex);
		auto it = mCommunities.find(communityIdIndex);
		if(it != mCommunities.end()) {
			string folder = ServerGlobals::g_FilesPath + '/';
			folder += it->second.folderName;
			if(!std::filesystem::exists(folder)) {
				std::filesystem::create_directories(folder);
			}
			return folder;
		}
		return "";
	}

	const CommunityIndexEntry& GroupIndex::getCommunityDetails(const string& communityId) const
	{
		shared_lock _lock(mWorkMutex);
		for (auto& it : mCommunities) {
			if (it.second.communityId == communityId) {
				return it.second;
			}
		}
		throw controller::GroupNotFoundException("couldn't found config details for community", communityId);
	}

	const CommunityIndexEntry& GroupIndex::getCommunityDetails(const hiero::TopicId& topicId) const
	{
		shared_lock _lock(mWorkMutex);
		for (auto& it : mCommunities) {
			if (topicId == hiero::TopicId(it.second.topicId)) {
				return it.second;
			}
		}
		throw controller::GroupNotFoundException("couldn't found config details for community by topic id", topicId.toString());
	}

	const CommunityIndexEntry& GroupIndex::getCommunityDetails(uint32_t communityIdIndex) const
	{
		shared_lock _lock(mWorkMutex);
		auto it = mCommunities.find(communityIdIndex);
		if (it != mCommunities.end()) {
			return it->second;
		}
		throw controller::GroupNotFoundException("couldn't found config details for community", communityIdIndex);
	}

	bool GroupIndex::isCommunityInConfig(uint32_t communityIdIndex) const
	{
		shared_lock _lock(mWorkMutex);
		return mCommunities.find(communityIdIndex) != mCommunities.end();
	}

	void GroupIndex::iterate(function<bool(const CommunityIndexEntry&)> callback) const
	{
		shared_lock _lock(mWorkMutex);
		for (const auto& it : mCommunities) {
			auto result = callback(it.second);
			if (!result) break;
		}
	}

	vector<string> GroupIndex::listCommunitiesIds() const
	{
		shared_lock _lock(mWorkMutex);
		vector<string> result;
		result.reserve(mCommunities.size());
		for (auto it = mCommunities.begin(); it != mCommunities.end(); it++) {
			result.emplace_back(it->second.communityId);
		}
		return result;
	}

	vector<uint32_t> GroupIndex::listCommunitiesIdIndices() const
	{
		shared_lock _lock(mWorkMutex);
		vector<uint32_t> result;
		result.reserve(mCommunities.size());
		for (auto it = mCommunities.begin(); it != mCommunities.end(); it++) {
			result.emplace_back(it->second.communityIdIndex);
		}
		return result;
	}
}
