#include "JsonRPCHandler.h"

#include "gradido_blockchain/blockchain/FilterBuilder.h"
#include "gradido_blockchain/interaction/toJson/Context.h"
#include "gradido_blockchain/interaction/calculateAccountBalance/Context.h"
#include "gradido_blockchain/interaction/calculateCreationSum/Context.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"

#include "../blockchain/FileBased.h"
#include "../blockchain/FileBasedProvider.h"
#include "../blockchain/NodeTransactionEntry.h"

#include "../model/Apollo/TransactionList.h"

#include "rapidjson/prettywriter.h"
#include "magic_enum/magic_enum.hpp"

#include <set>

using namespace rapidjson;
using namespace gradido;
using namespace blockchain;
using namespace interaction;
using namespace magic_enum;

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

	std::shared_ptr<Abstract> blockchain;
	std::string groupAlias;

	// load group for all requests
	if (!getStringParameter(responseJson, params, "groupAlias", groupAlias, true) && 
		!getStringParameter(responseJson, params, "communityId", groupAlias, true)) {
		error(responseJson, JSON_RPC_ERROR_UNKNOWN_GROUP, "neither groupAlias nor communityId were specified");
		return;
	}
	blockchain = FileBasedProvider::getInstance()->findBlockchain(groupAlias);
	if (!blockchain) {
		error(responseJson, JSON_RPC_ERROR_UNKNOWN_GROUP, "community not known");
		return;
	}

	// load public key for nearly all requests
	std::string pubkey;
	std::string pubkeyHex;
	std::set<std::string> noNeedForPubkey = {
		"puttransaction", "getlasttransaction", "getTransactions", "gettransaction"
	};
	if (noNeedForPubkey.find(method) == noNeedForPubkey.end()) {
		if (!getStringParameter(responseJson, params, "pubkey", pubkeyHex)) {
			return;
		}
		pubkey = std::move(*DataTypeConverter::hexToBinString(pubkeyHex).release());
	}

	Value resultJson(kObjectType);
	if (method == "getlasttransaction") {
		Profiler timeUsed;
		std::string format = "base64";
		getStringParameter(responseJson, params, "format", format);
		auto lastTransaction = blockchain->findOne(Filter::LAST_TRANSACTION);
		if (lastTransaction) {
			auto serializedTransaction = lastTransaction->getSerializedTransaction();
			if("base64" == format) {
				auto base64Transaction = serializedTransaction->convertToBase64();
				resultJson.AddMember("transaction", Value(base64Transaction.data(), base64Transaction.size(), alloc), alloc);
			} else if("json" == format) {
				toJson::Context toJson(*lastTransaction->getConfirmedTransaction());
				resultJson.AddMember("transaction", toJson.run(mRootJson), alloc);
			} else {
				error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, "unsupported format");
			}
			resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);
		}
		else {
			error(responseJson, JSON_RPC_ERROR_GRADIDO_NODE_ERROR, "no transaction");
			return;
		}
		
	}
	// TODO: rename to listsinceblock
	else if (method == "getTransactions") {
		std::string format;
		uint64_t transactionId = 0;
		uint32_t maxResultCount = 100;

		if (!getUInt64Parameter(responseJson, params, "fromTransactionId", transactionId) || !getStringParameter(responseJson, params, "format", format)) {
			return;
		}
		getUIntParameter(responseJson, params, "maxResultCount", maxResultCount, true);
		//printf("group: %s, id: %d\n", groupAlias.data(), transactionId);
		FilterBuilder builder;
		auto filter = builder
			.setMinTransactionNr(transactionId)
			.setPagination({ maxResultCount })
			.build();
		findAllTransactions(resultJson, filter, blockchain, format);
	}
	else if (method == "getaddressbalance") {
		std::string date_string;
		if (!getStringParameter(responseJson, params, "date", date_string)) {
			return;
		}

		auto date = DataTypeConverter::dateTimeStringToTimePoint(date_string);				
		std::string coinCommunityId = "";
		if (params.HasMember("coinCommunityId") && params["coinCommunityId"].IsString()) {
			coinCommunityId = params["coinCommunityId"].GetString();
		}
		getAddressBalance(resultJson, pubkey, date, blockchain, coinCommunityId);

	}
	else if (method == "getaddresstype") {
		getAddressType(resultJson, pubkey, blockchain);
	}
	else if (method == "getaddresstxids") {
		getAddressTxids(resultJson, pubkey, blockchain);
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
		std::string format;
		uint64_t transactionId = 0;
		std::shared_ptr<memory::Block> iotaMessageId;

		if (!getStringParameter(responseJson, params, "format", format)) {
			return;
		}
		getUInt64Parameter(responseJson, params, "transactionId", transactionId, true);
		getBinaryFromHexStringParameter(responseJson, params, "iotaMessageId", iotaMessageId, true);
		if (!transactionId && !iotaMessageId) {
			error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, "transactionId or iotaMessageId needed");
			return;
		}

		getTransaction(resultJson, blockchain, format, transactionId, iotaMessageId);
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

		auto date = DataTypeConverter::dateTimeStringToTimePoint(date_string);
		getCreationSumForMonth(resultJson, pubkey, month, year, date, blockchain);
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

		listTransactions(resultJson, blockchain, pubkey, currentPage, pageSize, orderDESC, onlyCreations);
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
		listTransactionsForAddress(resultJson, pubkey, firstTransactionNr, maxResultCount, blockchain);
	}
	else if (method == "puttransaction") {
		std::string base64Transaction;
		uint64_t transactionNr;
		if (!getStringParameter(responseJson, params, "transaction", base64Transaction) || 
			!getUInt64Parameter(responseJson, params, "transactionNr", transactionNr)) {
			return;
		}
		
		auto serializedTransaction = DataTypeConverter::base64ToBinString(base64Transaction);
		auto transaction = std::make_unique<gradido::data::GradidoTransaction>(&serializedTransaction);
		putTransaction(resultJson, transactionNr, std::move(transaction), blockchain);
	}
	else {
		error(responseJson, JSON_RPC_ERROR_METHODE_NOT_FOUND, "method not known");
	}
	
	responseJson.AddMember("result", resultJson, alloc);	
}
void JsonRPCHandler::findAllTransactions(
	rapidjson::Value& resultJson,
	const Filter& filter,
	std::shared_ptr<gradido::blockchain::Abstract> blockchain,
	const std::string& format
)
{
	Profiler timeUsed;
	auto alloc = mRootJson.GetAllocator();

	auto transactions = blockchain->findAll(filter);
	
	if (format == "json") {
		resultJson.AddMember("type", "json", alloc);
	}
	else {
		resultJson.AddMember("type", "base64", alloc);
	}
	Value jsonTransactionArray(kArrayType);
	for (auto it = transactions.begin(); it != transactions.end(); it++) {
		auto transactionSerialized = (*it)->getSerializedTransaction();
		if (transactionSerialized->size() > 0) {
			if (format == "json") {
				auto confirmedTransaction = std::make_unique<gradido::data::ConfirmedTransaction>(transactionSerialized);
				//auto jsonTransaction = Value(gradidoBlock->toJson().data(), alloc);
				toJson::Context toJson(*(*it)->getConfirmedTransaction());
				jsonTransactionArray.PushBack(toJson.run(mRootJson), alloc);
			} else {
				auto base64TransactionString = transactionSerialized->convertToBase64();
				auto base64Transaction = Value(base64TransactionString.data(), base64TransactionString.size(), alloc);
				jsonTransactionArray.PushBack(base64Transaction, alloc);
			}
		}
	}
	resultJson.AddMember("transactions", jsonTransactionArray, alloc);
	resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
}

