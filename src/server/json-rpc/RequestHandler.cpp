#include "RequestHandler.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/http/ServerConfig.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/istreamwrapper.h"

#include "loguru/loguru.hpp"
#include "cpp-httplib/httplib.h"

//#include "Poco/URI.h"
//#include "Poco/DeflatingStream.h"

//#include <exception>
#include <memory>
#include <sstream>

using namespace rapidjson;

namespace server {
	namespace json_rpc {

		static thread_local void* gtl_valueBuffer = malloc(RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY * 4);

		RequestHandler::RequestHandler()
			: mRootJson(kObjectType, new MemoryPoolAllocator<>(gtl_valueBuffer, RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY * 4))
		{
			// MemoryPoolAllocator<> valueAllocator(valueBuffer, valueBuffer->size());
			//MemoryPoolAllocator<> parseAllocator(parseBuffer, parseBuffer->size());
// 			mRootJson = Document()
		}

		Value RequestHandler::handleOneRpcCall(const rapidjson::Value& jsonRpcRequest)
		{
			// LOG_F(2, "handleOneRpcCall");
			Value responseJson(kObjectType);
			std::string method;
			auto& alloc = mRootJson.GetAllocator();

			if (checkObjectOrArrayParameter(responseJson, jsonRpcRequest, "params") && getStringParameter(responseJson, jsonRpcRequest, "method", method)) {
				try {
					handle(responseJson, method, jsonRpcRequest["params"]);
				}
				catch (GradidoBlockchainException& ex) {
					LOG_F(ERROR, "gradido node exception in Json RPC handle: %s", ex.getFullString().data());
					error(responseJson, JSON_RPC_ERROR_GRADIDO_NODE_ERROR, "gradido node intern exception");
				}
				catch (std::exception& ex) {
					LOG_F(ERROR, "exception in Json RPC handle : %s", ex.what());
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

		void RequestHandler::cors(httplib::Response& response)
		{
			if (ServerConfig::g_AllowUnsecureFlags & ServerConfig::UNSECURE_CORS_ALL) {
				response.set_header("Access-Control-Allow-Origin", "*");
				response.set_header("Access-Control-Allow-Headers", "Authorization, Access-Control-Allow-Headers, Origin,Accept, X-Requested-With, Content-Type, Access-Control-Request-Method, Access-Control-Request-Headers");
			}
		}

		void RequestHandler::handlePostPut(const httplib::Request& request, httplib::Response& response)
		{
			loguru::set_thread_name("json-rpc");
			cors(response);
			
			auto& alloc = mRootJson.GetAllocator();
			Value responseJson(kObjectType);
			responseJson.AddMember("jsonrpc", "2.0", alloc);
			responseJson.AddMember("id", Value(kNullType), alloc);

			Document rapidjson_params;			
			
			rapidjson_params.Parse(request.body.data());
			if (rapidjson_params.HasParseError())
			{
				Value data(kObjectType);
				data.AddMember("parseError", Value(GetParseError_En(rapidjson_params.GetParseError()), alloc), alloc);
				data.AddMember("errorOffset", rapidjson_params.GetErrorOffset(), alloc);
				error(responseJson, JSON_RPC_ERROR_PARSE_ERROR, "error parsing request to json", &data);
			}
			else
			{
				if (rapidjson_params.IsObject()) {
					responseJson = handleOneRpcCall(rapidjson_params);
				}
				else if (rapidjson_params.IsArray()) {
					responseJson = Value(kArrayType);
					for (auto& v : rapidjson_params.GetArray()) {
						responseJson.PushBack(handleOneRpcCall(v), alloc);
					}
				}
				else {
					error(responseJson, JSON_RPC_ERROR_INVALID_REQUEST, "empty body");
				}
			}		

			// 3. Stringify the DOM
			static thread_local void* writeMemoryBuffer = malloc(RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY);
			MemoryPoolAllocator<> writeBufferAllocator(writeMemoryBuffer, RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY);
			GenericStringBuffer<UTF8<>, MemoryPoolAllocator<>> buffer(&writeBufferAllocator, RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY);
			
			// StringBuffer buffer();
			Writer writer(buffer);
			responseJson.Accept(writer);
			// printf("response: %s\n", buffer.GetString());
			response.set_content(buffer.GetString(), "application/json");
		}

		void RequestHandler::handleOptions(const httplib::Request& request, httplib::Response& response)
		{
			cors(response);
		}

		void RequestHandler::handleGet(const httplib::Request& request, httplib::Response& response)
		{
			throw GradidoNotImplementedException("REST Api not implemented yet");
		}

		bool RequestHandler::parseQueryParametersToRapidjson(const std::multimap<std::string, std::string>& params, Document& rapidParams)
		{
			rapidParams.SetObject();
			for (const auto& param : params) {
				int tempi = 0;
				Value name_field(param.first.data(), rapidParams.GetAllocator());
				if (DataTypeConverter::NUMBER_PARSE_OKAY == DataTypeConverter::strToInt(param.second, tempi)) {
					//rapidParams[it->first.data()] = rapidjson::Value(tempi, rapidParams.GetAllocator());
					rapidParams.AddMember(name_field.Move(), tempi, rapidParams.GetAllocator());
				}
				else {
					rapidParams.AddMember(name_field.Move(), Value(param.second.data(), rapidParams.GetAllocator()), rapidParams.GetAllocator());
				}
			}

			return true;
		}

		void RequestHandler::error(Value& responseJson, JsonRPCErrorCodes code, const char* message, rapidjson::Value* data/* = nullptr*/)
		{
			Value error(kObjectType);
			auto alloc = mRootJson.GetAllocator();
			error.AddMember("code", code, alloc);
			error.AddMember("message", Value(message, alloc), alloc);
			if (data && !data->IsNull()) {
				error.AddMember("data", *data, alloc);
			}
			LOG_F(2, "json rpc error %d: %s", code, message);
			responseJson.AddMember("error", error, alloc);
		}
		void RequestHandler::error(Value& responseJson, JsonRPCErrorCodes code, GradidoBlockchainException& ex)
		{
			auto errorDetails = ex.getDetails(mRootJson.GetAllocator());
			error(responseJson, code, ex.getFullString().data(), &errorDetails);
		}


		bool RequestHandler::getIntParameter(Value& responseJson, const Value& params, const char* fieldName, int& iParameter, bool optional /*= false*/)
		{
			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			if (!itr->value.IsInt()) {
				message = "invalid " + message;
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			iParameter = itr->value.GetInt();
			return true;
		}

		bool RequestHandler::getBoolParameter(Value& responseJson, const Value& params, const char* fieldName, bool& bParameter, bool optional /*= false*/)
		{
			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			if (!itr->value.IsBool()) {
				message = "invalid " + message;
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			bParameter = itr->value.GetBool();
			return true;
		}

		bool RequestHandler::getUIntParameter(Value& responseJson, const Value& params, const char* fieldName, unsigned int& iParameter, bool optional /*= false*/)
		{
			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			if (!itr->value.IsUint()) {
				message = "invalid " + message;
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			iParameter = itr->value.GetUint();
			return true;
		}

		bool RequestHandler::getUInt64Parameter(Value& responseJson, const Value& params, const char* fieldName, uint64_t& iParameter, bool optional /*= false*/)
		{
			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			if (!itr->value.IsUint64()) {
				message = "invalid " + message;
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			iParameter = itr->value.GetUint64();
			return true;
		}
		bool RequestHandler::getStringParameter(Value& responseJson, const Value& params, const char* fieldName, std::string& strParameter, bool optional /*= false*/)
		{
			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			if (!itr->value.IsString()) {
				message = "invalid " + message;
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			strParameter = std::string(itr->value.GetString(), itr->value.GetStringLength());
			return true;
		}

		bool RequestHandler::getBinaryFromHexStringParameter(
			rapidjson::Value& responseJson,
			const rapidjson::Value& params,
			const char* fieldName,
			std::shared_ptr<const memory::Block> binaryParameter,
			bool optional /*= false*/
		) {
			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			if (!itr->value.IsString()) {
				message = "invalid " + message;
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}
			binaryParameter = std::make_shared< memory::Block>(memory::Block::fromHex(itr->value.GetString(), itr->value.GetStringLength()));
			return true;
		}

		bool RequestHandler::getStringIntParameter(Value& responseJson, const Value& params, const char* fieldName, std::string& strParameter, int& iParameter, bool optional /*= false*/)
		{
			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
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
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}

			return true;
		}

		bool RequestHandler::checkArrayParameter(Value& responseJson, const Value& params, const char* fieldName, bool optional /*= false*/)
		{

			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}

			if (!itr->value.IsArray()) {
				message += " is not a array";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}

			return true;
		}

		bool RequestHandler::checkObjectParameter(Value& responseJson, const Value& params, const char* fieldName, bool optional /*= false*/)
		{
			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}

			if (!itr->value.IsObject()) {
				message += " is not a object";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}

			return true;
		}

		bool RequestHandler::checkObjectOrArrayParameter(Value& responseJson, const Value& params, const char* fieldName, bool optional /*= false*/)
		{
			Value::ConstMemberIterator itr = params.FindMember(fieldName);
			std::string message = fieldName;
			if (itr == params.MemberEnd()) {
				message += " not found";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}

			if (!itr->value.IsObject() && !itr->value.IsArray()) {
				message += " is not a object or array";
				if (!optional) error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, message.data());
				return false;
			}

			return true;
		}

	}
}