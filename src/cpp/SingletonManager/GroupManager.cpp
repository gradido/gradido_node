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

	mGroupIndex = new controller::GroupIndex(new model::files::GroupIndex(groupIndexFileName));
	mGroupIndex->update();
	mInitalized = true;

	return 0;
}

GroupManager* GroupManager::getInstance()
{
	static GroupManager theOne;
	return &theOne;
}

Poco::SharedPtr<controller::Group> GroupManager::findGroup(const std::string& base58GroupHash)
{
	if (!mInitalized) return nullptr;
	//Poco::Mutex::ScopedLock lock(mWorkingMutex);
	auto skey = mGroupAccessExpireCache.get(base58GroupHash);
	if (!skey.isNull()) {
		return skey;
	}

	if (!mGroupIndex) return nullptr;

	auto folder = mGroupIndex->getFolder(base58GroupHash);
	if (folder.depth() == 0) {
		return nullptr;
	}
	Poco::SharedPtr<controller::Group> group = new controller::Group(base58GroupHash, folder);
	mGroupAccessExpireCache.add(base58GroupHash, group);
	//mGroups.insert(std::pair<std::string, controller::Group*>(base58GroupHash, group));
	return group;
}