void JsonRPCHandler::getTransaction(
	rapidjson::Value& resultJson,
	std::shared_ptr<gradido::blockchain::Abstract> blockchain,
	const std::string& format,
	uint64_t transactionId/* = 0*/,
	std::shared_ptr<memory::Block> iotaMessageId /* = nullptr */
)
{
	Profiler timeUsed;
	auto& alloc = mRootJson.GetAllocator();
	
	std::shared_ptr<TransactionEntry> transactionEntry;
	if (transactionId) {
		transactionEntry = blockchain->getTransactionForId(transactionId);
	}
	else {
		transactionEntry = blockchain->findByMessageId(iotaMessageId);
	}
	if (!transactionEntry) {
		error(resultJson, JSON_RPC_ERROR_TRANSACTION_NOT_FOUND, "transaction not found");
		return;
	}
	
	auto transactionSerialized = transactionEntry->getSerializedTransaction();
	if (transactionSerialized->size() > 0) {
		if (format == "json") {
			auto gradidoBlock = std::make_unique<gradido::data::ConfirmedTransaction>(transactionSerialized);
			toJson::Context toJson(*transactionEntry->getConfirmedTransaction());
			resultJson.AddMember("transaction", toJson.run(mRootJson), alloc);
		}
		else {
			auto base64TransactionString = transactionSerialized->convertToBase64();
			resultJson.AddMember("transaction", Value(base64TransactionString.data(), base64TransactionString.size(), alloc), alloc);
		}
	}	

	if (format == "json") {
		resultJson.AddMember("type", "json", alloc);
	}
	else {
		resultJson.AddMember("type", "base64", alloc);
	}
	resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
}

