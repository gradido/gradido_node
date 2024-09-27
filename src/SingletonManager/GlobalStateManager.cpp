#include "GlobalStateManager.h"
#include "../ServerGlobals.h"

GlobalStateManager::GlobalStateManager()
	: mGlobalStateFile(ServerGlobals::g_FilesPath + '/')
{
	mGlobalStateFile.init();
}

GlobalStateManager::~GlobalStateManager()
{
	exit();
}

void GlobalStateManager::exit()
{
	mGlobalStateFile.exit();
}

GlobalStateManager* GlobalStateManager::getInstance()
{
	static GlobalStateManager one;
	return &one;
}

