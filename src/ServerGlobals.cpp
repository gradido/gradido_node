#include "ServerGlobals.h"

#include "client/hiero/MirrorClient.h"

using namespace std::chrono;

namespace ServerGlobals {

	task::CPUSheduler* 					g_CPUScheduler = nullptr;
	task::CPUSheduler* 					g_WriteFileCPUScheduler = nullptr;
	task::CPUSheduler*					g_IotaRequestCPUScheduler = nullptr;
	cache::GroupIndex* 					g_GroupIndex = nullptr;
	std::string							g_FilesPath;
	std::chrono::seconds				g_CacheTimeout(600);
	std::chrono::seconds				g_TimeoutCheck(60);
	std::chrono::seconds				g_WriteToDiskTimeout(10);
	std::string							g_IotaMqttBrokerUri;
	std::atomic<size_t>		            g_NumberExistingTasks;
	bool								g_LogTransactions = false;
	bool								g_isOfflineMode = false;
	client::hiero::MirrorClient*		g_HieroMirrorNode = nullptr;

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
		if (g_IotaRequestCPUScheduler) {
			delete g_IotaRequestCPUScheduler;
			g_IotaRequestCPUScheduler = nullptr;
		}
		if (g_GroupIndex) {
			delete g_GroupIndex;
			g_GroupIndex = nullptr;
		}
		if (g_HieroMirrorNode) {
			delete g_HieroMirrorNode;
			g_HieroMirrorNode = nullptr;
		}
	}


	bool initHiero(std::string_view hieroNetworkType) {
		g_HieroMirrorNode = new client::hiero::MirrorClient(hieroNetworkType);
		return true;
	}

	void loadTimeouts(const MapEnvironmentToConfig& cfg)
	{
		g_CacheTimeout = seconds(cfg.getInt("cache.timeout", duration_cast<seconds>(g_CacheTimeout).count()));
		g_TimeoutCheck = seconds(cfg.getInt("cache.checks_interval", duration_cast<seconds>(g_TimeoutCheck).count()));
		g_WriteToDiskTimeout = seconds(cfg.getInt("cache.write_to_disk_interval", duration_cast<seconds>(g_WriteToDiskTimeout).count()));
	}
};
