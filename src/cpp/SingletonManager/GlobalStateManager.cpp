#include "GlobalStateManager.h"
#include "../ServerGlobals.h"

GlobalStateManager::GlobalStateManager()
	: mGlobalStateFile(nullptr), mInitalized(false), mLastIotaMilestone(0)
{
	mGlobalStateFile = new model::files::State(Poco::Path(ServerGlobals::g_FilesPath + "/"));
	
	mLastIotaMilestone = mGlobalStateFile->getInt32ValueForKey("lastIotaMilestone", mLastIotaMilestone);

	mInitalized = true;
}

GlobalStateManager::~GlobalStateManager()
{
	mInitalized = false;
	if (mGlobalStateFile) {		
		delete mGlobalStateFile;
		mGlobalStateFile = nullptr;
	}
}

GlobalStateManager* GlobalStateManager::getInstance()
{
	static GlobalStateManager one;
	return &one;
}

void GlobalStateManager::updateLastIotaMilestone(int32_t lastIotaMilestone)
{
	mLastIotaMilestone = lastIotaMilestone;
	mGlobalStateFile->setKeyValue("lastIotaMilestone", std::to_string(lastIotaMilestone));
}