#ifndef MAIN_SERVER_INCLUDED
#define MAIN_SERVER_INCLUDED

#include "gradido_blockchain/ServerApplication.h"

#include <string>

class MainServer : public ServerApplication
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


private:
};

#endif //MAIN_SERVER_INCLUDED
