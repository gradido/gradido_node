#ifndef GRADIDO_NODE_SERVER_GLOBALS 
#define GRADIDO_NODE_SERVER_GLOBALS

#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/Types.h"

#include "task/CPUSheduler.h"

namespace ServerGlobals {

	extern UniLib::controller::CPUSheduler* g_CPUScheduler;

};

#endif //GRADIDO_NODE_SERVER_GLOBALS