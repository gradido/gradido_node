#include "MainServer.h"
#include "ServerGlobals.h"

//#include "HTTPInterface/PageRequestHandlerFactory.h"
#include "JSONRPCInterface/JsonRequestHandlerFactory.h"

#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/http/ServerConfig.h"

#include "Poco/Util/HelpFormatter.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Environment.h"
#include "Poco/Logger.h"
#include "Poco/File.h"
#include "Poco/Path.h"
#include "Poco/AsyncChannel.h"
#include "Poco/SimpleFileChannel.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/SplitterChannel.h"

#include <sodium.h>
#include <iostream>

#include "loguru.hpp"

#include "SingletonManager/GroupManager.h"
#include "SingletonManager/OrderingManager.h"
#include "SingletonManager/CacheManager.h"
#include "iota/MqttClientWrapper.h"

MainServer::MainServer()
	: _helpRequested(false)
{
}

MainServer::~MainServer()
{
}


void MainServer::initialize(Application& self)
{
	loadConfiguration(); // load default configuration files, if present
	ServerApplication::initialize(self);
}

void MainServer::uninitialize()
{
	GroupManager::getInstance()->exit();
	ServerApplication::uninitialize();
}

void MainServer::defineOptions(Poco::Util::OptionSet& options)
{
	ServerApplication::defineOptions(options);

	options.addOption(
		Poco::Util::Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false));
}

void MainServer::handleOption(const std::string& name, const std::string& value)
{
	ServerApplication::handleOption(name, value);
	if (name == "help") _helpRequested = true;
}

void MainServer::displayHelp()
{
	Poco::Util::HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("OPTIONS");
	helpFormatter.setHeader("Gradido Node");
	helpFormatter.format(std::cout);
}

void MainServer::createConsoleFileAsyncLogger(std::string name, std::string filePath)
{
	Poco::AutoPtr<Poco::ConsoleChannel> logConsoleChannel(new Poco::ConsoleChannel);
	Poco::AutoPtr<Poco::SimpleFileChannel> logFileChannel(new Poco::SimpleFileChannel(filePath));
	logFileChannel->setProperty("rotation", "500 K");
	Poco::AutoPtr<Poco::SplitterChannel> logSplitter(new Poco::SplitterChannel);
	logSplitter->addChannel(logConsoleChannel);
	logSplitter->addChannel(logFileChannel);

	Poco::AutoPtr<Poco::AsyncChannel> logAsyncChannel(new Poco::AsyncChannel(logSplitter));

	Poco::Logger& log = Poco::Logger::get(name);
	log.setChannel(logAsyncChannel);
	log.setLevel("debug");
}

void MainServer::createFileAsyncLogger(std::string name, std::string filePath)
{
	Poco::AutoPtr<Poco::SimpleFileChannel> logFileChannel(new Poco::SimpleFileChannel(filePath));
	logFileChannel->setProperty("rotation", "500 K");
	Poco::AutoPtr<Poco::AsyncChannel> logAsyncChannel(new Poco::AsyncChannel(logFileChannel));

	Poco::Logger& log = Poco::Logger::get(name);
	log.setChannel(logAsyncChannel);
	log.setLevel("debug");
}

