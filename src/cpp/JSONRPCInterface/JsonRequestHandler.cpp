#include "JsonRequestHandler.h"

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"

#include "../lib/DataTypeConverter.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

//#include "Poco/URI.h"
//#include "Poco/DeflatingStream.h"

//#include <exception>
#include <memory>

using namespace rapidjson;

JsonRequestHandler::JsonRequestHandler()
	: mResponseJson(kObjectType), mResponseResult(kObjectType)
{

}

void JsonRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
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
				catch (std::exception& ex) {
					stateError("exception", ex.what());
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
		mResponseJson.AddMember("result", mResponseResult, alloc);
		mResponseJson.AddMember("jsonrpc", "2.0", alloc);
		mResponseJson.AddMember("id", id, alloc);
		mResponseJson.Accept(writer);

		printf("result: %s\n", buffer.GetString());

		responseStream << buffer.GetString() << std::endl;
	}
	else {
		responseStream << "{\"result\":{\"state\":\"error\", \"msg\":\"no result\"}}";
	}
	
	//if (_compressResponse) _gzipStream.close();
	
}


bool JsonRequestHandler::parseQueryParametersToRapidjson(const Poco::URI& uri, Document& rapidParams)
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

void JsonRequestHandler::parseJsonWithErrorPrintFile(std::istream& request_stream, Document& rapidParams, ErrorList* errorHandler /* = nullptr*/, const char* functionName /* = nullptr*/)
{
	// debugging answer

	std::stringstream responseStringStream;
	for (std::string line; std::getline(request_stream, line); ) {
		responseStringStream << line << std::endl;
	}

	rapidParams.Parse(responseStringStream.str().data());
	if (rapidParams.HasParseError()) {
		auto error_code = rapidParams.GetParseError();
		if (errorHandler) {
			errorHandler->addError(new ParamError(functionName, "error parsing request answer", error_code));
			errorHandler->addError(new ParamError(functionName, "position of last parsing error", rapidParams.GetErrorOffset()));
		}
		std::string dateTimeString = Poco::DateTimeFormatter::format(Poco::DateTime(), "%d_%m_%yT%H_%M_%S");
		std::string filename = dateTimeString + "_response.html";
		FILE* f = fopen(filename.data(), "wt");
		if (f) {
			std::string responseString = responseStringStream.str();
			fwrite(responseString.data(), 1, responseString.size(), f);
			fclose(f);
		}
	}
}

void JsonRequestHandler::stateError(const char* msg, std::string details)
{
	auto alloc = mResponseJson.GetAllocator();
	mResponseResult.AddMember("state", "error", alloc);
	mResponseResult.AddMember("msg", Value(msg, alloc), alloc);

	if (details.size()) {
		mResponseResult.AddMember("details", Value(details.data(), alloc), alloc);
	}
}


void JsonRequestHandler::stateError(const char* msg, ErrorList* errorReciver)
{
	auto alloc = mResponseJson.GetAllocator();
	mResponseResult.AddMember("state", "error", alloc);
	mResponseResult.AddMember("msg", Value(msg, alloc).Move(), alloc);
	Value details(kArrayType);
	auto error_vec = errorReciver->getErrorsArray();
	for (auto it = error_vec.begin(); it != error_vec.end(); it++) {
		details.PushBack(Value(it->data(), alloc).Move(), alloc);
	}
	mResponseResult.AddMember("details", details.Move(), alloc);
}

void JsonRequestHandler::stateSuccess()
{
	mResponseResult.AddMember("state", "success", mResponseJson.GetAllocator());
}


void JsonRequestHandler::customStateError(const char* state, const char* msg, std::string details /* = "" */)
{
	auto alloc = mResponseJson.GetAllocator();
	mResponseResult.AddMember("state", Value(state, alloc).Move(), alloc);
	mResponseResult.AddMember("msg", Value(msg, alloc).Move(), alloc);

	if (details.size()) {
		mResponseResult.AddMember("details", Value(details.data(), alloc).Move(), alloc);
	}
}


void JsonRequestHandler::stateWarning(const char* msg, std::string details/* = ""*/)
{
	auto alloc = mResponseJson.GetAllocator();
	mResponseResult.AddMember("state", "warning", alloc);
	mResponseResult.AddMember("msg", Value(msg, alloc).Move(), alloc);

	if (details.size()) {
		mResponseResult.AddMember("details", Value(details.data(), alloc).Move(), alloc);
	}
}



bool JsonRequestHandler::getIntParameter(const rapidjson::Value& params, const char* fieldName, int& iParameter)
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

bool JsonRequestHandler::getBoolParameter(const rapidjson::Value& params, const char* fieldName, bool& bParameter)
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

bool JsonRequestHandler::getUIntParameter(const rapidjson::Value& params, const char* fieldName, unsigned int& iParameter)
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

bool JsonRequestHandler::getUInt64Parameter(const rapidjson::Value& params, const char* fieldName, Poco::UInt64& iParameter)
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
bool JsonRequestHandler::getStringParameter(const rapidjson::Value& params, const char* fieldName, std::string& strParameter)
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

bool JsonRequestHandler::getStringIntParameter(const rapidjson::Value& params, const char* fieldName, std::string& strParameter, int& iParameter)
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

bool JsonRequestHandler::checkArrayParameter(const rapidjson::Value& params, const char* fieldName)
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

bool JsonRequestHandler::checkObjectParameter(const rapidjson::Value& params, const char* fieldName)
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

bool JsonRequestHandler::checkObjectOrArrayParameter(const rapidjson::Value& params, const char* fieldName)
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


