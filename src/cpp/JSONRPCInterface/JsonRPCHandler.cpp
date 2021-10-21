#include "JsonRPCHandler.h"

#include "../lib/BinTextConverter.h"
#include "../lib/Profiler.h"
#include "../SingletonManager/GroupManager.h"
#include "../model/transactions/GradidoBlock.h"
#include "Poco/AutoPtr.h"

using namespace rapidjson;

Document JsonRPCHandler::handle(std::string method, const rapidjson::Value& params)
{
	Document result(kObjectType);
	auto alloc = result.GetAllocator();

	if (method == "puttransaction") {
		if (!params.IsObject()) {
			return stateError("params not an object");
		}
		std::string groupString;
		std::string transactionString;
		try {
			 groupString = params["group"].GetString();
			 transactionString = params["transaction"].GetString();
		}
		catch (std::exception& e) {
			return stateError("parameter error");
		}

		auto group = convertBinTransportStringToBin(groupString);
		printf("transaction: %s\n", groupString.data());
		auto transaction = convertBinTransportStringToBin(transactionString);
		if ("" == group || "" == transaction) {
			return stateError("wrong parameter format");
		}
		else {
			result = putTransaction(transaction, group);
		}
	}
	else if (method == "getlasttransaction") {
		result.AddMember("state", "success", alloc);
		result.AddMember("transaction", "", alloc);
		return result;
	}
	else {
		return stateError("method not known");
	}
	return result;

}


Document JsonRPCHandler::putTransaction(const std::string& transactionBinary, const std::string& groupAlias)
{
	Profiler timeUsed;
	Document result(kObjectType);
	auto alloc = result.GetAllocator();

	auto gm = GroupManager::getInstance();
	auto group = gm->findGroup(groupAlias);
	Poco::AutoPtr<model::GradidoBlock> transaction(new model::GradidoBlock(transactionBinary, group));
	transaction->addGroupAlias(groupAlias);

	if (transaction->errorCount() > 0) {
		result.AddMember("state", "error", alloc);
		result.AddMember("errors", transaction->getErrorsArray(alloc), alloc);
		return result;
	}

	if (!group->addTransaction(transaction)) {
		result.AddMember("state", "error", alloc);
		result.AddMember("msg", "error adding transaction", alloc);
		result.AddMember("details", transaction->getErrorsArray(alloc), alloc);
		return result;
	}

	result.AddMember("state", "success", alloc);
	result.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
	return result;
}
