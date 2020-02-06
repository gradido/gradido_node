#include "ServerGlobals.h"

namespace ServerGlobals {

	UniLib::controller::CPUSheduler* g_CPUScheduler = nullptr;
	controller::GroupIndex* g_GroupIndex = nullptr;
	std::string	g_FilesPath;

	void clearMemory()
	{
		if (g_CPUScheduler) {
			delete g_CPUScheduler;
			g_CPUScheduler = nullptr;
		}
		if (g_GroupIndex) {
			delete g_GroupIndex;
			g_GroupIndex = nullptr;
		}
	}
};