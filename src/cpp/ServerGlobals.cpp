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

#include "iota/HTTPApi.h"
#include "lib/Profiler.h"

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
	Context::Ptr g_SSL_CLient_Context = nullptr;
	JsonRequest* g_IotaRequestHandler = nullptr;

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
		if (g_IotaRequestHandler) {
			delete g_IotaRequestHandler;
			g_IotaRequestHandler = nullptr;
		}
	}

	bool initSSLClientContext()
	{
		SharedPtr<InvalidCertificateHandler> pCert = new RejectCertificateHandler(false); // reject invalid certificates
		/*
		Context(Usage usage,
		const std::string& certificateNameOrPath,
		VerificationMode verMode = VERIFY_RELAXED,
		int options = OPT_DEFAULTS,
		const std::string& certificateStoreName = CERT_STORE_MY);
		*/
		try {
#ifdef POCO_NETSSL_WIN
			g_SSL_CLient_Context = new Context(Context::CLIENT_USE, "cacert.pem", Context::VERIFY_RELAXED, Context::OPT_DEFAULTS);
#else

			g_SSL_CLient_Context = new Context(Context::CLIENT_USE, "", "", Poco::Path::config() + "grd_login/cacert.pem", Context::VERIFY_RELAXED, 9, true, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
#endif
		}
		catch (Poco::Exception& ex) {
			printf("[ServerConfig::initSSLClientContext] error init ssl context, maybe no cacert.pem found?\nPlease make sure you have cacert.pem (CA/root certificates) next to binary from https://curl.haxx.se/docs/caextract.html\n");
			return false;
		}
		SSLManager::instance().initializeClient(0, pCert, g_SSL_CLient_Context);


		return true;
	}

	bool initIota(const Poco::Util::LayeredConfiguration& cfg)
	{
		std::string iota_host = cfg.getString("iota.host", "api.lb-0.h.chrysalis-devnet.iota.cafe");
		// testnet
		// api.lb-0.h.chrysalis-devnet.iota.cafe
		// mainnet:
		// chrysalis-nodes.iota.org
        int iota_port = cfg.getInt("iota.port", 443);
		g_IotaRequestHandler = new JsonRequest(iota_host, iota_port, "/api/v1/");

#ifdef __linux__
		strcpy(g_IotaClientConfig.host, iota_host.data());
		g_IotaClientConfig.port = iota_port;
		g_IotaClientConfig.use_tls = true;
#else

#endif
		Profiler timeUsed;
		iota::getNodeInfo();
		printf("time for node info: %s\n", timeUsed.string().data());

		timeUsed.reset();
		auto nodeInfo = iotaHttp::getNodeInfo();
		printf("HTTP Api get node info time: %s\n", timeUsed.string().data());

        return true;
	}
};
