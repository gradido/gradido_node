#include "Poco/Net/SSLManager.h"
#include "Poco/Net/RejectCertificateHandler.h"
#include "Poco/SharedPtr.h"
#include "Poco/Path.h"

using Poco::Net::SSLManager;
using Poco::Net::Context;
using Poco::Net::InvalidCertificateHandler;
using Poco::Net::RejectCertificateHandler;
using Poco::SharedPtr;


#include "ServerGlobals.h"

#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/http/IotaRequest.h"

namespace ServerGlobals {

	task::CPUSheduler* 	g_CPUScheduler = nullptr;
	task::CPUSheduler* 	g_WriteFileCPUScheduler = nullptr;
	task::CPUSheduler*    g_IotaRequestCPUScheduler = nullptr;
	controller::GroupIndex* 			g_GroupIndex = nullptr;
	std::string							g_FilesPath;
	std::chrono::seconds				g_CacheTimeout(600);
	std::chrono::seconds				g_TimeoutCheck(60);
	std::chrono::seconds				g_WriteToDiskTimeout(10);
	IotaRequest*						g_IotaRequestHandler = nullptr;
	Poco::URI							g_IotaMqttBrokerUri;
	Poco::AtomicCounter		            g_NumberExistingTasks;
	bool								g_LogTransactions = false;

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


	bool initIota(const Poco::Util::LayeredConfiguration& cfg)
	{
		std::string iota_host = cfg.getString("iota.host", "api.lb-0.h.chrysalis-devnet.iota.cafe");
		// testnet
		// api.lb-0.h.chrysalis-devnet.iota.cafe
		// mainnet:
		// chrysalis-nodes.iota.org
        int iota_port = cfg.getInt("iota.port", 443);
		g_IotaRequestHandler = new IotaRequest(iota_host, iota_port, "/api/v1/");

		int mqtt_port = cfg.getInt("iota.mqtt.port", 1883);
		g_IotaMqttBrokerUri = Poco::URI(iota_host + ":" + std::to_string(mqtt_port));

        return true;
	}

	void loadTimeouts(const Poco::Util::LayeredConfiguration& cfg)
	{
		ServerGlobals::g_CacheTimeout = std::chrono::seconds(cfg.getUInt("CacheTimeout", ServerGlobals::g_CacheTimeout.count()));
		ServerGlobals::g_TimeoutCheck = std::chrono::seconds(cfg.getUInt("TimeoutChecks", ServerGlobals::g_TimeoutCheck.count()));
		ServerGlobals::g_WriteToDiskTimeout = std::chrono::seconds(cfg.getUInt("WriteToDiskTimeout", ServerGlobals::g_WriteToDiskTimeout.count()));
	}
};
