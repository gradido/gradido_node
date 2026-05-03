#include "client/hiero/ConsensusClient.h"
#include "MainServer.h"
#include "ServerGlobals.h"

#include "blockchain/FileBasedProvider.h"
#include "server/json-rpc/ApiHandler.h"
// #include "iota/MqttClientWrapper.h"
#include "SingletonManager/CacheManager.h"

#include "hiero/Addressbook.h"
#include "client/hiero/const.h"
#include "lib/PersistentDictionary.h"

#include "gradido_blockchain/AppContext.h"
#include "gradido_blockchain/lib/MonotonicTimer.h"
#include "gradido_blockchain/http/ServerConfig.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sodium.h>
#include <string>
#include <vector>

#include "cpp-httplib/httplib.h"
#include "loguru.hpp"

using gradido::blockchain::FileBasedProvider;
using gradido::g_appContext, gradido::AppContext;
using server::json_rpc::ApiHandler;
using std::filesystem::create_directories, std::filesystem::exists, std::filesystem::is_regular_file, std::filesystem::path;
using std::shared_ptr, std::make_unique;
using std::string;
using std::vector;

MainServer::MainServer()
	: mHttpServer(nullptr)
{
}

MainServer::~MainServer()
{
}

bool MainServer::init()
{
	MonotonicTimer usedTime;
	ServerGlobals::g_FilesPath = getHomeDir() + "/.gradido";
	create_directories(ServerGlobals::g_FilesPath);
	
	// ********** logging ************************************
	string logPath = ServerGlobals::g_FilesPath + "/logs";
	create_directories(logPath);
	// beware, no logrotation in loguru
	// TODO: add config options to choose which to use
	// TODO: check switching to https://github.com/gabime/spdlog
	string errorLogFile = logPath + "/errors.log";
	loguru::add_file(errorLogFile.data(), loguru::Append, loguru::Verbosity_WARNING);
	// info 
	string infoLogFile = logPath + "/infos.log";
	loguru::add_file(infoLogFile.data(), loguru::Append, loguru::Verbosity_INFO);
	// infos and above
	string debugLogFile = logPath + "/debug.log";
	loguru::add_file(debugLogFile.data(), loguru::Truncate, loguru::Verbosity_MAX);
#if defined(__linux__) || defined(__unix__)
	// use syslog on linux, this has logrotation build in
	loguru::add_syslog("GradidoNode", loguru::Verbosity_MAX);
#endif

	// *************** load from config ********************************************

	// load config file if found, else use a empty config
	auto configFile = findConfigFile();
	MapEnvironmentToConfig config(configFile);

	unsigned short jsonrpc_port = (unsigned short)config.getInt("server.json_rpc", 8340);

	auto communityDictionary = make_unique<PersistentDictionary<std::string>>(ServerGlobals::g_FilesPath + "/communityIdsCache");
	communityDictionary->init(GRADIDO_NODE_MAGIC_NUMBER_COMMUNITY_INDEX_CACHE_BYTES);
	auto nameHashDictionary = make_unique<PersistentDictionary<GenericHash, GenericHashHash, GenericHashEqual>>(ServerGlobals::g_FilesPath + "/nameHashCache");
	nameHashDictionary->init(GRADIDO_NODE_MAGIC_NUMBER_COMMUNITY_INDEX_CACHE_BYTES);
	g_appContext = make_unique<AppContext>(std::move(communityDictionary), std::move(nameHashDictionary));
	g_appContext->syncCommunityContextsWithCommunityIds();

	// timeouts
	ServerGlobals::loadTimeouts(config);
	ServerGlobals::g_LogTransactions = config.getBool("logging.log_transactions", ServerGlobals::g_LogTransactions);
	// ServerGlobals::initIota(config);
	ServerConfig::readUnsecureFlags(config);

	// start cpu scheduler
	// std::thread::hardware_concurrency() sometime return 0 if number couldn't be determined
	uint8_t worker_count = std::max(2, (int)std::thread::hardware_concurrency() * 2);
	// I think 1 or 2 by HDD is ok, more by SSD, but should be profiled on work load
	uint8_t io_worker_count = config.getInt("io.worker_count", 2);
	ServerGlobals::g_CPUScheduler = new task::CPUSheduler(worker_count, "Default Worker");
	// let scheduler check if one of the pending tasks is now ready
	CacheManager::getInstance()->getFuzzyTimer()->addTimer("mainCPUScheduler", ServerGlobals::g_CPUScheduler, std::chrono::milliseconds(100));
	ServerGlobals::g_WriteFileCPUScheduler = new task::CPUSheduler(io_worker_count, "IO Worker");
	// ServerGlobals::g_IotaRequestCPUScheduler = new task::CPUSheduler(2, "Iota Worker");
	string hieroNetworkType = config.getString("clients.hiero.networkType", "testnet");
	ServerGlobals::initHiero(hieroNetworkType);

	uint8_t hieroNodeCount = config.getInt("clients.hiero.nodeCount", 3);
	uint8_t hieroNodeCountPerCommunity = config.getInt("clients.hiero.nodeCountPerCommunity", 3);
	vector<shared_ptr<client::hiero::ConsensusClient>> hieroClients;

	if (!ServerGlobals::g_isOfflineMode) {		
		//iota::MqttClientWrapper::getInstance()->init();
		string grpcAddressesFile = ServerGlobals::g_FilesPath + "/addressbook/" + hieroNetworkType + ".pb";
		
		if (!hieroNodeCount || !hieroNodeCountPerCommunity) {
			LOG_F(ERROR, "clients.hiero.nodeCountPerCommunity and clients.hiero.nodeCount need to be both >0");
		}
		if (hieroNodeCountPerCommunity > hieroNodeCount) {
			LOG_F(ERROR, "clients.hiero.nodeCountPerCommunity (%d) mustn't be greate than clients.hiero.nodeCount (%d)", hieroNodeCount, hieroNodeCountPerCommunity);
		}
		hiero::Addressbook addressbook(grpcAddressesFile.c_str());
		addressbook.load();
		
		hieroClients.reserve(hieroNodeCount);
		for (int i = 0; i < hieroNodeCount; i++) {
			if (mMasterStopSource.stop_requested()) break;
			const auto& hieroNode = addressbook.pickRandomNode();
			const auto& endpoint = hieroNode.pickRandomEndpoint();
			auto hieroServiceEndpointUrl = endpoint.getConnectionString();
			auto hieroClient = client::hiero::ConsensusClient::createForTarget(
				hieroServiceEndpointUrl, 
				endpoint.getPort() == hiero::PORT_NODE_TLS,
				hieroNode.getNodeCertHash()
			);
			if (!hieroClient) {
				LOG_F(ERROR, "Error connecting with hiero network via service endpoint: %s", hieroServiceEndpointUrl.c_str());
				return false;
			}
			LOG_F(INFO, "Hiero endpoint: %s (%s)",
				hieroServiceEndpointUrl.c_str(),
				hieroNode.getDescription().c_str()
			);
			hieroClients.push_back(hieroClient);
		}
	}

	if (!FileBasedProvider::getInstance()->init(getStopToken(), ServerGlobals::g_FilesPath + "/communities.json", std::move(hieroClients), hieroNodeCountPerCommunity)) {
		LOG_F(ERROR, "Error loading communities, please try to delete communities folders and try again!");
		return false;
	}
	if (!mMasterStopSource.stop_requested()) {
		// start jsonrpc 2.0 server
		mHttpServer = new Server("0.0.0.0", jsonrpc_port, "http-server");
		mHttpServer->init();
		// mHttpServer->registerResponseHandler("/api", new server::json_rpc::ApiHandlerFactory());
		mHttpServer->registerCallbackHandler("/api", MethodType::OPTIONS, [](const httplib::Request& req, httplib::Response& res) {
				ApiHandler::cors(res);
			}
		);
		mHttpServer->registerCallbackHandler("/api", MethodType::GET, [](const httplib::Request& req, httplib::Response& res) {
				res.set_content("REST API", "text/plain");
			}
		);
		mHttpServer->registerCallbackHandler("/api", MethodType::DEL, [](const httplib::Request& req, httplib::Response& res) {
				res.set_content("---", "text/plain");
			}
		);
		mHttpServer->registerCallbackHandler("/api", MethodType::POST, [](const httplib::Request& req, httplib::Response& res) {
				ApiHandler handler;
				handler.handlePostPut(req, res);
			}
		);
		mHttpServer->run();
		LOG_F(INFO, "started in %s, json rpc port: %d", usedTime.string().c_str(), jsonrpc_port);
	}
	else {
		LOG_F(INFO, "stopped before startup was finished in: %s", usedTime.string().c_str());
		return false;
	}	
	return true;
}