int MainServer::main(const std::vector<std::string>& args)
{
	Profiler usedTime;
	if (_helpRequested)
	{
		displayHelp();
	}
	else
	{
		// ********** logging ************************************
		std::string log_Path = "/var/log/grd_node/";
		//#ifdef _WIN32
#if defined(_WIN32) || defined(_WIN64)
		log_Path = "./";
#endif

		// init speed logger
		Poco::AutoPtr<Poco::SimpleFileChannel> speedLogFileChannel(new Poco::SimpleFileChannel(log_Path + "speedLog.txt"));
		/*
		The optional log file rotation mode:
		never:      no rotation (default)
		<n>:  rotate if file size exceeds <n> bytes
		<n> K:     rotate if file size exceeds <n> Kilobytes
		<n> M:    rotate if file size exceeds <n> Megabytes
		*/
		speedLogFileChannel->setProperty("rotation", "500 K");
		Poco::AutoPtr<Poco::AsyncChannel> speedLogAsyncChannel(new Poco::AsyncChannel(speedLogFileChannel));

		Poco::Logger& speedLogger = Poco::Logger::get("SpeedLog");
		speedLogger.setChannel(speedLogAsyncChannel);
		speedLogger.setLevel("information");

		// logging for request handling
		createConsoleFileAsyncLogger("requestLog", log_Path + "requestLog.txt");
		// logging for mqtt client 
		createConsoleFileAsyncLogger("mqttLog", log_Path + "mqttLog.txt");
		Poco::Logger& mqttLog = Poco::Logger::get("mqttLog");
		mqttLog.setLevel("information");

		// error logging
		createConsoleFileAsyncLogger("errorLog", log_Path + "errorLog.txt");
		Poco::Logger& errorLog = Poco::Logger::get("errorLog");
		errorLog.setLevel("debug");

		// info logging
		createConsoleFileAsyncLogger("infoLog", log_Path + "infoLog.txt");
		Poco::Logger& infoLogger = Poco::Logger::get("infoLog");
		infoLogger.setLevel("debug");

		createFileAsyncLogger("logTransactions", log_Path + "receivedTransactions.txt");

		// *************** load from config ********************************************

		std::string cfg_Path = Poco::Path::home() + ".gradido" + Poco::Path::separator();
		try {
			loadConfiguration(cfg_Path + "gradido.properties");
		}
		catch (Poco::Exception& ex) {
			errorLog.error("error loading config: %s", ex.displayText());
		}
		unsigned short jsonrpc_port = (unsigned short)config().getInt("JSONRPCServer.port", 8340);
		
		// timeouts
		ServerGlobals::loadTimeouts(config());
		ServerGlobals::g_LogTransactions = config().getBool("LogTransactions", ServerGlobals::g_LogTransactions);
		if (ServerGlobals::g_LogTransactions) {
			Poco::Logger::get("logTransactions").setLevel("information");
		}

		ServerGlobals::g_FilesPath = Poco::Path::home() + ".gradido";
		Poco::File homeFolder(ServerGlobals::g_FilesPath);
		if (!homeFolder.exists()) {
			homeFolder.createDirectory();
		}

		std::string cacertPath = Poco::Path::home() + ".gradido" + Poco::Path::separator() + "cacert.pem";
		if (!ServerConfig::initSSLClientContext(cacertPath.data())) {
			//return Application::EXIT_CANTCREAT;
			printf("WARNING!!!: Problems with ssl, cannot request data from ssl servers\n");
		}
		ServerGlobals::initIota(config());
		ServerConfig::readUnsecureFlags(config());		

		// start cpu scheduler
		uint8_t worker_count = Poco::Environment::processorCount() * 2;
		// I think 1 or 2 by HDD is ok, more by SSD, but should be profiled on work load
		uint8_t io_worker_count = config().getInt("io.workerCount", 2);
		ServerGlobals::g_CPUScheduler = new task::CPUSheduler(worker_count, "Default Worker");
		ServerGlobals::g_WriteFileCPUScheduler = new task::CPUSheduler(io_worker_count, "IO Worker");
		ServerGlobals::g_IotaRequestCPUScheduler = new task::CPUSheduler(2, "Iota Worker");
		iota::MqttClientWrapper::getInstance()->init();
		
		auto gm = GroupManager::getInstance();
		if (GroupManager::getInstance()->init("group.index", config())) {
			std::clog << "Error loading group, please try to delete group folders and try again!" << std::endl;
			return Application::EXIT_DATAERR;
		}
		

		OrderingManager::getInstance();

		// HTTP Interface Server
		// set-up a server socket
		/* Poco::Net::ServerSocket svs(port);
		// set-up a HTTPServer instance
		Poco::ThreadPool& pool = Poco::ThreadPool::defaultPool();
		Poco::Net::HTTPServer srv(new PageRequestHandlerFactory, svs, new Poco::Net::HTTPServerParams);

		// start the HTTPServer
		srv.start();
		*/

		// JSON Interface Server
		Poco::Net::ServerSocket jsonrpc_svs(jsonrpc_port);
		Poco::Net::HTTPServer jsonrpc_srv(new JsonRequestHandlerFactory, jsonrpc_svs, new Poco::Net::HTTPServerParams);

		// start the json server
		jsonrpc_srv.start();

		std::clog << "[Gradido_Node::main] started in " << usedTime.string().data() << ", jsonrpc on port: " << jsonrpc_port << std::endl;
		//printf("[Gradido_Node::main] started in %s, jsonrpc on port: %d\n", usedTime.string().data(), jsonrpc_port);
		// wait for CTRL-C or kill
		waitForTerminationRequest();


		// Stop the HTTPServer
		//srv.stop();
		// Stop the json server
		jsonrpc_srv.stop();

		std::clog << "[Gradido_Node::main] Running Tasks Count on shutdown: " << std::to_string(ServerGlobals::g_NumberExistingTasks.value()) << std::endl;

		// stop worker scheduler
		// TODO: make sure that pending transaction are still write out to storage
		CacheManager::getInstance()->getFuzzyTimer()->stop();
		ServerGlobals::g_CPUScheduler->stop();
		ServerGlobals::g_WriteFileCPUScheduler->stop();
		ServerGlobals::g_IotaRequestCPUScheduler->stop();
	}
	return Application::EXIT_OK;
}

