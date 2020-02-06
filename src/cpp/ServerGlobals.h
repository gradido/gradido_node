#ifndef GRADIDO_NODE_SERVER_GLOBALS 
#define GRADIDO_NODE_SERVER_GLOBALS

#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/Types.h"

#include "task/CPUSheduler.h"

#include "controller/GroupIndex.h"

namespace ServerGlobals {

	extern UniLib::controller::CPUSheduler* g_CPUScheduler;
	extern controller::GroupIndex*			g_GroupIndex;
	extern std::string						g_FilesPath;

	void clearMemory();
};

#endif //GRADIDO_NODE_SERVER_GLOBALS