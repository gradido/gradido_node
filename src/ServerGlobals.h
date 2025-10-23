#ifndef GRADIDO_NODE_SERVER_GLOBALS 
#define GRADIDO_NODE_SERVER_GLOBALS

#include "gradido_blockchain/http/IotaRequest.h"
#include "gradido_blockchain/lib/MapEnvironmentToConfig.h"
#include "cache/GroupIndex.h"
#include "task/CPUSheduler.h"

#include <chrono>
#include <atomic>

namespace client {
	namespace hiero {
		class MirrorClient;
	}
}

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
	extern std::string						g_IotaMqttBrokerUri;
	extern std::atomic<size_t>              g_NumberExistingTasks;
	extern bool								g_LogTransactions;
	// if true serve only stored transactions, for example debug without internet connection
	extern bool								g_isOfflineMode;
	extern client::hiero::MirrorClient*		g_HieroMirrorNode;

	void clearMemory();
	// bool initIota(const MapEnvironmentToConfig& cfg);
	bool initHiero(std::string_view hieroNetworkType);
	void loadTimeouts(const MapEnvironmentToConfig& cfg);
};

#endif //GRADIDO_NODE_SERVER_GLOBALS