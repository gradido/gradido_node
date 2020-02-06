#include "MainServer.h"
#include "ServerGlobals.h"

//#include "HTTPInterface/PageRequestHandlerFactory.h"
#include "JSONRPCInterface/JsonRequestHandlerFactory.h"

#include "lib/Profiler.h"

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
#include <google/protobuf/stubs/common.h>

#include "SingletonManager/GroupManager.h"

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
	log.setLevel("information");
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

		// error logging
		createConsoleFileAsyncLogger("errorLog", log_Path + "errorLog.txt");
		Poco::Logger& errorLog = Poco::Logger::get("errorLog");

		// *************** load from config ********************************************

		std::string cfg_Path = Poco::Path::config() + "grd_node/";
		try {
			loadConfiguration(cfg_Path + "grd_node.properties");
		}
		catch (Poco::Exception& ex) {
			errorLog.error("error loading config: %s", ex.displayText());
		}

		unsigned short port = (unsigned short)config().getInt("HTTPServer.port", 9970);
		unsigned short jsonrpc_port = (unsigned short)config().getInt("JSONRPCServer.port", 8340);
		unsigned short tcp_port = (unsigned short)config().getInt("TCPServer.port", 8341);


		
		// start cpu scheduler
		uint8_t worker_count = Poco::Environment::processorCount() * 2;

		ServerGlobals::g_FilesPath = Poco::Path::home() + ".gradido";
		Poco::File homeFolder(ServerGlobals::g_FilesPath);
		if (!homeFolder.exists()) {
			homeFolder.createDirectory();
		}
		
		ServerGlobals::g_CPUScheduler = new UniLib::controller::CPUSheduler(worker_count, "Default Worker");

		GroupManager::getInstance()->init("group.index");
		

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

		printf("[Gradido_LoginServer::main] started in %s, jsonrpc on port: %d\n", usedTime.string().data(), jsonrpc_port);
		// wait for CTRL-C or kill
		waitForTerminationRequest();

		// Stop the HTTPServer
		//srv.stop();
		// Stop the json server
		jsonrpc_srv.stop();

		// Optional:  Delete all global objects allocated by libprotobuf.
		google::protobuf::ShutdownProtobufLibrary();

	}
	return Application::EXIT_OK;
}

