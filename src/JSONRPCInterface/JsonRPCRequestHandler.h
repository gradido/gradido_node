#ifndef __JSON_INTERFACE_JSON_REQUEST_HANDLER_
#define __JSON_INTERFACE_JSON_REQUEST_HANDLER_

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/URI.h"
#include "Poco/Logger.h"

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/http/JsonRPCRequest.h"
#include "gradido_blockchain/MemoryManager.h"

#include "rapidjson/document.h"


class JsonRPCRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	JsonRPCRequestHandler();

	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
	rapidjson::Value handleOneRpcCall(const rapidjson::Value& jsonRpcRequest);

	//virtual Poco::JSON::Object* handle(Poco::Dynamic::Var params) = 0;
	//virtual void handle(const jsonrpcpp::Request& request, Json& response) = 0;
	virtual void handle(rapidjson::Value& responseJson, std::string method, const rapidjson::Value& params) { };
	
	static bool parseQueryParametersToRapidjson(const Poco::URI& uri, rapidjson::Document& rapidParams);

	//* \param optional if set to true, no error message will be generated

	bool getIntParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, int& iParameter, bool optional = false);
	bool getUIntParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, unsigned int& iParameter, bool optional = false);
	bool getBoolParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, bool& bParameter, bool optional = false);
	bool getUInt64Parameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, Poco::UInt64& iParameter, bool optional = false);
	bool getStringParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, std::string& strParameter, bool optional = false);
	bool getBinaryFromHexStringParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, MemoryBin** binaryParameter, bool optional = false);
	bool getStringIntParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, std::string& strParameter, int& iParameter, bool optional = false);
	bool checkArrayParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, bool optional = false);
	bool checkObjectParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, bool optional = false);
	bool checkObjectOrArrayParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, bool optional = false);

	void error(rapidjson::Value& responseJson, JsonRPCErrorCodes code, const char* message, rapidjson::Value* data = nullptr);
	void error(rapidjson::Value& responseJson, JsonRPCErrorCodes code, GradidoBlockchainException& ex);

protected:
	rapidjson::Document mRootJson;

};

#endif // __JSON_INTERFACE_JSON_REQUEST_HANDLER_