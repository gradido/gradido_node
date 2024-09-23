#include "JsonRPC.h"
#include "gradido_blockchain/http/JsonRequest.h"
#include "../SingletonManager/LoggerManager.h"
#include "rapidjson/prettywriter.h"

using namespace rapidjson;

namespace client {
	JsonRPC::JsonRPC(const Poco::URI& uri, bool base64 /*= true*/)
		: Base(uri, base64 ? Base::NotificationFormat::PROTOBUF_BASE64 : Base::NotificationFormat::JSON)
	{

	}

	bool JsonRPC::postRequest(const Poco::Net::NameValueCollection& parameterValuePairs)
	{
		JsonRequest request(mUri.getPathAndQuery());

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
			LoggerManager::getInstance()->mErrorLogging.error("[client::JsonRPC::postRequest] Result Json Exception: %s\n", ex.getFullString().data());
		}
		catch (Poco::Exception& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("[client::JsonRPC::postRequest] Poco Exception: %s\n", ex.displayText().data());
		}
		return true;
	}
}