#include "JsonRPCRequestHandler.h"

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/DeflatingStream.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/istreamwrapper.h"

#include "../SingletonManager/LoggerManager.h"


//#include "Poco/URI.h"
//#include "Poco/DeflatingStream.h"

//#include <exception>
#include <memory>
#include <sstream>

using namespace rapidjson;

JsonRPCRequestHandler::JsonRPCRequestHandler()
	: mRootJson(kObjectType)
{

}

Value JsonRPCRequestHandler::handleOneRpcCall(const rapidjson::Value& jsonRpcRequest)
{
	Value responseJson(kObjectType);
	std::string method;
	auto alloc = mRootJson.GetAllocator();

	if (checkObjectOrArrayParameter(responseJson, jsonRpcRequest, "params") && getStringParameter(responseJson, jsonRpcRequest, "method", method)) {
		try {
			responseJson = handle(method, jsonRpcRequest["params"]);
		}
		catch (GradidoBlockchainException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("gradido node exception in Json RPC handle: %s", ex.getFullString());
			error(responseJson, JSON_RPC_ERROR_GRADIDO_NODE_ERROR, "gradido node intern exception");
		}
		catch (Poco::Exception& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("Poco exception in Json RPC handle: %s", ex.displayText());
			error(responseJson, JSON_RPC_ERROR_GRADIDO_NODE_ERROR, "gradido node intern exception");
		}
		catch (std::exception& ex) {
			std::string what(ex.what());
			LoggerManager::getInstance()->mErrorLogging.error("exception in Json RPC handle: %s", what);
			error(responseJson, JSON_RPC_ERROR_GRADIDO_NODE_ERROR, "gradido node intern exception");
		}
	}

	responseJson.AddMember("jsonrpc", "2.0", alloc);
	if (jsonRpcRequest.HasMember("id")) {
		responseJson.AddMember("id", Value(jsonRpcRequest["id"], alloc), alloc);
	}
	else {
		responseJson.AddMember("id", Value(kNullType), alloc);
	}
	return responseJson;
}

void JsonRPCRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
	response.setChunkedTransferEncoding(false);
	response.setContentType("application/json");

	bool _compressResponse(request.hasToken("Accept-Encoding", "gzip"));
	if (_compressResponse) response.set("Content-Encoding", "gzip");

	std::ostream& _responseStream = response.send();
	Poco::DeflatingOutputStream _gzipStream(_responseStream, Poco::DeflatingStreamBuf::STREAM_GZIP, 1);
	std::ostream& responseStream = _compressResponse ? _gzipStream : _responseStream;

	auto method = request.getMethod();
	std::istream& request_stream = request.stream();

	auto alloc = mRootJson.GetAllocator();
	Value responseJson(kObjectType);
	responseJson.AddMember("jsonrpc", "2.0", alloc);
	responseJson.AddMember("id", Value(kNullType), alloc);

	// TODO: put group name in request url to keep function calls as similar as possible to bitcoin and co
	Document rapidjson_params;
	if (method == "POST" || method == "PUT") 
	{
		IStreamWrapper request_stream_wrapped(request_stream);
		rapidjson_params.ParseStream(request_stream_wrapped);
		if (rapidjson_params.HasParseError()) 
		{
			Value data(kObjectType);
			data.AddMember("parseError", Value(GetParseError_En(rapidjson_params.GetParseError()), alloc), alloc);
			data.AddMember("errorOffset", rapidjson_params.GetErrorOffset(), alloc);
			error(responseJson, JSON_RPC_ERROR_PARSE_ERROR, "error parsing request to json", data);
		}
		else 
		{
			if (rapidjson_params.IsObject()) {
				responseJson = handleOneRpcCall(rapidjson_params);
			}
			else if (rapidjson_params.IsArray()) {
				Value responseJson = Value(kArrayType);
				for (auto& v : rapidjson_params.GetArray()) {
					responseJson.PushBack(handleOneRpcCall(v), alloc);
				}
			}
			else {
				error(responseJson, JSON_RPC_ERROR_INVALID_REQUEST, "empty body");
			}
		}
	}
	else if (method == "GET") 
	{
		Poco::URI uri(request.getURI());
		parseQueryParametersToRapidjson(uri, rapidjson_params);

		//rapid_json_result = handle(rapidjson_params);
		printf("[%s] must be implemented\n", __FUNCTION__);
	}
	else {
		error(responseJson, JSON_RPC_ERROR_INVALID_REQUEST, "HTTP method unknown");
	}

	// 3. Stringify the DOM
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	responseJson.Accept(writer);	
	responseStream << buffer.GetString() << std::endl;	
	
	if (_compressResponse) _gzipStream.close();	
}


