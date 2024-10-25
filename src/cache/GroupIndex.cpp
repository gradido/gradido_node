#include "GroupIndex.h"
#include "../controller/ControllerExceptions.h"

#include "../ServerGlobals.h"
#include "gradido_blockchain/lib/RapidjsonHelper.h"

#include "loguru/loguru.hpp"

#include <filesystem>

namespace cache {
	GroupIndex::GroupIndex(const std::string& jsonConfigFileName)
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
		std::scoped_lock _lock(mWorkMutex);

		clear();
		try {
			auto& cfg = mConfig.load();
			if (cfg.IsArray()) {
				auto communitiesArray = cfg.GetArray();
				for (rapidjson::SizeType i = 0; i < communitiesArray.Size(); i++) {
					auto& communityEntry = communitiesArray[i];
					CommunityIndexEntry entry;
					rapidjson_helper::checkMember(communityEntry, "alias", rapidjson_helper::MemberType::STRING);
					rapidjson_helper::checkMember(communityEntry, "folder", rapidjson_helper::MemberType::STRING);
					entry.alias = communityEntry["alias"].GetString();
					entry.folderName = communityEntry["folder"].GetString();
					if (communityEntry.HasMember("newBlockUri")) {
						entry.newBlockUri = communityEntry["newBlockUri"].GetString();
					}
					if (communityEntry.HasMember("blockUriType")) {
						entry.blockUriType = communityEntry["blockUriType"].GetString();
					}
					mCommunities.insert({ entry.alias, entry });
				}
			}
			else {
				throw RapidjsonInvalidMemberException("expected array as root node in commuities config", "", "array");
			}
		}
		catch (GradidoBlockchainException& ex) {
			LOG_F(ERROR, ex.getFullString().data());
			LOG_F(WARNING, "start without communities");
		}
		
		return mCommunities.size();
	}

	std::string GroupIndex::getFolder(const std::string& communityAlias)
	{
		std::scoped_lock _lock(mWorkMutex);

		auto it = mCommunities.find(communityAlias);
		if(it != mCommunities.end()) {
			std::string folder = ServerGlobals::g_FilesPath + '/';
			folder += it->second.folderName;
			if(!std::filesystem::exists(folder)) {
				std::filesystem::create_directories(folder);
			}
			return folder;
		}
		return "";
	}
	const CommunityIndexEntry& GroupIndex::getCommunityDetails(const std::string& communityAlias) const
	{
		std::scoped_lock _lock(mWorkMutex);

		auto it = mCommunities.find(communityAlias);
		if (it != mCommunities.end()) {
			return it->second;
		}
		throw controller::GroupNotFoundException("couldn't found config details for community", communityAlias);
	}
	bool GroupIndex::isCommunityInConfig(const std::string& communityAlias) const
	{
		std::scoped_lock _lock(mWorkMutex);
		return mCommunities.find(communityAlias) != mCommunities.end();
	}

	std::vector<std::string> GroupIndex::listGroupAliases()
	{
		std::scoped_lock _lock(mWorkMutex);
		std::vector<std::string> result;
		for (auto it = mCommunities.begin(); it != mCommunities.end(); it++) {
			result.push_back(it->first);
		}
		return result;
	}
}
