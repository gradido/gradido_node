#include "ServerGlobals.h"

namespace ServerGlobals {

	UniLib::controller::CPUSheduler* 	g_CPUScheduler = nullptr;
	UniLib::controller::CPUSheduler* 	g_WriteFileCPUScheduler = nullptr;
	controller::GroupIndex* 			g_GroupIndex = nullptr;
	std::string							g_FilesPath;
	Poco::UInt16						g_CacheTimeout = 600;
	Poco::UInt16						g_TimeoutCheck = 60;
	Poco::UInt16						g_WriteToDiskTimeout = 10;
#ifdef __linux__
	iota_client_conf_t 					g_IotaClientConfig;
#endif

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

	bool initIota(const Poco::Util::LayeredConfiguration& cfg)
	{
		std::string iota_host = cfg.getString("iota.host", "api.lb-0.h.chrysalis-devnet.iota.cafe");
		// testnet
		// api.lb-0.h.chrysalis-devnet.iota.cafe
		// mainnet:
		// chrysalis-nodes.iota.org
        int iota_port = cfg.getInt("iota.port", 443);
#ifdef __linux__
		strcpy(g_IotaClientConfig.host, iota_host.data());
		g_IotaClientConfig.port = iota_port;
		g_IotaClientConfig.use_tls = true;
#else

#endif

        return true;
	}
};
