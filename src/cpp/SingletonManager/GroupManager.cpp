#include "GroupManager.h"

//#include "Poco/Path.h"
#include "Poco/File.h"

#include "../ServerGlobals.h"

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
		findGroup(*it);
	}


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
