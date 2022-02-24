#ifndef GRADIDO_NODE_SERVER_GLOBALS 
#define GRADIDO_NODE_SERVER_GLOBALS

#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/Types.h"
#include "Poco/Logger.h"
#include "Poco/Net/Context.h"
#include "Poco/AtomicCounter.h"

#include "task/CPUSheduler.h"
#include "gradido_blockchain/http/IotaRequest.h"

#include "controller/GroupIndex.h"

namespace ServerGlobals {

	extern task::CPUSheduler* g_CPUScheduler;
	extern task::CPUSheduler* g_WriteFileCPUScheduler;
	extern task::CPUSheduler* g_IotaRequestCPUScheduler;
	extern controller::GroupIndex*			g_GroupIndex;
	extern std::string						g_FilesPath;
	//! cache timeout in seconds, default 10 minutes
	extern Poco::UInt16						g_CacheTimeout;
	//! in which timespan the timeout manager checks timeouts in seconds, default 1 minute
	extern Poco::UInt16						g_TimeoutCheck;
	//! in which timespan data will be flushed to disk, in seconds, default 10 seconds
	extern Poco::UInt16						g_WriteToDiskTimeout;
	extern Poco::Net::Context::Ptr g_SSL_CLient_Context;
	extern IotaRequest* g_IotaRequestHandler;
	extern Poco::AtomicCounter              g_NumberExistingTasks;
	extern bool								g_LogTransactions;

	void clearMemory();
	bool initSSLClientContext();
	bool initIota(const Poco::Util::LayeredConfiguration& cfg);
};

#endif //GRADIDO_NODE_SERVER_GLOBALS