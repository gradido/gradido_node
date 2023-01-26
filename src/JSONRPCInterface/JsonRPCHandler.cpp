#include "JsonRPCHandler.h"

#include "../lib/BinTextConverter.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/LoggerManager.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"

#include "Poco/AutoPtr.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/Timezone.h"
#include "Poco/Logger.h"

#include "../model/NodeTransactionEntry.h"
#include "../model/Apollo/TransactionList.h"

#include "rapidjson/prettywriter.h"

using namespace rapidjson;

void JsonRPCHandler::handle(Value& responseJson, std::string method, const Value& params)
{
	auto alloc = mRootJson.GetAllocator();

#ifdef _DEBUG
	if (method != "puttransaction") {
		// Debugging
		StringBuffer buffer;
		PrettyWriter<StringBuffer> writer(buffer);
		params.Accept(writer);
		std::string requestJsonString(buffer.GetString());
		Poco::Logger::get("requestLog").debug("incoming json-rpc request, params: %s", requestJsonString);
	}
#endif // DEBUG

	Poco::SharedPtr<controller::Group> group;
	std::string groupAlias;

	// load group for all requests
	if (!getStringParameter(responseJson, params, "groupAlias", groupAlias)) {
		return;
	}
	auto gm = GroupManager::getInstance();
	group = gm->findGroup(groupAlias);
	if (group.isNull()) {
		error(responseJson, JSON_RPC_ERROR_UNKNOWN_GROUP, "group not known");
		return;
	}

	// load public key for nearly all requests
	std::string pubkey;
	std::string pubkeyHex;
	std::set<std::string> noNeedForPubkey = {
		"puttransaction", "getlasttransaction"
	};
	if (noNeedForPubkey.find(method) == noNeedForPubkey.end()) {
		if (!getStringParameter(responseJson, params, "pubkey", pubkeyHex)) {
			return;
		}
		pubkey = std::move(*DataTypeConverter::hexToBinString(pubkeyHex).release());
	}

	int timezoneDifferential = Poco::Timezone::tzd();

	Value resultJson(kObjectType);
	if (method == "getlasttransaction") {
		auto lastTransaction = group->getLastTransaction();
		if (!lastTransaction.isNull()) {
			auto serializedTransaction = lastTransaction->getSerialized();
			auto base64Transaction = DataTypeConverter::binToBase64(std::move(serializedTransaction));
			resultJson.AddMember("transaction", Value(base64Transaction->data(), alloc), alloc);
		}
		else {
			error(responseJson, JSON_RPC_ERROR_GRADIDO_NODE_ERROR, "no transaction");
			return;
		}
		
	}
	// TODO: rename to listsinceblock
	else if (method == "getTransactions") {
		//printf("getTransactions called\n");
		if(!params.IsObject()) {
			error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, "params not an object");
			return;
		}
		std::string format;
		uint64_t transactionId = 0;

		if (!getUInt64Parameter(responseJson, params, "fromTransactionId", transactionId) || !getStringParameter(responseJson, params, "format", format)) {
			return;
		}
		//printf("group: %s, id: %d\n", groupAlias.data(), transactionId);
		getTransactions(resultJson, transactionId, group, format);
	}
	else if (method == "getaddressbalance") {
		std::string date_string;
		if (!getStringParameter(responseJson, params, "date", date_string)) {
			return;
		}

		Poco::DateTime date;
		try {
			date = Poco::DateTimeParser::parse(date_string, timezoneDifferential);
		}
		catch (Poco::Exception& ex) {
			Value data(kObjectType);
			data.AddMember("PocoException", Value(ex.what(), alloc), alloc);
			error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, "cannot parse date", data);
			return;
		}
		std::string coinGroupId = "";
		if (params.HasMember("coinGroupId") && params["coinGroupId"].IsString()) {
			coinGroupId = params["coinGroupId"].GetString();
		}
		getAddressBalance(resultJson, pubkey, date, group, coinGroupId);

	}
	else if (method == "getaddresstype") {
		getAddressType(resultJson, pubkey, group);
	}
	else if (method == "getaddresstxids") {
		getAddressTxids(resultJson, pubkey, group);
	}
	else if (method == "getblock") {
		error(responseJson, JSON_RPC_ERROR_NOT_IMPLEMENTED, "getblock not implemented yet");
		return;
	}
	else if (method == "getblockcount") {
		error(responseJson, JSON_RPC_ERROR_NOT_IMPLEMENTED, "getblockcount not implemented yet");
		return;
	}
	else if (method == "getblockhash") {
		error(responseJson, JSON_RPC_ERROR_NOT_IMPLEMENTED, "getblockhash not implemented yet");
		return;
	}
	else if (method == "getreceivedbyaddress") {
		error(responseJson, JSON_RPC_ERROR_NOT_IMPLEMENTED, "getreceivedbyaddress not implemented yet");
		return;
	}
	else if (method == "gettransaction") {
		error(responseJson, JSON_RPC_ERROR_NOT_IMPLEMENTED, "gettransaction not implemented yet");
		return;
	}
	else if (method == "getcreationsumformonth") {
		int month = 0, year = 0;
		if (!getIntParameter(responseJson, params, "month", month) ||
			!getIntParameter(responseJson, params, "year", year)) {
			return;
		}
		std::string date_string;
		if (!getStringParameter(responseJson, params, "startSearchDate", date_string)) {
			return;
		}

		Poco::DateTime date;
		try {
			date = Poco::DateTimeParser::parse(date_string, timezoneDifferential);
		}
		catch (Poco::Exception& ex) {
			Value data(kObjectType);
			data.AddMember("PocoException", Value(ex.what(), alloc), alloc);
			error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, "cannot parse date", data);
			return;
		}
		getCreationSumForMonth(resultJson, pubkey, month, year, date, group);
	}
	else if (method == "listtransactions") {
		int currentPage = 1, pageSize = 25;
		if (params.HasMember("currentPage") && params["currentPage"].IsInt()) {
			currentPage = params["currentPage"].GetInt();
		}
		if (params.HasMember("pageSize") && params["pageSize"].IsInt()) {
			pageSize = params["pageSize"].GetInt();
		}
		bool orderDESC = true, onlyCreations = false;
		if (params.HasMember("orderDESC") && params["orderDESC"].IsBool()) {
			orderDESC = params["orderDESC"].GetBool();
		}
		if (params.HasMember("onlyCreations") && params["onlyCreations"].IsBool()) {
			onlyCreations = params["onlyCreations"].GetBool();
		}

		listTransactions(resultJson, group, pubkey, currentPage, pageSize, orderDESC, onlyCreations);
	}
	else if (method == "listtransactionsforaddress") {
		uint64_t firstTransactionNr = 1;
		uint32_t maxResultCount = 0;
		if (params.HasMember("maxResultCount") && params["maxResultCount"].IsUint()) {
			maxResultCount = params["maxResultCount"].GetUint();
		}
		if (params.HasMember("firstTransactionNr") && params["firstTransactionNr"].IsUint64()) {
			firstTransactionNr = params["firstTransactionNr"].GetUint64();
		}
		listTransactionsForAddress(resultJson, pubkey, firstTransactionNr, maxResultCount, group);
	}
	else if (method == "puttransaction") {
		std::string base64Transaction;
		uint64_t transactionNr;
		if (!getStringParameter(responseJson, params, "transaction", base64Transaction) || 
			!getUInt64Parameter(responseJson, params, "transactionNr", transactionNr)) {
			return;
		}
		
		auto serializedTransaction = DataTypeConverter::base64ToBinString(base64Transaction);
		auto transaction = std::make_unique<model::gradido::GradidoTransaction>(&serializedTransaction);
		putTransaction(resultJson, transactionNr, std::move(transaction), group);
	}
	else {
		error(responseJson, JSON_RPC_ERROR_METHODE_NOT_FOUND, "method not known");
	}
	
	responseJson.AddMember("result", resultJson, alloc);	
}

