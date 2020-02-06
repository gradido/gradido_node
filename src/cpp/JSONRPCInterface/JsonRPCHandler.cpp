#include "JsonRPCHandler.h"

#include "../lib/BinTextConverter.h"
#include "../lib/Profiler.h"
#include "../SingletonManager/GroupManager.h"
#include "../model/transactions/Transaction.h"

void JsonRPCHandler::handle(const jsonrpcpp::Request& request, Json& response)
{
	if (request.method == "puttransaction") {
		if (request.params.has("group") && request.params.has("transaction")) {
			auto group = convertBinTransportStringToBin(request.params.get("group"));
			auto transaction = convertBinTransportStringToBin(request.params.get("transaction"));
			if ("" == group || "" == transaction) {
				response = { {"state", "error"}, {"msg", "wrong parameter format"} };
			}
			else {
				putTransaction(transaction, group, response);
			}
		}
		else {
			response = { { "state", "error" },{ "msg", "missing parameter" } };
		}
	}
	else if (request.method == "getlasttransaction") {
		response = { {"state" , "success"}, {"transaction", ""} };
	}
	else {
		response = { { "state", "error" },{ "msg", "method not known" } };
	}
}

void JsonRPCHandler::putTransaction(const std::string& transactionBinary, const std::string& groupPublicBinary, Json& response)
{
	Profiler timeUsed;
	auto gm = GroupManager::getInstance();
	auto groupBase58 = convertBinToBase58(groupPublicBinary);
	auto group = gm->findGroup(groupBase58);
	auto transaction = new model::Transaction(transactionBinary);
	if (transaction->errorCount() > 0) {
		Json errorJson;
		transaction->getErrors(errorJson);
		response["errors"] = errorJson;
		response["state"] = "error";
		return;
	}
	
	if (transaction->getID() > 1) {

	}
	else {
		// it is the genesis transaction :)
	}

	response = { { "state", "error" },{ "groupBase58", groupBase58 } };
	
}