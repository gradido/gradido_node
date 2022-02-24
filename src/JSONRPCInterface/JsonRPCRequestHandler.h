#ifndef __JSON_INTERFACE_JSON_REQUEST_HANDLER_
#define __JSON_INTERFACE_JSON_REQUEST_HANDLER_

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/URI.h"
#include "Poco/Logger.h"

#include "rapidjson/document.h"

class JsonRPCRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	JsonRPCRequestHandler();

	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);

	//virtual Poco::JSON::Object* handle(Poco::Dynamic::Var params) = 0;
	//virtual void handle(const jsonrpcpp::Request& request, Json& response) = 0;
	virtual void handle(std::string method, const rapidjson::Value& params) { };

	void parseJsonWithErrorPrintFile(std::istream& request_stream, rapidjson::Document& rapidParams);
	static bool parseQueryParametersToRapidjson(const Poco::URI& uri, rapidjson::Document& rapidParams);

	bool getIntParameter(const rapidjson::Value& params, const char* fieldName, int& iParameter);
	bool getUIntParameter(const rapidjson::Value& params, const char* fieldName, unsigned int& iParameter);
	bool getBoolParameter(const rapidjson::Value& params, const char* fieldName, bool& bParameter);
	bool getUInt64Parameter(const rapidjson::Value& params, const char* fieldName, Poco::UInt64& iParameter);
	bool getStringParameter(const rapidjson::Value& params, const char* fieldName, std::string& strParameter);
	bool getStringIntParameter(const rapidjson::Value& params, const char* fieldName, std::string& strParameter, int& iParameter);
	bool checkArrayParameter(const rapidjson::Value& params, const char* fieldName);
	bool checkObjectParameter(const rapidjson::Value& params, const char* fieldName);
	bool checkObjectOrArrayParameter(const rapidjson::Value& params, const char* fieldName);

	void stateError(const char* msg, std::string details = "");
	void customStateError(const char* state, const char* msg, std::string details = "");
	void stateSuccess();
	void stateWarning(const char* msg, std::string details = "");

protected:
	rapidjson::Document mResponseJson;
	rapidjson::Value    mResponseResult;


};

#endif // __JSON_INTERFACE_JSON_REQUEST_HANDLER_