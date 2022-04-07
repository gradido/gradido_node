#include "JsonRPCHandler.h"

#include "../lib/BinTextConverter.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "../SingletonManager/GroupManager.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"

#include "Poco/AutoPtr.h"
#include "Poco/DateTimeFormatter.h"

#include "../model/NodeTransactionEntry.h"
#include "../model/Apollo/TransactionList.h"

#include "rapidjson/prettywriter.h"

using namespace rapidjson;

void JsonRPCHandler::handle(std::string method, const Value& params)
{
	auto alloc = mResponseJson.GetAllocator();

	// Debugging
	StringBuffer buffer;
	PrettyWriter<StringBuffer> writer(buffer);
	params.Accept(writer);
	printf("incoming json-rpc request, params: \n%s\n", buffer.GetString());

	std::set<std::string> noNeedForGroupAlias = {
		"getRandomUniqueCoinColor",
		"isGroupUnique"
	};

	std::string groupAlias;
	if (noNeedForGroupAlias.find(method) == noNeedForGroupAlias.end() && !getStringParameter(params, "groupAlias", groupAlias)) {
		mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
		return;
	}

	if (method == "getlasttransaction") {
		stateSuccess();
		mResponseResult.AddMember("transaction", "", alloc);
	}
	// TODO: rename to listsinceblock
	else if (method == "getTransactions") {
		printf("getTransactions called\n");
		if(!params.IsObject()) {
			mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
			stateError("params not an object");
			return;
		}
		std::string format;
		uint64_t transactionId = 0;

		if (!getUInt64Parameter(params, "fromTransactionId", transactionId)) {
			mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
			return;
		}		
		getStringParameter(params, "format", format);
		printf("group: %s, id: %d\n", groupAlias.data(), transactionId);
		getTransactions(transactionId, groupAlias, format);
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
		std::string pubkey;
		if (!getStringParameter(params, "pubkey", pubkey)) {
			mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
			return;
		}
		int currentPage = 1, pageSize = 25;
		getIntParameter(params, "currentPage", currentPage);
		getIntParameter(params, "pageSize", pageSize);
		bool orderDESC = true, onlyCreations = false;
		getBoolParameter(params, "orderDESC", orderDESC);
		getBoolParameter(params, "onlyCreations", onlyCreations);

		return listTransactions(groupAlias, pubkey, currentPage, pageSize, orderDESC, onlyCreations);
	}
	else if (method == "getgroupdetails") {
		return getGroupDetails(groupAlias);
	}
	else if (method == "isGroupUnique") {
		std::string groupAlias; 
		uint32_t coinColor;
		getStringParameter(params, "groupAlias", groupAlias);
		getUIntParameter(params, "coinColor", coinColor);
		isGroupUnique(groupAlias, coinColor);
	}
	else if (method == "getRandomUniqueCoinColor") {
		getRandomUniqueCoinColor();
	}
	else {
		mResponseErrorCode = JSON_RPC_ERROR_METHODE_NOT_FOUND;
		stateError("method not known");
	}
}


