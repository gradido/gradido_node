#include "JsonRPCHandler.h"

#include "../lib/BinTextConverter.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "../SingletonManager/GroupManager.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"

#include "Poco/AutoPtr.h"
#include "Poco/DateTimeFormatter.h"

#include "../model/NodeTransactionEntry.h"

#include "rapidjson/prettywriter.h"

using namespace rapidjson;

void JsonRPCHandler::handle(std::string method, const Value& params)
{
	auto alloc = mResponseJson.GetAllocator();

	StringBuffer buffer;
	PrettyWriter<StringBuffer> writer(buffer);
	params.Accept(writer);
	printf("incoming json-rpc request, params: \n%s\n", buffer.GetString());

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
		std::string groupAlias;
		std::string format;
		uint64_t transactionId = 0;

		if (!getStringParameter(params, "group", groupAlias) || !getUInt64Parameter(params, "fromTransactionId", transactionId)) {
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
		stateError("not implemented yet");
	}
	else if (method == "getgroupdetails") {
		std::string groupAlias;
		if (!getStringParameter(params, "groupAlias", groupAlias)) {
			mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
			return;
		}
		return getGroupDetails(groupAlias);
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
	if (onlyCreations) {
		throw std::runtime_error("onlyCreations = true, not implemented yet");
	}
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
	Value transactionList(kObjectType);
	transactionList.AddMember("gdtSum", "0", alloc);
	transactionList.AddMember("count", allTransactions.size(), alloc);

	Value transactions(kArrayType);

	
	// sort in ascending order
	// TODO: check if it is already sorted
	std::sort(allTransactions.begin(), allTransactions.end(), [](
		const Poco::SharedPtr<model::NodeTransactionEntry>& x,
		const Poco::SharedPtr<model::NodeTransactionEntry>& y) {
			return x->getTransactionNr() > y->getTransactionNr();
	});	

	int iteratorPage = 1;
	int pageIterator = 1;
	std::list<Value> transactionsTempList;
	for (auto it = allTransactions.begin(); it != allTransactions.end(); it++) {
		if (pageIterator >= pageSize) {
			iteratorPage++;
			pageIterator = 0;
		}
		if (iteratorPage > currentPage) break;
		if (iteratorPage == currentPage) 
		{
			Value transaction(kObjectType);
			auto gradidoBlock = std::make_unique<model::gradido::GradidoBlock>((*it)->getSerializedTransaction());
			auto gradidoTransaction = gradidoBlock->getGradidoTransaction();
			auto transactionBody = gradidoTransaction->getTransactionBody();
			if (transactionBody->getTransactionType() == model::gradido::TRANSACTION_CREATION) {
				transaction.AddMember("type", "creation", alloc);
				auto creation = transactionBody->getCreationTransaction();
				transaction.AddMember("balance", Value(creation->getAmount().data(), alloc), alloc);
				transaction.AddMember("name", "Gradido Akademie", alloc);
			}
			else if (transactionBody->getTransactionType() == model::gradido::TRANSACTION_TRANSFER ||
				transactionBody->getTransactionType() == model::gradido::TRANSACTION_DEFERRED_TRANSFER) {
				auto transfer = transactionBody->getTransferTransaction();
				if (transfer->getRecipientPublicKeyString() == *pubkey.get()) {
					transaction.AddMember("type", "receive", alloc);
					transaction.AddMember("name", Value(DataTypeConverter::binToHex(transfer->getSenderPublicKeyString()).data(), alloc), alloc);
				}
				else if (transfer->getSenderPublicKeyString() == *pubkey.get()) {
					transaction.AddMember("type", "send", alloc);
					transaction.AddMember("name", Value(DataTypeConverter::binToHex(transfer->getRecipientPublicKeyString()).data(), alloc), alloc);
				}
				transaction.AddMember("balance", Value(transfer->getAmount().data(), alloc), alloc);
			}
			else {
				throw std::runtime_error("transaction type not implemented yet");
			}
			transaction.AddMember("memo", Value(transactionBody->getMemo().data(), alloc), alloc);
			transaction.AddMember("transactionId", gradidoBlock->getID(), alloc);
			auto dateString = Poco::DateTimeFormatter::format(gradidoBlock->getReceivedAsTimestamp(), "%Y-%m-%dT%H:%M:%S.%i%z");
			transaction.AddMember("date", Value(dateString.data(), alloc), alloc);
			Value decay(kObjectType);
			// TODO: calculate decay
			transaction.AddMember("decay", decay, alloc);
			if (orderDESC) {
				// PushFront
				//transactionList.PushBack(transaction, alloc);
				transactionsTempList.push_front(std::move(transaction));
			}
			else {
				transactionsTempList.push_back(std::move(transaction));
			}
		}
		// TODO: skip unknown transaction types and if onlyCreations is set, skip all non creations
		pageIterator++;
	}
	for (auto it = transactionsTempList.begin(); it != transactionsTempList.end(); it++) {
		transactionList.PushBack(*it, alloc);
	}
	transactionsTempList.clear();

	transactionList.AddMember("transactions", transactions, alloc);
	mResponseResult.AddMember("transactionList", transactionList, alloc);
	mResponseResult.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);
}