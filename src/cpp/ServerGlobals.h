#ifndef GRADIDO_NODE_SERVER_GLOBALS 
#define GRADIDO_NODE_SERVER_GLOBALS

#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/Types.h"
#include "Poco/Logger.h"

#include "task/CPUSheduler.h"


#include "controller/GroupIndex.h"

namespace ServerGlobals {

	extern UniLib::controller::CPUSheduler* g_CPUScheduler;
	extern UniLib::controller::CPUSheduler* g_WriteFileCPUScheduler;
	extern controller::GroupIndex*			g_GroupIndex;
	extern std::string						g_FilesPath;
	// cache timeout in seconds, default 10 minutes
	extern Poco::UInt16						g_CacheTimeout;
	// in which timespans the timeout manager checks timeouts in seconds, default 1 minute
	extern Poco::UInt16						g_TimeoutCheck;

	void clearMemory();
};

#endif //GRADIDO_NODE_SERVER_GLOBALS