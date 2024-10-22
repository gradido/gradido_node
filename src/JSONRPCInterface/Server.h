#ifndef __GRADIDO_NODE_JSON_RPC_INTERFACE_SERVER_H
#define __GRADIDO_NODE_JSON_RPC_INTERFACE_SERVER_H

#include <memory>

namespace httplib {
	class Server;
}

class Server
{
public: 
	Server();
	~Server();

	bool run(const char* host = "0.0.0.0", int port = 8340);
protected:
	std::unique_ptr<httplib::Server> mServer;

};

#endif //__GRADIDO_NODE_JSON_RPC_INTERFACE_SERVER_H