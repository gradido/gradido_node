#include "JsonRPCRequestHandler.h"

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "../SingletonManager/LoggerManager.h"

//#include "Poco/URI.h"
//#include "Poco/DeflatingStream.h"

//#include <exception>
#include <memory>
#include <sstream>

using namespace rapidjson;

JsonRPCRequestHandler::JsonRPCRequestHandler()
	: mResponseJson(kObjectType), mResponseResult(kObjectType), mResponseErrorCode(JSON_RPC_ERROR_NONE)
{

}

void JsonRPCRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{

	response.setChunkedTransferEncoding(false);
	response.setContentType("application/json");
	//bool _compressResponse(request.hasToken("Accept-Encoding", "gzip"));
	//if (_compressResponse) response.set("Content-Encoding", "gzip");

	std::ostream& responseStream = response.send();
	//Poco::DeflatingOutputStream _gzipStream(_responseStream, Poco::DeflatingStreamBuf::STREAM_GZIP, 1);
	//std::ostream& responseStream = _compressResponse ? _gzipStream : _responseStream;

	auto method = request.getMethod();
	std::istream& request_stream = request.stream();

	int id = 0;
	// TODO: put group name in request url to keep function calls as similar as possible to bitcoin and co
	Document rapidjson_params;
	if (method == "POST" || method == "PUT") {
		// extract parameter from request
		parseJsonWithErrorPrintFile(request_stream, rapidjson_params);
		getIntParameter(rapidjson_params, "id", id);

		if (rapidjson_params.IsObject()) {
			bool isParams = checkObjectOrArrayParameter(rapidjson_params, "params");
			std::string method;
			bool isMethod = getStringParameter(rapidjson_params, "method", method);
			if (isParams && isMethod) {
				try {
					handle(method, rapidjson_params["params"]);
				}
				catch (GradidoBlockchainException& ex) {
					mResponseErrorCode = JSON_RPC_ERROR_GRADIDO_NODE_ERROR;
					LoggerManager::getInstance()->mErrorLogging.error("gradido node exception in Json RPC handle: %s", ex.getFullString());
					stateError("gradido node intern exception");
				}
				catch (Poco::Exception& ex) {
					mResponseErrorCode = JSON_RPC_ERROR_GRADIDO_NODE_ERROR;
					LoggerManager::getInstance()->mErrorLogging.error("Poco exception in Json RPC handle: %s", ex.displayText());
					stateError("gradido node intern exception");
				}
				catch (std::exception& ex) {
					mResponseErrorCode = JSON_RPC_ERROR_GRADIDO_NODE_ERROR;
					std::string what(ex.what());
					LoggerManager::getInstance()->mErrorLogging.error("exception in Json RPC handle: %s", what);
					stateError("gradido node intern exception");
				}
			}
		}
		else {
			stateError("empty body");
		}
	}
	else if (method == "GET") {
		Poco::URI uri(request.getURI());
		parseQueryParametersToRapidjson(uri, rapidjson_params);

		//rapid_json_result = handle(rapidjson_params);
		printf("[%s] must be implemented\n", __FUNCTION__);
	}
	else {
		responseStream << "{\"result\":{\"state\":\"error\", \"msg\":\"no post or put request\"}}";
	}

	if (!mResponseJson.IsNull()) {
		// 3. Stringify the DOM
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		auto alloc = mResponseJson.GetAllocator();
		if (mResponseErrorCode) {
			Value error(kObjectType);
			error.AddMember("code", mResponseErrorCode, alloc);
			error.AddMember("message", mResponseResult["msg"], alloc);
			if (mResponseResult.HasMember("details")) {
				error.AddMember("data", mResponseResult["details"], alloc);
			}
			mResponseJson.AddMember("error", error, alloc);
		}
		else {
			mResponseJson.AddMember("result", mResponseResult, alloc);
		}
		mResponseJson.AddMember("jsonrpc", "2.0", alloc);
		mResponseJson.AddMember("id", id, alloc);
		mResponseJson.Accept(writer);

		//printf("result: %s\n", buffer.GetString());

		responseStream << buffer.GetString() << std::endl;
	}
	else {
		responseStream << "{\"result\":{\"state\":\"error\", \"msg\":\"no result\"}}";
	}
	
	//if (_compressResponse) _gzipStream.close();
	
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

void JsonRPCRequestHandler::parseJsonWithErrorPrintFile(std::istream& request_stream, Document& rapidParams)
{
	// debugging answer

	std::stringstream responseStringStream;
	for (std::string line; std::getline(request_stream, line); ) {
		responseStringStream << line << std::endl;
	}

	rapidParams.Parse(responseStringStream.str().data());
	if (rapidParams.HasParseError()) {
		throw RapidjsonParseErrorException(
			"JsonRPCRequestHandler::parseJsonWithErrorPrintFile",
			rapidParams.GetParseError(), 
			rapidParams.GetErrorOffset()
		).setRawText(responseStringStream.str());
	}
}

void JsonRPCRequestHandler::stateError(const char* msg, std::string details)
{
	// clear prev error states
	mResponseResult.RemoveMember("state");
	mResponseResult.RemoveMember("msg");
	mResponseResult.RemoveMember("details");

	auto alloc = mResponseJson.GetAllocator();
	mResponseResult.AddMember("state", "error", alloc);
	mResponseResult.AddMember("msg", Value(msg, alloc), alloc);

	if (details.size()) {
		mResponseResult.AddMember("details", Value(details.data(), alloc), alloc);
	}
}

void JsonRPCRequestHandler::stateError(const char* msg, GradidoBlockchainException& ex)
{
	// clear prev error states
	mResponseResult.RemoveMember("state");
	mResponseResult.RemoveMember("msg");
	mResponseResult.RemoveMember("details");

	auto alloc = mResponseJson.GetAllocator();
	mResponseResult.AddMember("state", "error", alloc);
	mResponseResult.AddMember("msg", Value(msg, alloc), alloc);
	mResponseResult.AddMember("details", ex.getDetails(alloc), alloc);
}


void JsonRPCRequestHandler::stateSuccess()
{	
	// clear error states
	mResponseResult.RemoveMember("state");
	mResponseResult.RemoveMember("msg");
	mResponseResult.RemoveMember("details");
	
	mResponseResult.AddMember("state", "success", mResponseJson.GetAllocator());
}


void JsonRPCRequestHandler::customStateError(const char* state, const char* msg, std::string details /* = "" */)
{
	auto alloc = mResponseJson.GetAllocator();
	mResponseResult.AddMember("state", Value(state, alloc).Move(), alloc);
	mResponseResult.AddMember("msg", Value(msg, alloc).Move(), alloc);

	if (details.size()) {
		mResponseResult.AddMember("details", Value(details.data(), alloc).Move(), alloc);
	}
}


void JsonRPCRequestHandler::stateWarning(const char* msg, std::string details/* = ""*/)
{
	auto alloc = mResponseJson.GetAllocator();
	mResponseResult.AddMember("state", "warning", alloc);
	mResponseResult.AddMember("msg", Value(msg, alloc).Move(), alloc);

	if (details.size()) {
		mResponseResult.AddMember("details", Value(details.data(), alloc).Move(), alloc);
	}
}



bool JsonRPCRequestHandler::getIntParameter(const rapidjson::Value& params, const char* fieldName, int& iParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		stateError(message.data());
		return false;
	}
	if (!itr->value.IsInt()) {
		message = "invalid " + message;
		stateError(message.data());
		return false;
	}
	iParameter = itr->value.GetInt();
	return true;
}

