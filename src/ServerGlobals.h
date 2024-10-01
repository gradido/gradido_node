#ifndef GRADIDO_NODE_SERVER_GLOBALS 
#define GRADIDO_NODE_SERVER_GLOBALS

#include <chrono>

#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/Types.h"
#include "Poco/Logger.h"
#include "Poco/Net/Context.h"
#include "Poco/AtomicCounter.h"
#include "Poco/URI.h"

#include "task/CPUSheduler.h"
#include "gradido_blockchain/http/IotaRequest.h"

#include "cache/GroupIndex.h"

namespace ServerGlobals {

	extern task::CPUSheduler*				g_CPUScheduler;
	extern task::CPUSheduler*				g_WriteFileCPUScheduler;
	extern task::CPUSheduler*				g_IotaRequestCPUScheduler;
	extern cache::GroupIndex*				g_GroupIndex;
	extern std::string						g_FilesPath;
	//! cache timeout in seconds, default 10 minutes
	extern std::chrono::seconds             g_CacheTimeout;
	//! in which timespan the timeout manager checks timeouts in seconds, default 1 minute
	extern std::chrono::seconds				g_TimeoutCheck;
	//! in which timespan data will be flushed to disk, in seconds, default 10 seconds
	extern std::chrono::seconds				g_WriteToDiskTimeout;	
	extern IotaRequest*						g_IotaRequestHandler;
	extern Poco::URI						g_IotaMqttBrokerUri;
	extern Poco::AtomicCounter              g_NumberExistingTasks;
	extern bool								g_LogTransactions;

	void clearMemory();
	bool initIota(const Poco::Util::LayeredConfiguration& cfg);
	void loadTimeouts(const Poco::Util::LayeredConfiguration& cfg);
};

#endif //GRADIDO_NODE_SERVER_GLOBALS