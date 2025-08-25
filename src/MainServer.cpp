#include "MainServer.h"
#include "ServerGlobals.h"

#include "blockchain/FileBasedProvider.h"
#include "iota/MqttClientWrapper.h"
#include "server/json-rpc/ApiHandlerFactory.h"
#include "SingletonManager/OrderingManager.h"
#include "SingletonManager/CacheManager.h"

#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/http/ServerConfig.h"

// Hiero
#include "Client.h"

#include <algorithm>
#include <sodium.h>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "loguru.hpp"

using namespace gradido;
using namespace blockchain;
namespace fs = std::filesystem;

MainServer::MainServer()
	: mHttpServer(nullptr), mHieroClient(nullptr)
{
}

MainServer::~MainServer()
{
}

bool MainServer::init()
{
	Profiler usedTime;
	ServerGlobals::g_FilesPath = getHomeDir() + "/.gradido";
	fs::create_directories(ServerGlobals::g_FilesPath);

	// ********** logging ************************************
	std::string logPath = ServerGlobals::g_FilesPath + "/logs";
	fs::create_directories(logPath);
	// beware, no logrotation in loguru
	// TODO: add config options to choose which to use
	// TODO: check switching to https://github.com/gabime/spdlog
	std::string errorLogFile = logPath + "/errors.log";
	loguru::add_file(errorLogFile.data(), loguru::Append, loguru::Verbosity_WARNING);
	// info 
	std::string infoLogFile = logPath + "/infos.log";
	loguru::add_file(infoLogFile.data(), loguru::Append, loguru::Verbosity_INFO);
	// infos and above
	std::string debugLogFile = logPath + "/debug.log";
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

	// timeouts
	ServerGlobals::loadTimeouts(config);
	ServerGlobals::g_LogTransactions = config.getBool("logging.log_transactions", ServerGlobals::g_LogTransactions);
	ServerGlobals::initIota(config);
	ServerConfig::readUnsecureFlags(config);

	// start cpu scheduler
	// std::thread::hardware_concurrency() sometime return 0 if number couldn't be determined
	uint8_t worker_count = std::min(2, (int)std::thread::hardware_concurrency() * 2);
	// I think 1 or 2 by HDD is ok, more by SSD, but should be profiled on work load
	uint8_t io_worker_count = config.getInt("io.worker_count", 2);
	ServerGlobals::g_CPUScheduler = new task::CPUSheduler(worker_count, "Default Worker");
	ServerGlobals::g_WriteFileCPUScheduler = new task::CPUSheduler(io_worker_count, "IO Worker");
	ServerGlobals::g_IotaRequestCPUScheduler = new task::CPUSheduler(2, "Iota Worker");
	if(!ServerGlobals::g_isOfflineMode) {
		iota::MqttClientWrapper::getInstance()->init();
	}

	if (!FileBasedProvider::getInstance()->init(ServerGlobals::g_FilesPath + "/communities.json")) {
		LOG_F(ERROR, "Error loading communities, please try to delete communities folders and try again!");
		return false;
	}
	OrderingManager::getInstance()->init();

	// Hiero Client
	mHieroClient = new Hiero::Client(
		std::move(
			Hiero::Client::forName(
				config.getString("hiero.network_type", "testnet")
			)
		)
	);

	// JSON Interface Server
	mHttpServer = new Server("0.0.0.0", jsonrpc_port, "http-server");
	mHttpServer->init();
	mHttpServer->registerResponseHandler("/api", new server::json_rpc::ApiHandlerFactory());
	mHttpServer->run();
	LOG_F(INFO, "started in %s, json rpc port: %d", usedTime.string().data(), jsonrpc_port);
	// start the json server
	// doesn't return
	return true;
}

void MainServer::exit()
{
	LOG_F(INFO, "Running Tasks Count on shutdown: %llu", ServerGlobals::g_NumberExistingTasks.load());

	// stop worker scheduler
	// TODO: make sure that pending transaction are still write out to storage
	if (mHttpServer) {
		mHttpServer->exit();
		delete mHttpServer;
		mHttpServer = nullptr;
	}

	if (mHieroClient) {
		mHieroClient->close();
		delete mHieroClient;
		mHieroClient = nullptr;
	}

	iota::MqttClientWrapper::getInstance()->exit();
	CacheManager::getInstance()->getFuzzyTimer()->stop();
	ServerGlobals::g_CPUScheduler->stop();
	ServerGlobals::g_WriteFileCPUScheduler->stop();
	ServerGlobals::g_IotaRequestCPUScheduler->stop();
	FileBasedProvider::getInstance()->exit();
}

bool MainServer::configExists(const std::string& fileName) {
	return fs::exists(fileName) && fs::is_regular_file(fileName);
}

std::string MainServer::findConfigFile()
{
	// possible paths
	fs::path currentPath = "gradido.yaml"; // current location
	fs::path homePath = fs::path(getHomeDir()) / ".gradido" / "gradido.yaml";
	
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

std::string MainServer::getHomeDir()
{
#if defined(_WIN32) || defined(_WIN64)
	return fs::path(getenv("USERPROFILE")).string(); // windows
#else 
	return fs::path(getenv("HOME")).string(); // linux
#endif
}

