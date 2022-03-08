#include "JsonRPCHandler.h"

#include "../lib/BinTextConverter.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "../SingletonManager/GroupManager.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"
#include "Poco/AutoPtr.h"

#include "../model/NodeTransactionEntry.h"

using namespace rapidjson;

void JsonRPCHandler::handle(std::string method, const Value& params)
{
	auto alloc = mResponseJson.GetAllocator();

	if (method == "getlasttransaction") {
		stateSuccess();
		mResponseResult.AddMember("transaction", "", alloc);
	}
	// TODO: rename to listsinceblock
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
	else if (method == "getaddressbalance") {
		stateError("not implemented yet");
	}
	else if (method == "getaddresstxids") {
		stateError("not implemented yet");
	}
	else if (method == "getblock") {
		stateError("not implemented yet");
	}
	else if (method == "getblockcount") {
		stateError("not implemented yet");
	}
	else if (method == "getblockhash") {
		stateError("not implemented yet");
	}
	else if (method == "getreceivedbyaddress") {
		stateError("not implemented yet");
	}
	else if (method == "gettransaction") {
		stateError("not implemented yet");
	}
	else if (method == "listtransactions") {
		stateError("not implemented yet");
	}
	else if (method == "getgroupdetails") {
		std::string groupAlias;
		if (!getStringParameter(params, "group", groupAlias)) return;
		return getGroupDetails(groupAlias);
	}
	else {
		stateError("method not known");
	}
}


void JsonRPCHandler::getGroupDetails(const std::string& groupAlias)
{
	auto gm = GroupManager::getInstance();
	auto group = gm->findGroup(GROUP_REGISTER_GROUP_ALIAS);
	if(group.isNull()) {
		stateError("node server error");
		return;
	}
	auto gradidoBlock = group->getLastTransaction([=](const model::gradido::GradidoBlock* _gradidoBlock) {
		return _gradidoBlock->getGradidoTransaction()->getTransactionBody()->getGlobalGroupAdd()->getGroupAlias() == groupAlias;
	});
	if (gradidoBlock.isNull()) {
		stateError("group not known");
		return;
	}
	auto groupAdd = gradidoBlock->getGradidoTransaction()->getTransactionBody()->getGlobalGroupAdd();
	auto alloc = mResponseJson.GetAllocator();
	stateSuccess();
	mResponseJson.AddMember("groupName", Value(groupAdd->getGroupName().data(), alloc), alloc);
	mResponseJson.AddMember("groupAlias", Value(groupAdd->getGroupAlias().data(), alloc), alloc);
	mResponseJson.AddMember("coinColor", groupAdd->getCoinColor(), alloc);
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
	auto transactions = group->findTransactionsFromXToLast(fromTransactionId);
	printf("%d transactions for group: %s found\n", transactions.size(), groupAlias.data());
	stateSuccess();
	mResponseResult.AddMember("type", "base64", alloc);
	Value jsonTransactionArray(kArrayType);
	Poco::AutoPtr<model::gradido::GradidoBlock> prevTransaction;
	for (auto it = transactions.begin(); it != transactions.end(); it++) {
		auto transactionSerialized = (*it)->getSerializedTransaction();
		if (transactionSerialized->size() > 0) {
			// check for tx hash error			
			/*Profiler time;
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
			auto base64Transaction = Value(DataTypeConverter::binToBase64(*transactionSerialized).data(), alloc);
			jsonTransactionArray.PushBack(base64Transaction, alloc);
			//printf("base 64: %s\n", base64Transaction.GetString());
		}
	}
	mResponseResult.AddMember("transactions", jsonTransactionArray, alloc);
	mResponseResult.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
}