void JsonRPCHandler::getTransactions(
	Value& resultJson, 
	int64_t fromTransactionId, 
	Poco::SharedPtr<controller::Group> group, 
	const std::string& format
)
{
	Profiler timeUsed;
	auto alloc = mRootJson.GetAllocator();

	//printf("group found and loaded\n");
	auto transactions = group->findTransactionsFromXToLast(fromTransactionId);
	//printf("%d transactions for group: %s found\n", transactions.size(), groupAlias.data());
	
	if (format == "json") {
		resultJson.AddMember("type", "json", alloc);
	}
	else {
		resultJson.AddMember("type", "base64", alloc);
	}
	Value jsonTransactionArray(kArrayType);
	Poco::AutoPtr<model::gradido::GradidoBlock> prevTransaction;
	for (auto it = transactions.begin(); it != transactions.end(); it++) {

		auto transactionSerialized = (*it)->getSerializedTransaction();
		if (transactionSerialized->size() > 0) {
			if (format == "json") {
				auto gradidoBlock = std::make_unique<model::gradido::GradidoBlock>(transactionSerialized);
				//auto jsonTransaction = Value(gradidoBlock->toJson().data(), alloc);
				auto jsonTransaction = gradidoBlock->toJson(mRootJson);
				jsonTransactionArray.PushBack(jsonTransaction, alloc);
			} else {
				auto base64Transaction = Value(DataTypeConverter::binToBase64(*transactionSerialized).data(), alloc);
				jsonTransactionArray.PushBack(base64Transaction, alloc);
			}
		}
	}
	resultJson.AddMember("transactions", jsonTransactionArray, alloc);
	resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
}

