#include "GroupManager.h"

//#include "Poco/Path.h"
#include "Poco/File.h"

#include "../ServerGlobals.h"
#include "../lib/Exceptions.h"

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

int GroupManager::init(const char* groupIndexFileName)
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
	for (auto it = groups.begin(); it != groups.end(); it++) {
		// load all groups to start iota message listener from all groups
		try {
			findGroup(*it);
		}
		catch (TransactionLoadingException& ex) {
			printf("group: %s, %s: transaction nr: %s, error: %d\n",
				it->data(), ex.name(), ex.message().data(), ex.code()
			);
			return -1;
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
	Poco::SharedPtr<controller::Group> group = new controller::Group(groupAlias, folder);
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
