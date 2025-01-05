#include "JsonRPC.h"

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/http/JsonRequest.h"

#include "rapidjson/prettywriter.h"
#include "loguru/loguru.hpp"

using namespace rapidjson;

namespace client {
	JsonRPC::JsonRPC(const std::string& uri, bool base64 /*= true*/)
		: Base(uri, base64 ? Base::NotificationFormat::PROTOBUF_BASE64 : Base::NotificationFormat::JSON)
	{

	}

	bool JsonRPC::postRequest(const std::map<std::string, std::string>& parameterValuePairs)
	{
		JsonRequest request(mUri);

		Value params(kObjectType);
		auto alloc = request.getJsonAllocator();
		for (auto it = parameterValuePairs.begin(); it != parameterValuePairs.end(); it++) {		
			params.AddMember(Value(it->first.data(), alloc), Value(it->second.data(), alloc), alloc);
		}		
		
		try {
			auto result = request.postRequest(params);
		/*	if (result.IsObject()) {
				StringBuffer buffer;
				PrettyWriter<StringBuffer> writer(buffer);
				result.Accept(writer);
				LoggerManager::getInstance()->mErrorLogging.information("response from notify community of new block: %s", std::string(buffer.GetString()));
			}
			*/
		}
		catch (RapidjsonParseErrorException& ex) {
			LOG_F(ERROR, "Result Json Exception: %s", ex.getFullString().data());
		}
		catch (GradidoBlockchainException& ex) {
			LOG_F(ERROR, "uncatched GradidoBlockchainException: %s", ex.getFullString().data());
		} 
		catch (std::exception& ex) {
			LOG_F(ERROR, "uncatched exception: %s", ex.what());
		}
		return true;
	}
}