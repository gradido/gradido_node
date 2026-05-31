#include "Base.h"

#include "gradido_blockchain/data/hiero/TransactionId.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/interaction/serialize/Context.h"
#include "gradido_blockchain/serialization/toJsonString.h"
#include "magic_enum/magic_enum.hpp"
#include "loguru/loguru.hpp"

using namespace gradido::data;
using namespace gradido::interaction;
using namespace serialization;
using namespace magic_enum::bitwise_operators;
using namespace rapidjson;

namespace client {

	Base::Base(const std::string& successUrl, const std::string& failedUrl)
		: mSuccessUrl(successUrl), mFailedUrl(failedUrl), mFormat(NotificationFormat::PROTOBUF_BASE64)
	{

	}

	Base::Base(const std::string& successUrl, const std::string& failedUrl, NotificationFormat format)
		: mSuccessUrl(successUrl), mFailedUrl(failedUrl), mFormat(format)
	{

	}

	Base::~Base()
	{

	}

	bool Base::notificateNewTransaction(const ConfirmedTransaction& confirmedTransaction)
	{
		if (mSuccessUrl.empty()) {
			return true;
		}
		std::map<std::string, std::string> params;
		
		if ((mFormat & NotificationFormat::PROTOBUF_BASE64) == NotificationFormat::PROTOBUF_BASE64) {
			serialize::Context serializer(confirmedTransaction);
			params.insert({ "transactionBase64", serializer.run()->convertToBase64() });
		}
		if ((mFormat & NotificationFormat::JSON) == NotificationFormat::JSON) {
			auto transactionJson = toJsonString(confirmedTransaction);
			std::replace(transactionJson.begin(), transactionJson.end(), '"', '\'');
			params.insert({ "transactionJson", transactionJson });
		}
		return notificate(params, mSuccessUrl);		
	}

	bool Base::notificateFailedTransaction(
		const gradido::data::GradidoTransaction& gradidoTransaction, 
		const std::string& errorMessage, 
		const hiero::TransactionId& hieroTransactionId
	) {
		if (mFailedUrl.empty()) {
			return true;
		}
		std::map<std::string, std::string> params;

		if ((mFormat & NotificationFormat::PROTOBUF_BASE64) == NotificationFormat::PROTOBUF_BASE64) {
			serialize::Context serializer(gradidoTransaction);
			params.insert({ "transactionBase64", serializer.run()->convertToBase64() });
		}
		if ((mFormat & NotificationFormat::JSON) == NotificationFormat::JSON) {
			auto transactionJson = toJsonString(gradidoTransaction);
			std::replace(transactionJson.begin(), transactionJson.end(), '"', '\''); // replace all 'x' to 'y'
			params.insert({ "transactionJson", transactionJson });
		}
		
		params.insert({ "error", errorMessage });
		params.insert({ "hieroTransactionId", hieroTransactionId.toString()});
		return notificate(std::move(params), mFailedUrl);
	}

	bool Base::notificate(const std::map<std::string, std::string>& params, const std::string& url)
	{
		try {
			return postRequest(params, url);
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