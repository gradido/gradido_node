#include "Base.h"

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/interaction/serialize/Context.h"
#include "gradido_blockchain/interaction/toJson/Context.h"
#include "../SingletonManager/LoggerManager.h"

#include "magic_enum/magic_enum.hpp"

using namespace gradido::data;
using namespace gradido::interaction;
using namespace magic_enum::bitwise_operators;
using namespace rapidjson;

namespace client {

	Base::Base(const Poco::URI& uri)
		: mUri(uri), mFormat(NotificationFormat::PROTOBUF_BASE64)
	{

	}

	Base::Base(const Poco::URI& uri, NotificationFormat format)
		: mUri(uri), mFormat(format)
	{

	}

	Base::~Base()
	{

	}

	bool Base::notificateNewTransaction(const ConfirmedTransaction& confirmedTransaction)
	{
		Poco::Net::NameValueCollection params;
		
		if ((mFormat & NotificationFormat::PROTOBUF_BASE64) == NotificationFormat::PROTOBUF_BASE64) {
			serialize::Context serializer(confirmedTransaction);
			params.add("transactionBase64", serializer.run()->convertToBase64());
		}
		if ((mFormat & NotificationFormat::JSON) == NotificationFormat::JSON) {
			auto transactionJson = toJson::Context(confirmedTransaction).run();
			std::replace(transactionJson.begin(), transactionJson.end(), '"', '\'');
			params.add("transactionJson", transactionJson);
		}
		return notificate(std::move(params));		
	}

	bool Base::notificateFailedTransaction(const gradido::data::GradidoTransaction& gradidoTransaction, const std::string& errorMessage, const std::string& messageId)
	{
		Poco::Net::NameValueCollection params;

		if ((mFormat & NotificationFormat::PROTOBUF_BASE64) == NotificationFormat::PROTOBUF_BASE64) {
			serialize::Context serializer(gradidoTransaction);
			params.add("transactionBase64", serializer.run()->convertToBase64());
		}
		if ((mFormat & NotificationFormat::JSON) == NotificationFormat::JSON) {
			auto transactionJson = toJson::Context(gradidoTransaction).run();
			std::replace(transactionJson.begin(), transactionJson.end(), '"', '\''); // replace all 'x' to 'y'
			params.add("transactionJson", transactionJson);
		}
		
		params.add("error", errorMessage);
		params.add("messageId", messageId);
		return notificate(std::move(params));
	}

	bool Base::notificate(Poco::Net::NameValueCollection params)
	{
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
		return false;
	}

}