void JsonRPCHandler::getAddressBalance(
	Value& resultJson,
	const std::string& pubkey,
	Poco::DateTime date,
	Poco::SharedPtr<controller::Group> group,
	const std::string& coinGroupId /* = "" */
)
{
	assert(!group.isNull());
	auto alloc = mRootJson.GetAllocator();

	auto balance = group->calculateAddressBalance(pubkey, coinGroupId, date);
	std::string balanceString;
	model::gradido::TransactionBase::amountToString(&balanceString, balance);
	MemoryManager::getInstance()->releaseMathMemory(balance);

	resultJson.AddMember("balance", Value(balanceString.data(), alloc), alloc);
}

void JsonRPCHandler::getAddressType(Value& resultJson, const std::string& pubkey, Poco::SharedPtr<controller::Group> group)
{
	assert(!group.isNull());
	auto alloc = mRootJson.GetAllocator();
	auto type = group->getAddressType(pubkey);

	resultJson.AddMember("addressType", Value(model::gradido::RegisterAddress::getAddressStringFromType(type).data(), alloc), alloc);
}

void JsonRPCHandler::getAddressTxids(Value& resultJson, const std::string& pubkey, Poco::SharedPtr<controller::Group> group)
{
	assert(!group.isNull());
	assert(pubkey.size());
	auto transactionNrs = group->findTransactionIds(pubkey);

	auto alloc = mRootJson.GetAllocator();
	Value transactionNrsJson(kArrayType);
	std::for_each(transactionNrs.begin(), transactionNrs.end(), [&](const uint64_t& value) {
		transactionNrsJson.PushBack(value, alloc);
	});

	resultJson.AddMember("transactionNrs", transactionNrsJson, alloc);
}

void JsonRPCHandler::getCreationSumForMonth(
	Value& resultJson,
	const std::string& pubkey, 
	int month, 
	int year, 
	Poco::DateTime searchStartDate, 
	Poco::SharedPtr<controller::Group> group
)
{
	assert(!group.isNull());
	auto sum = model::gradido::TransactionCreation::calculateCreationSum(pubkey, month, year, searchStartDate, group);
	std::string sumString;
	model::gradido::TransactionBase::amountToString(&sumString, sum);
	MemoryManager::getInstance()->releaseMathMemory(sum);
	auto alloc = mRootJson.GetAllocator();
	resultJson.AddMember("sum", Value(sumString.data(), alloc), alloc);
}

void JsonRPCHandler::listTransactions(
	Value& resultJson,
	Poco::SharedPtr<controller::Group> group,
	const std::string& pubkey,
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
	auto alloc = mRootJson.GetAllocator();
	auto allTransactions = group->findTransactions(pubkey);

	model::Apollo::TransactionList transactionList(group, std::move(std::make_unique<std::string>(pubkey)), alloc);
	Poco::Timestamp now;
	auto transactionListValue = transactionList.generateList(allTransactions, now, currentPage, pageSize, orderDESC, onlyCreations);

	auto balance = group->calculateAddressBalance(pubkey, "", now);
	std::string balanceString;
	model::gradido::TransactionBase::amountToString(&balanceString, balance);
	transactionListValue.AddMember("balance", Value(balanceString.data(), alloc), alloc);

	resultJson.AddMember("transactionList", transactionListValue, alloc);
	resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);
}

void JsonRPCHandler::listTransactionsForAddress(
	Value& resultJson,
	const std::string& userPublicKey,
	uint64_t firstTransactionNr,
	uint32_t maxResultCount,
	Poco::SharedPtr<controller::Group> group
)
{
	Profiler timeUsed;
	auto transactions = group->findTransactions(userPublicKey, maxResultCount, firstTransactionNr);
	auto alloc = mRootJson.GetAllocator();
	Value jsonTransactionsArray(kArrayType);
	for (auto transaction : transactions) {
		auto serializedTransactionBase64 = DataTypeConverter::binToBase64(*transaction->getSerializedTransaction());
		jsonTransactionsArray.PushBack(Value(serializedTransactionBase64.data(), alloc), alloc);
	}
	resultJson.AddMember("transactions", jsonTransactionsArray, alloc);
	resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);
}

void JsonRPCHandler::putTransaction(
	Value& resultJson,
	uint64_t transactionNr,
	std::unique_ptr<model::gradido::GradidoTransaction> transaction,
	Poco::SharedPtr<controller::Group> group
)
{
	assert(!group.isNull());
	Profiler timeUsed;

	transaction->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
	group->getArchiveTransactionsOrderer()->addPendingTransaction(std::move(transaction), transactionNr);
	auto alloc = mRootJson.GetAllocator();
	resultJson.AddMember("timeUsed", timeUsed.millis(), alloc);
}