void JsonRPCHandler::getCreationSumForMonth(
	rapidjson::Value& resultJson,
	memory::ConstBlockPtr pubkey,
	Timepoint targetDate,
	Timepoint transactionCreationDate,
	std::shared_ptr<gradido::blockchain::Abstract> blockchain
)
{
	Profiler timeUsed;
	auto& alloc = mRootJson.GetAllocator();
	assert(blockchain);
	
	calculateCreationSum::Context calculateCreationSum(transactionCreationDate, targetDate, pubkey);
	auto sumString = calculateCreationSum.run(*blockchain).toString();
	resultJson.AddMember("sum", Value(sumString.data(), sumString.size(), alloc), alloc);
	resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
}

void JsonRPCHandler::getAddressBalance(
	rapidjson::Value& resultJson,
	memory::ConstBlockPtr pubkey,
	Timepoint date,
	std::shared_ptr<gradido::blockchain::Abstract> blockchain,
	const std::string& coinCommunityId = ""
)
{
	assert(blockchain);
	auto& alloc = mRootJson.GetAllocator();
	calculateAccountBalance::Context calculateAccountBalance(*blockchain);
	auto balanceString = calculateAccountBalance.run(pubkey, date, 0, coinCommunityId).toString();
	
	resultJson.AddMember("balance", Value(balanceString.data(), balanceString.size(), alloc), alloc);
}

void JsonRPCHandler::getAddressType(Value& resultJson, memory::ConstBlockPtr pubkey, std::shared_ptr<gradido::blockchain::Abstract> blockchain)
{
	assert(blockchain);
	auto& alloc = mRootJson.GetAllocator();
	auto typeString = enum_name(blockchain->getAddressType({ 0, 0, pubkey }));

	resultJson.AddMember("addressType", Value(typeString.data(), typeString.size(), alloc), alloc);
}

void JsonRPCHandler::getAddressTxids(Value& resultJson, memory::ConstBlockPtr pubkey, std::shared_ptr<gradido::blockchain::Abstract> blockchain)
{
	assert(blockchain);
	assert(pubkey);

	auto fileBasedBlockchain = std::dynamic_pointer_cast<gradido::blockchain::FileBased>(blockchain);
	assert(fileBasedBlockchain);

	auto transactionNrs = fileBasedBlockchain->findAllFast({ 0, 0, pubkey });

	auto alloc = mRootJson.GetAllocator();
	Value transactionNrsJson(kArrayType);
	for (auto& transactionNr : transactionNrs) {
		transactionNrsJson.PushBack(transactionNr, alloc);
	}

	resultJson.AddMember("transactionNrs", transactionNrsJson, alloc);
}



void JsonRPCHandler::listTransactions(
	rapidjson::Value& resultJson,
	std::shared_ptr<gradido::blockchain::Abstract> blockchain,
	const gradido::blockchain::Filter& filter
)
{

	assert(blockchain);
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
	auto allTransactions = blockchain->findAll(filter);

	model::Apollo::TransactionList transactionList(group, std::move(std::make_unique<std::string>(pubkey)), alloc);
	Timepoint now;
	auto transactionListValue = transactionList.generateList(allTransactions, now, currentPage, pageSize, orderDESC, onlyCreations);

	auto balance = group->calculateAddressBalance(pubkey, "", now, group->getLastTransaction()->getID()+1);
	std::string balanceString;
	gradido::data::TransactionBase::amountToString(&balanceString, balance);
	transactionListValue.AddMember("balance", Value(balanceString.data(), alloc), alloc);

	resultJson.AddMember("transactionList", transactionListValue, alloc);
	resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);
}

void JsonRPCHandler::listTransactionsForAddress(
	Value& resultJson,
	const std::string& userPublicKey,
	uint64_t firstTransactionNr,
	uint32_t maxResultCount,
	std::shared_ptr<controller::Group> group
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
	std::unique_ptr<gradido::data::GradidoTransaction> transaction,
	std::shared_ptr<controller::Group> group
)
{
	assert(!group.isNull());
	Profiler timeUsed;

	transaction->validate(gradido::data::TRANSACTION_VALIDATION_SINGLE);
	group->getArchiveTransactionsOrderer()->addPendingTransaction(std::move(transaction), transactionNr);
	auto alloc = mRootJson.GetAllocator();
	resultJson.AddMember("timeUsed", timeUsed.millis(), alloc);
}
