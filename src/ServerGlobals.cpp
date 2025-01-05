#include "ServerGlobals.h"

#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/http/IotaRequest.h"

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
	IotaRequest*						g_IotaRequestHandler = nullptr;
	std::string							g_IotaMqttBrokerUri;
	std::atomic<size_t>		            g_NumberExistingTasks;
	bool								g_LogTransactions = false;
	bool								g_isOfflineMode = false;

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
		if (g_IotaRequestHandler) {
			delete g_IotaRequestHandler;
			g_IotaRequestHandler = nullptr;
		}
	}


	bool initIota(const MapEnvironmentToConfig& cfg)
	{
		// testnet
		// api.lb-0.h.chrysalis-devnet.iota.cafe
		// mainnet:
		// chrysalis-nodes.iota.org
		std::string iotaHost = cfg.getString("clients.iota.rest_api.host", "api.lb-0.h.chrysalis-devnet.iota.cafe");
		int iotaPort = cfg.getInt("clients.iota.rest_api.port", 443);
		g_IotaRequestHandler = new IotaRequest(iotaHost, iotaPort, "/api/v1/");

		std::string iotaMqttHost = cfg.getString("clients.iota.mqtt.host", "api.lb-0.h.chrysalis-devnet.iota.cafe");
		int mqttPort = cfg.getInt("clients.iota.mqtt.port", 1883);
		g_IotaMqttBrokerUri = iotaHost + ":" + std::to_string(mqttPort);

		g_isOfflineMode = cfg.getBool("clients.isOfflineMode", false);
	    return true;	
	}

	void loadTimeouts(const MapEnvironmentToConfig& cfg)
	{
		g_CacheTimeout = seconds(cfg.getInt("cache.timeout", duration_cast<seconds>(g_CacheTimeout).count()));
		g_TimeoutCheck = seconds(cfg.getInt("cache.checks_interval", duration_cast<seconds>(g_TimeoutCheck).count()));
		g_WriteToDiskTimeout = seconds(cfg.getInt("cache.write_to_disk_interval", duration_cast<seconds>(g_WriteToDiskTimeout).count()));
	}
};