void JsonRPCHandler::getGroupDetails(const std::string& groupAlias)
{
	auto gm = GroupManager::getInstance();
	auto group = gm->findGroup(GROUP_REGISTER_GROUP_ALIAS);
	if(group.isNull()) {
		mResponseErrorCode = JSON_RPC_ERROR_GRADIDO_NODE_ERROR;
		stateError("node server error");
		return;
	}
	auto gradidoBlock = group->getLastTransaction([=](const model::gradido::GradidoBlock* _gradidoBlock) {
		return _gradidoBlock->getGradidoTransaction()->getTransactionBody()->getGlobalGroupAdd()->getGroupAlias() == groupAlias;
	});
	if (gradidoBlock.isNull()) {
		mResponseErrorCode = JSON_RPC_ERROR_UNKNOWN_GROUP;
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

void JsonRPCHandler::getRandomUniqueCoinColor()
{
	auto gm = GroupManager::getInstance();
	auto groupRegisterGroup = gm->getGroupRegisterGroup();
	auto alloc = mResponseJson.GetAllocator();

	stateSuccess();
	mResponseJson.AddMember("coinColor", groupRegisterGroup->generateUniqueCoinColor(), alloc);
}

void JsonRPCHandler::isGroupUnique(const std::string& groupAlias, uint32_t coinColor)
{
	auto gm = GroupManager::getInstance();
	auto groupRegisterGroup = gm->getGroupRegisterGroup();
	auto alloc = mResponseJson.GetAllocator();

	if (!groupAlias.size() && !coinColor) {
		mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
		stateError("nothing to check");
	}
	stateSuccess();
	if (groupAlias.size()) {
		mResponseJson.AddMember("isGroupAliasUnique", groupRegisterGroup->isGroupAliasUnique(groupAlias), alloc);
	}
	if (coinColor) {
		mResponseJson.AddMember("isCoinColorUnique", groupRegisterGroup->isCoinColorUnique(coinColor), alloc);
	}
}

void JsonRPCHandler::getTransactions(int64_t fromTransactionId, const std::string& groupAlias, const std::string& format)
{
	Profiler timeUsed;
	auto alloc = mResponseJson.GetAllocator();

	auto gm = GroupManager::getInstance();
	auto group = gm->findGroup(groupAlias);
	if (group.isNull()) {
		mResponseErrorCode = JSON_RPC_ERROR_UNKNOWN_GROUP;
		stateError("group not known");
		return;
	}
	//printf("group found and loaded\n");
	auto transactions = group->findTransactionsFromXToLast(fromTransactionId);
	printf("%d transactions for group: %s found\n", transactions.size(), groupAlias.data());
	stateSuccess();
	if (format == "json") {
		mResponseResult.AddMember("type", "json", alloc);
	}
	else {
		mResponseResult.AddMember("type", "base64", alloc);
	}
	Value jsonTransactionArray(kArrayType);
	Poco::AutoPtr<model::gradido::GradidoBlock> prevTransaction;
	for (auto it = transactions.begin(); it != transactions.end(); it++) {
		
		auto transactionSerialized = (*it)->getSerializedTransaction();
		if (transactionSerialized->size() > 0) {
			if (format == "json") {
				auto gradidoBlock = std::make_unique<model::gradido::GradidoBlock>(transactionSerialized);
				//auto jsonTransaction = Value(gradidoBlock->toJson().data(), alloc);
				auto jsonTransaction = gradidoBlock->toJson(mResponseJson);
				jsonTransactionArray.PushBack(jsonTransaction, alloc);
			} else {
				auto base64Transaction = Value(DataTypeConverter::binToBase64(*transactionSerialized).data(), alloc);
				jsonTransactionArray.PushBack(base64Transaction, alloc);
			}
		}
	}
	mResponseResult.AddMember("transactions", jsonTransactionArray, alloc);
	mResponseResult.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
}

void JsonRPCHandler::listTransactions(
	const std::string& groupAlias,
	const std::string& publicKeyHex,
	int currentPage /*= 1*/,
	int pageSize /*= 25*/,
	bool orderDESC /*= true*/,
	bool onlyCreations /*= false*/
)
{
	/*
	graphql format for request used from frontend:
	{
		"operationName": null,
		"variables": {
			"currentPage": 1,
			"pageSize": 5,
			"order": "DESC",
			"onlyCreations": false
		},
		"query": "query ($currentPage: Int = 1, $pageSize: Int = 25, $order: Order = DESC, $onlyCreations: Boolean = false) {\n  transactionList(\n    currentPage: $currentPage\n    pageSize: $pageSize\n    order: $order\n    onlyCreations: $onlyCreations\n  ) {\n    gdtSum\n    count\n    balance\n    decay\n    decayDate\n    transactions {\n      type\n      balance\n      decayStart\n      decayEnd\n      decayDuration\n      memo\n      transactionId\n      name\n      email\n      date\n      decay {\n        balance\n        decayStart\n        decayEnd\n        decayDuration\n        decayStartBlock\n        __typename\n      }\n      firstTransaction\n      __typename\n    }\n    __typename\n  }\n}\n"
	}
	*/
	Profiler timeUsed;
	auto alloc = mResponseJson.GetAllocator();
	auto gm = GroupManager::getInstance();
	auto group = gm->findGroup(groupAlias);
	if (group.isNull()) {
		mResponseErrorCode = JSON_RPC_ERROR_UNKNOWN_GROUP;
		stateError("group not known");
		return;
	}

	auto pubkey = DataTypeConverter::hexToBinString(publicKeyHex);
	auto allTransactions = group->findTransactions(*pubkey.get());

	model::Apollo::TransactionList transactionList(group, std::move(pubkey), alloc);
	auto transactionListValue = transactionList.generateList(allTransactions, currentPage, pageSize, orderDESC, onlyCreations);

	stateSuccess();
	mResponseResult.AddMember("transactionList", transactionListValue, alloc);
	mResponseResult.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);
}