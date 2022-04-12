#include "Json.h"
#include "gradido_blockchain/http/JsonRequest.h"
#include "../SingletonManager/LoggerManager.h"
#include "rapidjson/prettywriter.h"

using namespace rapidjson;

namespace client {
	Json::Json(const Poco::URI& uri, bool base64 /*= true*/)
		: Base(uri, base64 ? Base::NOTIFICATION_FORMAT_PROTOBUF_BASE64 : Base::NOTIFICATION_FORMAT_JSON)
	{

	}

	bool Json::postRequest(const Poco::Net::NameValueCollection& parameterValuePairs)
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
			LoggerManager::getInstance()->mErrorLogging.error("[client::Json::postRequest] Result Json Exception: %s\n", ex.getFullString().data());
		}
		catch (Poco::Exception& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("[client::Json::postRequest] Poco Exception: %s\n", ex.displayText().data());
		}
		return true;
	}
}