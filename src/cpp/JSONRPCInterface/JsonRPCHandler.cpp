#include "JsonRPCHandler.h"

#include "../lib/BinTextConverter.h"
#include "../lib/DataTypeConverter.h"
#include "../lib/Profiler.h"
#include "../SingletonManager/GroupManager.h"
#include "../model/transactions/GradidoBlock.h"
#include "Poco/AutoPtr.h"

using namespace rapidjson;

void JsonRPCHandler::handle(std::string method, const rapidjson::Value& params)
{
	auto alloc = mResponseJson.GetAllocator();

	if (method == "puttransaction") {
		if (!params.IsObject()) {
			stateError("params not an object");
			return;
		}
		std::string groupString;
		std::string transactionString;
		try {
			 groupString = params["group"].GetString();
			 transactionString = params["transaction"].GetString();
		}
		catch (std::exception& e) {
			stateError("parameter error");
			return;
		}

		auto group = convertBinTransportStringToBin(groupString);
		printf("transaction: %s\n", groupString.data());
		auto transaction = convertBinTransportStringToBin(transactionString);
		if ("" == group || "" == transaction) {
			stateError("wrong parameter format");
			return;
		}
		else {
			putTransaction(transaction, group);
		}
	}
	else if (method == "getlasttransaction") {
		stateSuccess();
		mResponseResult.AddMember("transaction", "", alloc);
	}
	else if (method == "getTransactions") {
		printf("getTransactions called\n");
		if(!params.IsObject()) {
			return stateError("params not an object");
		}
		std::string groupAlias;
		uint64_t transactionId = 0;

		if(!getStringParameter(params, "group", groupAlias)) return;
		if(!getUInt64Parameter(params, "fromTransactionId", transactionId)) return;
		printf("group: %s, id: %d\n", groupAlias.data(), transactionId);
		getTransactions(transactionId, groupAlias);
	}
	else {
		stateError("method not known");
	}
}


void JsonRPCHandler::putTransaction(const std::string& transactionBinary, const std::string& groupAlias)
{
	Profiler timeUsed;
	auto alloc = mResponseJson.GetAllocator();

	auto gm = GroupManager::getInstance();
	auto group = gm->findGroup(groupAlias);
	Poco::AutoPtr<model::GradidoBlock> transaction(new model::GradidoBlock(transactionBinary, group));
	transaction->addGroupAlias(groupAlias);

	if (transaction->errorCount() > 0) {
		stateError("error by parsing transaction", transaction);
		return;
	}

	if (!group->addTransaction(transaction)) {
		stateError("error adding transaction", transaction);
		return;
	}

	stateSuccess();
	mResponseResult.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
}

void JsonRPCHandler::getTransactions(int64_t fromTransactionId, const std::string& groupAlias)
{
	Profiler timeUsed;
	auto alloc = mResponseJson.GetAllocator();

	auto gm = GroupManager::getInstance();
	auto group = gm->findGroup(groupAlias);
	if (group.isNull()) {
		stateError("group not known");
		return;
	}
	//printf("group found and loaded\n");
	auto transactions = group->findTransactionsSerialized(fromTransactionId);
	printf("%d transactions for group: %s found\n", transactions.size(), groupAlias.data());
	stateSuccess();
	mResponseResult.AddMember("type", "base64", alloc);
	Value jsonTransactionArray(kArrayType);
	Poco::AutoPtr<model::GradidoBlock> prevTransaction;
	for (auto it = transactions.begin(); it != transactions.end(); it++) {
		if (it->size() > 0) {
			// check for tx hash error			
			Profiler time;
			Poco::AutoPtr<model::GradidoBlock> gradidoBlock(new model::GradidoBlock(*it, group));
			//printf("time unserialize: %s\n", time.string().data());
			model::TransactionValidationLevel level = static_cast<model::TransactionValidationLevel>(model::TRANSACTION_VALIDATION_DATE_RANGE | model::TRANSACTION_VALIDATION_SINGLE | model::TRANSACTION_VALIDATION_SINGLE_PREVIOUS);
			auto validationResult = gradidoBlock->validate(level);
			
			if (!prevTransaction.isNull()) {
				time.reset();
				if (!gradidoBlock->validate(prevTransaction)) {
					printf("error validating transaction with prev transaction\n");
					gradidoBlock->printErrors();
				}
				//printf("time validate tx hash: %s\n", time.string().data());
			}
			prevTransaction = gradidoBlock;
			if (!validationResult) {
				printf("error validating transaction\n");
				gradidoBlock->printErrors();
				//break;
			}
			//*/
			jsonTransactionArray.PushBack(Value(DataTypeConverter::binToBase64(*it).data(), alloc), alloc);
			auto base64 = DataTypeConverter::binToBase64(*it);
			//printf("base 64: %s\n", base64.data());
		}
	}
	mResponseResult.AddMember("transactions", jsonTransactionArray, alloc);
	mResponseResult.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
}
