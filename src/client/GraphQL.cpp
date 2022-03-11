#include "GraphQL.h"
#include "gradido_blockchain/http/JsonRequest.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "Exceptions.h"

#include "../SingletonManager/LoggerManager.h"

#include "rapidjson/prettywriter.h"

using namespace rapidjson;

namespace client
{
	GraphQL::GraphQL(const Poco::URI& uri)
		: Base(uri, Base::NOTIFICATION_FORMAT_JSON)
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
		JsonRequest request(mUri);
		auto it = parameterValuePairs.find("transactionJson");
		if (it == parameterValuePairs.end()) {
			throw MissingParameterException("missing parameter", "transactionJson");
		}
		Value params(kObjectType);
		Value variables(kObjectType);
		auto alloc = request.getJsonAllocator();

		variables.AddMember("transactionJson", Value(it->second.data(), alloc), alloc);

		params.AddMember("operationName", NULL, alloc);
		params.AddMember("variables", variables, alloc);

		std::string grapqhlQuery = "query($transactionJson: String!){newGradidoBlock(transactionJson: $transactionJson){}}";
		params.AddMember("query", Value(grapqhlQuery.data(), alloc), alloc);

		try {
			auto result = request.postRequest(params);
			StringBuffer buffer;
			PrettyWriter<StringBuffer> writer(buffer);
			result.Accept(writer);
			LoggerManager::getInstance()->mErrorLogging.information("response from notify community (graphql) of new block: %s", std::string(buffer.GetString()));
			return true;
		}
		catch (RapidjsonParseErrorException& ex) {
			throw RequestResponseInvalidJsonException("NewGradidoBlock", mUri, ex.getRawText());
		}		
	}
}