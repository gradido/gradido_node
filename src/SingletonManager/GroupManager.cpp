#include "GroupManager.h"
#include "LoggerManager.h"
#include "../controller/ControllerExceptions.h"

//#include "Poco/Path.h"
#include "Poco/File.h"
#include "../ServerGlobals.h"
#include "../client/JsonRPC.h"

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
	mWorkMutex.unlock();

	auto groups = mGroupIndex->listGroupAliases();
	auto& errorLog = LoggerManager::getInstance()->mErrorLogging;
	for (auto it = groups.begin(); it != groups.end(); it++) {
		// load all groups to start iota message listener from all groups
		try {
			// with that call not running groups will be initialized and start listening
			auto group = findGroup(*it);

			// for notification of community server by new transaction
			std::string groupAliasConfig = "community." + *it;
			std::string communityNewBlockUri = groupAliasConfig + ".newBlockUri";
			auto communityNewBlockUriType = config.getString(groupAliasConfig + ".blockUriType", "json");

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
					errorLog.error("unknown new block uri type: %s", communityNewBlockUriType);
				}
				if (clientBase) {					
					clientBase->setGroupAlias(*it);
					group->setListeningCommunityServer(clientBase);
					errorLog.information("[GroupManager::init] notification of community: %s", *it);
				}
			}

		}
		catch (GradidoBlockchainTransactionNotFoundException& ex) {
			errorLog.error("[GroupManager::init] transaction not found exception: %s in group: %s", ex.getFullString(), *it);
			return -1;
		}
		catch (GradidoBlockchainException& ex) {
			errorLog.error("[GroupManager::init] gradido blockchain exception: %s in group: %s", ex.getFullString(), *it);
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

std::shared_ptr<controller::Group> GroupManager::findGroup(const std::string& groupAlias)
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

	//std::shared_ptr<controller::Group> group = new controller::Group(groupAlias, folder, registerGroup->findGroup(groupAlias).coinColor);
	std::shared_ptr<controller::Group> group = new controller::Group(groupAlias, folder);
	mGroupMap.insert({ groupAlias, group });
	mWorkMutex.unlock();
	group->init();
	//mGroups.insert(std::pair<std::string, controller::Group*>(groupAlias, group));
	return group;
}

std::vector<std::shared_ptr<controller::Group>> GroupManager::findAllGroupsWhichHaveTransactionsForPubkey(const std::string& pubkey)
{
	Poco::ScopedLock<Poco::FastMutex> _lock(mWorkMutex);
	std::vector<std::shared_ptr<controller::Group>> results;
	results.reserve(mGroupMap.size());
	for (auto it = mGroupMap.begin(); it != mGroupMap.end(); it++) {
		if (it->second->getAddressIndex()->getIndexForAddress(pubkey)) {
			results.push_back(it->second);
		}
	}

	return results;
}
