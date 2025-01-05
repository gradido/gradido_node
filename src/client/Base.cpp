#include "Base.h"

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/interaction/serialize/Context.h"
#include "gradido_blockchain/interaction/toJson/Context.h"

#include "magic_enum/magic_enum.hpp"
#include "loguru/loguru.hpp"

using namespace gradido::data;
using namespace gradido::interaction;
using namespace magic_enum::bitwise_operators;
using namespace rapidjson;

namespace client {

	Base::Base(const std::string& uri)
		: mUri(uri), mFormat(NotificationFormat::PROTOBUF_BASE64)
	{

	}

	Base::Base(const std::string& uri, NotificationFormat format)
		: mUri(uri), mFormat(format)
	{

	}

	Base::~Base()
	{

	}

	bool Base::notificateNewTransaction(const ConfirmedTransaction& confirmedTransaction)
	{
		std::map<std::string, std::string> params;
		
		if ((mFormat & NotificationFormat::PROTOBUF_BASE64) == NotificationFormat::PROTOBUF_BASE64) {
			serialize::Context serializer(confirmedTransaction);
			params.insert({ "transactionBase64", serializer.run()->convertToBase64() });
		}
		if ((mFormat & NotificationFormat::JSON) == NotificationFormat::JSON) {
			auto transactionJson = toJson::Context(confirmedTransaction).run();
			std::replace(transactionJson.begin(), transactionJson.end(), '"', '\'');
			params.insert({ "transactionJson", transactionJson });
		}
		return notificate(params);		
	}

	bool Base::notificateFailedTransaction(const gradido::data::GradidoTransaction& gradidoTransaction, const std::string& errorMessage, const std::string& messageId)
	{
		std::map<std::string, std::string> params;

		if ((mFormat & NotificationFormat::PROTOBUF_BASE64) == NotificationFormat::PROTOBUF_BASE64) {
			serialize::Context serializer(gradidoTransaction);
			params.insert({ "transactionBase64", serializer.run()->convertToBase64() });
		}
		if ((mFormat & NotificationFormat::JSON) == NotificationFormat::JSON) {
			auto transactionJson = toJson::Context(gradidoTransaction).run();
			std::replace(transactionJson.begin(), transactionJson.end(), '"', '\''); // replace all 'x' to 'y'
			params.insert({ "transactionJson", transactionJson });
		}
		
		params.insert({ "error", errorMessage });
		params.insert({ "messageId", messageId });
		return notificate(std::move(params));
	}

	bool Base::notificate(const std::map<std::string, std::string>& params)
	{
		try {
			return postRequest(params);
		}
		catch (RapidjsonParseErrorException& ex) {
			LOG_F(ERROR, "Result Json Exception: %s", ex.getFullString().data());
		}
		catch (RequestResponseInvalidJsonException& ex) {
			if (ex.containRawHtmlClosingTag()) {
				ex.printToFile("notificate", ".html");
			}
			else {
				ex.printToFile("notificate");
			}
			LOG_F(ERROR, "Invalid Json excpetion, written to file : %s", ex.getFullString().data());
		}
		catch (GradidoBlockchainException& ex) {
			LOG_F(ERROR, "Gradido Blockchain Exception: %s", ex.getFullString().data());
		}
		return false;
	}

}