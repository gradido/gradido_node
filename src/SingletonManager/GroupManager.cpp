#include "GroupManager.h"
#include "LoggerManager.h"

//#include "Poco/Path.h"
#include "Poco/File.h"
#include "../ServerGlobals.h"
#include "../client/Json.h"
#include "../client/GraphQL.h"



GroupManager::GroupManager()
	: mInitalized(false), mGroupIndex(nullptr)
{

}

GroupManager::~GroupManager()
{
	Poco::ScopedLock<Poco::FastMutex> lock(mWorkMutex);
	if (mGroupIndex) {
		delete mGroupIndex;
		mGroupIndex = nullptr;
	}
	mInitalized = false;
}

int GroupManager::init(const char* groupIndexFileName, Poco::Util::LayeredConfiguration& config)
{
	Poco::File groupIndexFile(Poco::Path(Poco::Path(ServerGlobals::g_FilesPath), groupIndexFileName));

	if (!groupIndexFile.exists()) {
		groupIndexFile.createFile();
	}

	mWorkMutex.lock();
	mGroupIndex = new controller::GroupIndex(new model::files::GroupIndex(groupIndexFileName));
	mGroupIndex->update();

	mInitalized = true;

	// load special group 
	
	Poco::SharedPtr<controller::Group> group = new controller::GroupRegisterGroup;
	group->init();
	mGroupMap.insert({ GROUP_REGISTER_GROUP_ALIAS, group });
	//*/
	mWorkMutex.unlock();

	auto groups = mGroupIndex->listGroupAliases();
	for (auto it = groups.begin(); it != groups.end(); it++) {
		// load all groups to start iota message listener from all groups
		try {
			auto group = findGroup(*it);
			// for notification of community server by new transaction
			std::string groupAliasConfig = "community." + *it;
			std::string communityNewBlockUri = groupAliasConfig + ".newBlockUri";
			std::string communityNewBlockUriTypeIndex = groupAliasConfig + ".blockUriType";
			std::string communityNewBlockUriType = "json";
			if (config.has(communityNewBlockUriTypeIndex)) {
				communityNewBlockUriType = config.getString(communityNewBlockUriTypeIndex);
			}
			if (config.has(communityNewBlockUri)) {
				client::Base* clientBase = nullptr;
				auto uri = Poco::URI(config.getString(communityNewBlockUri));
				if (communityNewBlockUriType == "json") {
					clientBase = new client::Json(uri);
				}
				else if (communityNewBlockUriType == "graphql") {
					clientBase = new client::GraphQL(uri);
				}
				else {
					LoggerManager::getInstance()->mErrorLogging.error("unknown new block uri type: %s", communityNewBlockUriType);
				}
				if (clientBase) {
					group->setListeningCommunityServer(clientBase);
				}
			}

		}
		catch (GradidoBlockchainTransactionNotFoundException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("[GroupManager::init] transaction not found exception: %s in group: %s", ex.getFullString(), *it);
			return -1;
		}
		catch (GradidoBlockchainException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("[GroupManager::init] gradido blockchain exception: %s in group: %s", ex.getFullString(), *it);
			return -2;
		}
	}

	return 0;
}

void GroupManager::exit()
{
	Poco::ScopedLock<Poco::FastMutex> _lock(mWorkMutex);
	mInitalized = false;
	for (auto it = mGroupMap.begin(); it != mGroupMap.end(); it++) {
		it->second->exit();
	}
	mGroupMap.clear();
}

GroupManager* GroupManager::getInstance()
{
	static GroupManager theOne;
	return &theOne;
}

Poco::SharedPtr<controller::Group> GroupManager::findGroup(const std::string& groupAlias)
{
	if (!mInitalized) return nullptr;

	mWorkMutex.lock();
	auto it = mGroupMap.find(groupAlias);
	if (it != mGroupMap.end()) {
		auto result = it->second;
		mWorkMutex.unlock();
		return result;
	}

	if (!mGroupIndex) {
		mWorkMutex.unlock();
		return nullptr;
	}

	auto folder = mGroupIndex->getFolder(groupAlias);
	if (folder.depth() == 0) {
		mWorkMutex.unlock();
		return nullptr;
	}
	//auto registerGroup = dynamic_cast<controller::GroupRegisterGroup*>(mGroupMap[GROUP_REGISTER_GROUP_ALIAS].get());

	//Poco::SharedPtr<controller::Group> group = new controller::Group(groupAlias, folder, registerGroup->findGroup(groupAlias).coinColor);
	Poco::SharedPtr<controller::Group> group = new controller::Group(groupAlias, folder, 0);
	mGroupMap.insert({ groupAlias, group });
	mWorkMutex.unlock();
	group->init();
	//mGroups.insert(std::pair<std::string, controller::Group*>(groupAlias, group));
	return group;
}

std::vector<Poco::SharedPtr<controller::Group>> GroupManager::findAllGroupsWhichHaveTransactionsForPubkey(const std::string& pubkey)
{
	Poco::ScopedLock<Poco::FastMutex> _lock(mWorkMutex);
	std::vector<Poco::SharedPtr<controller::Group>> results;
	results.reserve(mGroupMap.size());
	for (auto it = mGroupMap.begin(); it != mGroupMap.end(); it++) {
		if (it->second->getAddressIndex()->getIndexForAddress(pubkey)) {
			results.push_back(it->second);
		}
	}

	return results;
}
