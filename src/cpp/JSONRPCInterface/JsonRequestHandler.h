#ifndef __JSON_INTERFACE_JSON_REQUEST_HANDLER_
#define __JSON_INTERFACE_JSON_REQUEST_HANDLER_

#include "Poco/Net/HTTPRequestHandler.h"
//#include "Poco/JSON/Object.h"

#include "../lib/ErrorList.h"

//#include "jsonrpcpp.hpp"


class JsonRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);

	//virtual Poco::JSON::Object* handle(Poco::Dynamic::Var params) = 0;
	//virtual void handle(const jsonrpcpp::Request& request, Json& response) = 0;

	//static Poco::Dynamic::Var parseJsonWithErrorPrintFile(std::istream& request_stream, ErrorList* errorHandler = nullptr, const char* functionName = nullptr);

protected:


};

#endif // __JSON_INTERFACE_JSON_REQUEST_HANDLER_