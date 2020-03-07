#include "ServerGlobals.h"

namespace ServerGlobals {

	UniLib::controller::CPUSheduler* g_CPUScheduler = nullptr;
	extern UniLib::controller::CPUSheduler* g_WriteFileCPUScheduler = nullptr;
	controller::GroupIndex* g_GroupIndex = nullptr;
	std::string	g_FilesPath;
	Poco::UInt16							g_CacheTimeout = 600;
	extern Poco::UInt16						g_TimeoutCheck = 60;

	void clearMemory()
	{
		if (g_CPUScheduler) {
			delete g_CPUScheduler;
			g_CPUScheduler = nullptr;
		}
		if (g_WriteFileCPUScheduler) {
			delete g_WriteFileCPUScheduler;
			g_WriteFileCPUScheduler = nullptr;
		}
		if (g_GroupIndex) {
			delete g_GroupIndex;
			g_GroupIndex = nullptr;
		}
	}
};