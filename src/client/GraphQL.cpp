#include "GraphQL.h"
#include "gradido_blockchain/http/JsonRequest.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "Exceptions.h"

#include "../SingletonManager/LoggerManager.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/pointer.h"

using namespace rapidjson;

namespace client
{
	GraphQL::GraphQL(const Poco::URI& uri)
		: Base(uri, Base::NOTIFICATION_FORMAT_PROTOBUF_BASE64)
	{

	}

	GraphQL::~GraphQL()
	{

	}

	bool GraphQL::postRequest(const Poco::Net::NameValueCollection& parameterValuePairs)
	{
		/*
		* {
		  newGradidoBlock(transactionJson: "1000") {
		  }
		}
		*/
		std::string transactionMemberName;
		std::string graphQLQuery;
		switch(mFormat) {
			case NOTIFICATION_FORMAT_PROTOBUF_BASE64:
				transactionMemberName = "transactionBase64";
				graphQLQuery = 
"mutation NewGradidoBlock($data: ConfirmedTransactionInput!) { \
  newGradidoBlock(data: $data) { \
    error { \
      name \
      message \
      type \
    } \
    recipe \
    succeed \
  } \
}";
				break;
			case NOTIFICATION_FORMAT_JSON:
				transactionMemberName = "transactionJson";
				graphQLQuery = "mutation($transactionJson: String!, $iotaTopic: String!){newGradidoBlock(transactionJson: $transactionJson, iotaTopic: $iotaTopic)}";
				break;
			default: throw new GradidoUnhandledEnum("unhandled Notification format", "NotificationFormat", static_cast<int>(mFormat));
		}
		
		JsonRequest request(mUri);
		auto it = parameterValuePairs.find(transactionMemberName);
		if (it == parameterValuePairs.end()) {
			throw MissingParameterException("missing parameter", transactionMemberName.data());
		}
		Value params(kObjectType);
		Value variables(kObjectType);
		Value data(kObjectType);
		auto alloc = request.getJsonAllocator();
		data.AddMember(Value(transactionMemberName.data(), alloc), Value(it->second.data(), alloc), alloc);		
		data.AddMember("iotaTopic", Value(mGroupAlias.data(), alloc), alloc);				
		variables.AddMember("data", data, alloc);

		params.AddMember("operationName", Value(Type::kNullType), alloc);
		params.AddMember("variables", variables, alloc);

		params.AddMember("query", Value(graphQLQuery.data(), alloc), alloc);

		try {
			auto result = request.postRequest(params);
			// default result 
			/*
			*
			{
				"data": {
					"newGradidoBlock": true
				}
			}
			*/
			// Access DOM by Get(). It return nullptr if the value does not exist.
			if (Value* newGradidoBlock = Pointer("/data/newGradidoBlock/succeed").Get(result))
				if(newGradidoBlock->IsBool() && newGradidoBlock->GetBool()) {
					return true;
			}
			StringBuffer buffer;
			PrettyWriter<StringBuffer> writer(buffer);
			result.Accept(writer);
			LoggerManager::getInstance()->mErrorLogging.information("response from notify community (graphql) of new block: %s", std::string(buffer.GetString()));
			return false;
		}
		catch (RapidjsonParseErrorException& ex) {
			throw RequestResponseInvalidJsonException("NewGradidoBlock", mUri, ex.getRawText());
		}		
		return false;
	}
}