bool JsonRPCRequestHandler::getBoolParameter(const rapidjson::Value& params, const char* fieldName, bool& bParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		stateError(message.data());
		return false;
	}
	if (!itr->value.IsBool()) {
		message = "invalid " + message;
		stateError(message.data());
		return false;
	}
	bParameter = itr->value.GetBool();
	return true;
}

bool JsonRPCRequestHandler::getUIntParameter(const rapidjson::Value& params, const char* fieldName, unsigned int& iParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		stateError(message.data());
		return false;
	}
	if (!itr->value.IsUint()) {
		message = "invalid " + message;
		stateError(message.data());
		return false;
	}
	iParameter = itr->value.GetUint();
	return true;
}

bool JsonRPCRequestHandler::getUInt64Parameter(const rapidjson::Value& params, const char* fieldName, Poco::UInt64& iParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		stateError(message.data());
		return false;
	}
	if (!itr->value.IsUint64()) {
		message = "invalid " + message;
		stateError(message.data());
		return false;
	}
	iParameter = itr->value.GetUint64();
	return true;
}
bool JsonRPCRequestHandler::getStringParameter(const rapidjson::Value& params, const char* fieldName, std::string& strParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		stateError(message.data());
		return false;
	}
	if (!itr->value.IsString()) {
		message = "invalid " + message;
		stateError(message.data());
		return false;
	}
	strParameter = std::string(itr->value.GetString(), itr->value.GetStringLength());
	return true;
}

bool JsonRPCRequestHandler::getStringIntParameter(const rapidjson::Value& params, const char* fieldName, std::string& strParameter, int& iParameter)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		stateError(message.data());
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
		stateError(message.data());
		return false;
	}

	return true;
}

bool JsonRPCRequestHandler::checkArrayParameter(const rapidjson::Value& params, const char* fieldName)
{

	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		stateError(message.data());
		return false;
	}

	if (!itr->value.IsArray()) {
		message += " is not a array";
		stateError(message.data());
		return false;
	}

	return true;
}

bool JsonRPCRequestHandler::checkObjectParameter(const rapidjson::Value& params, const char* fieldName)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		stateError(message.data());
		return false;
	}

	if (!itr->value.IsObject()) {
		message += " is not a object";
		stateError(message.data());
		return false;
	}

	return true;
}

bool JsonRPCRequestHandler::checkObjectOrArrayParameter(const rapidjson::Value& params, const char* fieldName)
{
	Value::ConstMemberIterator itr = params.FindMember(fieldName);
	std::string message = fieldName;
	if (itr == params.MemberEnd()) {
		message += " not found";
		stateError(message.data());
		return false;
	}

	if (!itr->value.IsObject() && !itr->value.IsArray()) {
		message += " is not a object or array";
		stateError(message.data());
		return false;
	}

	return true;
}


