#include "JsonRPCHandler.h"

#include "../lib/BinTextConverter.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "../SingletonManager/GroupManager.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"

#include "Poco/AutoPtr.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/Timezone.h"

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

	Poco::SharedPtr<controller::Group> group;
	std::string groupAlias;
	
	// load group for all requests
	if (!getStringParameter(params, "groupAlias", groupAlias)) {
		mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
		return;
	}
	auto gm = GroupManager::getInstance();
	group = gm->findGroup(groupAlias);
	if (group.isNull()) {
		mResponseErrorCode = JSON_RPC_ERROR_UNKNOWN_GROUP;
		stateError("group not known");
		return;
	}
	
	// load public key for all requests
	std::string pubkey;	
	std::string pubkeyHex;
	if (!getStringParameter(params, "pubkey", pubkeyHex)) {
		mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
		return;
	}
	pubkey = std::move(*DataTypeConverter::hexToBinString(pubkeyHex).release());
	
	int timezoneDifferential = Poco::Timezone::tzd();

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
		std::string date_string;
		if (!getStringParameter(params, "date", date_string)) {
			mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
			return;
		}
		
		Poco::DateTime date;
		try {
			date = Poco::DateTimeParser::parse(date_string, timezoneDifferential);
		}
		catch (Poco::Exception& ex) {
			stateError("cannot parse date", ex.what());
			return;
		}
		std::string coinGroupId = "";
		getStringParameter(params, "coinGroupId", coinGroupId);
		getAddressBalance(pubkey, date, group, coinGroupId);
		
	}
	else if (method == "getaddresstype") {
		getAddressType(pubkey, group);
	}
	else if (method == "getaddresstxids") {
		getAddressTxids(pubkey, group);
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
	else if (method == "getcreationsumformonth") {
		int month = 0, year = 0;
		if (!getIntParameter(params, "month", month) || 
			!getIntParameter(params, "year", year)) {
			mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
			return;
		}
		std::string date_string;
		if (!getStringParameter(params, "startSearchDate", date_string)) {
			mResponseErrorCode = JSON_RPC_ERROR_INVALID_PARAMS;
			return;
		}

		Poco::DateTime date;
		try {
			date = Poco::DateTimeParser::parse(date_string, timezoneDifferential);
		}
		catch (Poco::Exception& ex) {
			stateError("cannot parse date", ex.what());
			return;
		}
		getCreationSumForMonth(pubkey, month, year, date, group);
	}
	else if (method == "listtransactions") {
		int currentPage = 1, pageSize = 25;
		getIntParameter(params, "currentPage", currentPage);
		getIntParameter(params, "pageSize", pageSize);
		bool orderDESC = true, onlyCreations = false;
		getBoolParameter(params, "orderDESC", orderDESC);
		getBoolParameter(params, "onlyCreations", onlyCreations);

		return listTransactions(groupAlias, pubkey, currentPage, pageSize, orderDESC, onlyCreations);
	}
	else {
		mResponseErrorCode = JSON_RPC_ERROR_METHODE_NOT_FOUND;
		stateError("method not known");
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

void JsonRPCHandler::getAddressBalance(
	const std::string& pubkey, 
	Poco::DateTime date, 
	Poco::SharedPtr<controller::Group> group, 
	const std::string& coinGroupId /* = "" */
)
{
	assert(!group.isNull());
	auto alloc = mResponseJson.GetAllocator();

	auto balance = group->calculateAddressBalance(pubkey, coinGroupId, date);
	std::string balanceString;
	model::gradido::TransactionBase::amountToString(&balanceString, balance);
	MemoryManager::getInstance()->releaseMathMemory(balance);

	stateSuccess();
	mResponseResult.AddMember("balance", Value(balanceString.data(), alloc), alloc);
}

void JsonRPCHandler::getAddressType(const std::string& pubkey, Poco::SharedPtr<controller::Group> group)
{
	assert(!group.isNull());
	auto alloc = mResponseJson.GetAllocator();

	auto type = group->getAddressType(pubkey);
	stateSuccess();
	mResponseResult.AddMember("addressType", Value(model::gradido::RegisterAddress::getAddressStringFromType(type).data(), alloc), alloc);
}

void JsonRPCHandler::getAddressTxids(const std::string& pubkey, Poco::SharedPtr<controller::Group> group)
{
	assert(!group.isNull());
	assert(pubkey.size());
	auto transactionNrs = group->findTransactionIds(pubkey);

	auto alloc = mResponseJson.GetAllocator();
	Value transactionNrsJson(kArrayType);
	std::for_each(transactionNrs.begin(), transactionNrs.end(), [&](const uint64_t& value) {
		transactionNrsJson.PushBack(value, alloc);
	});
	stateSuccess();
	mResponseResult.AddMember("transactionNrs", transactionNrsJson, alloc);
}

void JsonRPCHandler::getCreationSumForMonth(const std::string& pubkey, int month, int year, Poco::DateTime searchStartDate, Poco::SharedPtr<controller::Group> group)
{
	assert(!group.isNull());
	auto sum = MathMemory::create();
	group->calculateCreationSum(pubkey, month, year, searchStartDate, sum->getData());
	std::string sumString;
	model::gradido::TransactionBase::amountToString(&sumString, sum->getData());
	stateSuccess();
	auto alloc = mResponseJson.GetAllocator();
	mResponseResult.AddMember("sum", Value(sumString.data(), alloc), alloc);
}

void JsonRPCHandler::listTransactions(
	const std::string& groupAlias,
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
	auto alloc = mResponseJson.GetAllocator();
	auto gm = GroupManager::getInstance();
	auto group = gm->findGroup(groupAlias);
	if (group.isNull()) {
		mResponseErrorCode = JSON_RPC_ERROR_UNKNOWN_GROUP;
		stateError("group not known");
		return;
	}

	auto allTransactions = group->findTransactions(pubkey);

	model::Apollo::TransactionList transactionList(group, std::move(std::make_unique<std::string>(pubkey)), alloc);
	Poco::Timestamp now;
	auto transactionListValue = transactionList.generateList(allTransactions, now, currentPage, pageSize, orderDESC, onlyCreations);

	auto balance = group->calculateAddressBalance(pubkey, 0, now);
	std::string balanceString;
	model::gradido::TransactionBase::amountToString(&balanceString, balance);
	transactionListValue.AddMember("balance", Value(balanceString.data(), alloc), alloc);

	stateSuccess();
	mResponseResult.AddMember("transactionList", transactionListValue, alloc);
	mResponseResult.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);
}