#include "Base.h"

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "../SingletonManager/LoggerManager.h"

using namespace rapidjson;

namespace client {

	Base::Base(const Poco::URI& uri)
		: mUri(uri), mFormat(NOTIFICATION_FORMAT_PROTOBUF_BASE64)
	{

	}

	Base::Base(const Poco::URI& uri, NotificationFormat format)
		: mUri(uri), mFormat(format)
	{

	}

	Base::~Base()
	{

	}

	bool Base::notificateNewTransaction(Poco::SharedPtr<model::gradido::GradidoBlock> gradidoBlock)
	{
		Poco::Net::NameValueCollection params;
		
		if (mFormat == NOTIFICATION_FORMAT_PROTOBUF_BASE64) {
			auto transactionBase64 = DataTypeConverter::binToBase64(gradidoBlock->getSerialized());
			params.add("transactionBase64", *transactionBase64.get());
		}
		else if (mFormat == NOTIFICATION_FORMAT_JSON) {
			auto transactionJson = gradidoBlock->toJson();
			std::replace(transactionJson.begin(), transactionJson.end(), '"', '\''); // replace all 'x' to 'y'
			params.add("transactionJson", transactionJson);
		}
		else {
			throw GradidoUnknownEnumException("unknown format", "NotificationFormat", mFormat);
		}
		try {
			return postRequest(params);
		}
		catch (RapidjsonParseErrorException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("[Base::notificateNewTransaction] Result Json Exception: %s\n", ex.getFullString());
		}
		catch (RequestResponseInvalidJsonException& ex) {
			if (ex.containRawHtmlClosingTag()) {
				ex.printToFile("notificate", ".html");
			}
			else {
				ex.printToFile("notificate");
			}
			LoggerManager::getInstance()->mErrorLogging.error("[Base::notificateNewTransaction] Invalid Json excpetion, written to file: %s\n", ex.getFullString());
		}
		catch (GradidoBlockchainException& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("[Base::notificateNewTransaction] Gradido Blockchain Exception: %s\n", ex.getFullString());
		}
		catch (Poco::Exception& ex) {
			LoggerManager::getInstance()->mErrorLogging.error("[Base::notificateNewTransaction] Poco Exception: %s\n", ex.displayText());
		}
	}
}