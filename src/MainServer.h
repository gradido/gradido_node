#ifndef MAIN_SERVER_INCLUDED
#define MAIN_SERVER_INCLUDED

#include "gradido_blockchain/Application.h"
#include "gradido_blockchain/http/Server.h"

#include <string>

namespace client {
	namespace grpc {
		class Client;
	}
}

class MainServer : public Application
{

	/// The main application class.
	///
	/// This class handles command-line arguments and
	/// configuration files.
	/// Start the Gradido_LoginServer executable with the help
	/// option (/help on Windows, --help on Unix) for
	/// the available command line options.
	///


public:
	MainServer();
	~MainServer();

protected:
	bool init();
	void exit();

	std::string findConfigFile();
	std::string getHomeDir();
	bool configExists(const std::string& fileName);

	Server* mHttpServer;
	std::shared_ptr<client::grpc::Client> mHieroClient;
private:
};

#endif //MAIN_SERVER_INCLUDED