bool JsonRPCRequestHandler::parseQueryParametersToRapidjson(const Poco::URI& uri, Document& rapidParams)
{
	auto queryParameters = uri.getQueryParameters();
	rapidParams.SetObject();
	for (auto it = queryParameters.begin(); it != queryParameters.end(); it++) {
		int tempi = 0;
		Value name_field(it->first.data(), rapidParams.GetAllocator());
		if (DataTypeConverter::NUMBER_PARSE_OKAY == DataTypeConverter::strToInt(it->second, tempi)) {
			//rapidParams[it->first.data()] = rapidjson::Value(tempi, rapidParams.GetAllocator());
			rapidParams.AddMember(name_field.Move(), tempi, rapidParams.GetAllocator());
		}
		else {
			rapidParams.AddMember(name_field.Move(), Value(it->second.data(), rapidParams.GetAllocator()), rapidParams.GetAllocator());
		}
	}

	return true;
}

void JsonRPCRequestHandler::error(Value& responseJson, JsonRPCErrorCodes code, const char* message, rapidjson::Value& data/* = rapidjson::Value()*/)
{
	Value error(kObjectType);
	auto alloc = mRootJson.GetAllocator();
	error.AddMember("code", code, alloc);
	error.AddMember("message", Value(message, alloc), alloc);
	if (!data.IsNull()) {
		error.AddMember("data", data, alloc);
	}
	responseJson.AddMember("error", error, alloc);
}
void JsonRPCRequestHandler::error(Value& responseJson, JsonRPCErrorCodes code, GradidoBlockchainException& ex)
{
	error(responseJson, code, ex.getFullString().data(), ex.getDetails(mRootJson.GetAllocator()));
}


bool JsonRPCRequestHandler::getIntParameter(Value& responseJson, const Value& params, const char* fieldName, int& iParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	if (!itr->value.IsInt()) {
		message = "invalid " + message;
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	iParameter = itr->value.GetInt();
	return true;
}

bool JsonRPCRequestHandler::getBoolParameter(Value& responseJson, const Value& params, const char* fieldName, bool& bParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	if (!itr->value.IsBool()) {
		message = "invalid " + message;
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	bParameter = itr->value.GetBool();
	return true;
}

bool JsonRPCRequestHandler::getUIntParameter(Value& responseJson, const Value& params, const char* fieldName, unsigned int& iParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	if (!itr->value.IsUint()) {
		message = "invalid " + message;
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	iParameter = itr->value.GetUint();
	return true;
}

bool JsonRPCRequestHandler::getUInt64Parameter(Value& responseJson, const Value& params, const char* fieldName, Poco::UInt64& iParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	if (!itr->value.IsUint64()) {
		message = "invalid " + message;
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	iParameter = itr->value.GetUint64();
	return true;
}
bool JsonRPCRequestHandler::getStringParameter(Value& responseJson, const Value& params, const char* fieldName, std::string& strParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	if (!itr->value.IsString()) {
		message = "invalid " + message;
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	strParameter = std::string(itr->value.GetString(), itr->value.GetStringLength());
	return true;
}

bool JsonRPCRequestHandler::getStringIntParameter(Value& responseJson, const Value& params, const char* fieldName, std::string& strParameter, int& iParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}
	if (itr->value.IsString()) {
		strParameter = std::string(itr->value.GetString(), itr->value.GetStringLength());
	}
	else if (itr->value.IsInt()) {
		iParameter = itr->value.GetInt();
	}
	else {
		message += " isn't neither int or string";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}

	return true;
}

bool JsonRPCRequestHandler::checkArrayParameter(Value& responseJson, const Value& params, const char* fieldName)
{

	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}

	if (!itr->value.IsArray()) {
		message += " is not a array";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}

	return true;
}

bool JsonRPCRequestHandler::checkObjectParameter(Value& responseJson, const Value& params, const char* fieldName)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}

	if (!itr->value.IsObject()) {
		message += " is not a object";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}

	return true;
}

bool JsonRPCRequestHandler::checkObjectOrArrayParameter(Value& responseJson, const Value& params, const char* fieldName)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}

	if (!itr->value.IsObject() && !itr->value.IsArray()) {
		message += " is not a object or array";
		error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
		return false;
	}

	return true;
}