void MainServer::exit()
{
	LOG_F(INFO, "Running Tasks Count on shutdown: %lu", ServerGlobals::g_NumberExistingTasks.load());

	// stop worker scheduler
	// TODO: make sure that pending transaction are still write out to storage
	if (mHttpServer) {
		mHttpServer->exit();
		delete mHttpServer;
		mHttpServer = nullptr;
	}
	printf("[shutdown] Stopped HTTP Server...\n");

	// iota::MqttClientWrapper::getInstance()->exit();
	CacheManager::getInstance()->getFuzzyTimer()->stop();
	printf("[shutdown] Stopped Fuzzy Timer...\n");
	// ServerGlobals::g_IotaRequestCPUScheduler->stop();
	FileBasedProvider::getInstance()->exit();
	printf("[shutdown] Stopped File Based Provider...\n");
	ServerGlobals::g_CPUScheduler->stop();
	printf("[shutdown] Stopped CPU Scheduler...\n");
	ServerGlobals::g_WriteFileCPUScheduler->stop();
	printf("[shutdown] Stopped IO Worker...\n");
	ServerGlobals::clearMemory();
	printf("[shutdown] Cleared Memory...\n");
}

bool MainServer::configExists(const string& fileName) {
	return exists(fileName) && is_regular_file(fileName);
}

string MainServer::findConfigFile()
{
	// possible paths
	path currentPath = "gradido.yaml"; // current location
	path homePath = path(getHomeDir()) / ".gradido" / "gradido.yaml";
	
	// check paths
	if (configExists(currentPath.string())) {
		return currentPath.string();
	}
	else if (configExists(homePath.string())) {
		return homePath.string();
	}
	else {
		LOG_F(WARNING, "./gradido.yaml or ~/.gradido/gradido.yaml not found, using default values!");
	}
	return "";
}

string MainServer::getHomeDir()
{
#if defined(_WIN32) || defined(_WIN64)
	return path(getenv("USERPROFILE")).string(); // windows
#else 
	return path(getenv("HOME")).string(); // linux
#endif
}

