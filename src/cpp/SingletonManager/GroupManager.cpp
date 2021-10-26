#include "GroupManager.h"

//#include "Poco/Path.h"
#include "Poco/File.h"

#include "../ServerGlobals.h"

GroupManager::GroupManager()
	: mInitalized(false), mGroupIndex(nullptr), mGroupAccessExpireCache(ServerGlobals::g_CacheTimeout * 1000)
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
	Poco::ScopedLock<Poco::FastMutex> lock(mWorkMutex);
	Poco::File groupIndexFile(Poco::Path(Poco::Path(ServerGlobals::g_FilesPath), groupIndexFileName));

	if (!groupIndexFile.exists()) {
		groupIndexFile.createFile();
	}

	mGroupIndex = new controller::GroupIndex(new model::files::GroupIndex(groupIndexFileName));
	mGroupIndex->update();
	//auto groups = mGroupIndex->listGroupAliases();
	
	mInitalized = true;

	return 0;
}

GroupManager* GroupManager::getInstance()
{
	static GroupManager theOne;
	return &theOne;
}

Poco::SharedPtr<controller::Group> GroupManager::findGroup(const std::string& groupAlias)
{
	if (!mInitalized) return nullptr;
	Poco::ScopedLock<Poco::FastMutex> lock(mWorkMutex);
	auto skey = mGroupAccessExpireCache.get(groupAlias);
	if (!skey.isNull()) {
		return skey;
	}

	if (!mGroupIndex) return nullptr;

	auto folder = mGroupIndex->getFolder(groupAlias);
	if (folder.depth() == 0) {
		return nullptr;
	}
	Poco::SharedPtr<controller::Group> group = new controller::Group(groupAlias, folder);
	mGroupAccessExpireCache.add(groupAlias, group);
	//mGroups.insert(std::pair<std::string, controller::Group*>(groupAlias, group));
	return group;
}