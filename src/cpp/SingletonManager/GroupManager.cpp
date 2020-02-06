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

	return 0;
}

GroupManager* GroupManager::getInstance()
{
	static GroupManager theOne;
	return &theOne;
}