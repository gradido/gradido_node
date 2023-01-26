#ifndef __JSON_INTERFACE_JSON_REQUEST_HANDLER_
#define __JSON_INTERFACE_JSON_REQUEST_HANDLER_

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/URI.h"
#include "Poco/Logger.h"

#include "gradido_blockchain/GradidoBlockchainException.h"

#include "rapidjson/document.h"

enum JsonRPCErrorCodes : int
{
	JSON_RPC_ERROR_NONE = 0,
	JSON_RPC_ERROR_GRADIDO_NODE_ERROR = -10000,
	JSON_RPC_ERROR_UNKNOWN_GROUP = -10001,
	JSON_RPC_ERROR_NOT_IMPLEMENTED = -10002,
	// default errors from json rpc standard: https://www.jsonrpc.org/specification
	// -32700 	Parse error 	Invalid JSON was received by the server.
	JSON_RPC_ERROR_PARSE_ERROR = -32700,
	// -32600 	Invalid Request The JSON sent is not a valid Request object.
	JSON_RPC_ERROR_INVALID_REQUEST = -32600,
	// -32601 	Method not found 	The method does not exist / is not available.
	JSON_RPC_ERROR_METHODE_NOT_FOUND = -32601,
	// -32602 	Invalid params 	Invalid method parameter(s).
	JSON_RPC_ERROR_INVALID_PARAMS = -32602,
	// -32603 	Internal error 	Internal JSON - RPC error.
	JSON_RPC_ERROR_INTERNAL_ERROR = -32603,
	// -32000 to -32099 	Server error 	Reserved for implementation-defined server-errors.

};

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

	bool getIntParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, int& iParameter);
	bool getUIntParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, unsigned int& iParameter);
	bool getBoolParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, bool& bParameter);
	bool getUInt64Parameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, Poco::UInt64& iParameter);
	bool getStringParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, std::string& strParameter);
	bool getStringIntParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName, std::string& strParameter, int& iParameter);
	bool checkArrayParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName);
	bool checkObjectParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName);
	bool checkObjectOrArrayParameter(rapidjson::Value& responseJson, const rapidjson::Value& params, const char* fieldName);

	void error(rapidjson::Value& responseJson, JsonRPCErrorCodes code, const char* message, rapidjson::Value& data = rapidjson::Value());
	void error(rapidjson::Value& responseJson, JsonRPCErrorCodes code, GradidoBlockchainException& ex);

protected:
	rapidjson::Document mRootJson;

};

#endif // __JSON_INTERFACE_JSON_REQUEST_HANDLER_