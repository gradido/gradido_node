#include "ApiHandler.h"

// need to be here, else it produce a linker error, or more precisly the member function generateList
// TODO: fix the reason
#include "../../model/Apollo/TransactionList.h"
#include "gradido_blockchain/blockchain/FilterBuilder.h"
#include "gradido_blockchain/interaction/calculateAccountBalance/Context.h"
#include "gradido_blockchain/interaction/calculateCreationSum/Context.h"
#include "gradido_blockchain/interaction/validate/Context.h"
#include "gradido_blockchain/serialization/toJson.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"

#include "../../blockchain/FileBased.h"
#include "../../blockchain/FileBasedProvider.h"
#include "../../blockchain/NodeTransactionEntry.h"

#include "rapidjson/prettywriter.h"
#include "magic_enum/magic_enum.hpp"
#include "loguru/loguru.hpp"

#include <set>

using namespace rapidjson;
using namespace gradido;
using namespace blockchain;
using namespace interaction;
using namespace serialization;
using namespace data;
using namespace magic_enum;

namespace server {
	namespace json_rpc {

		void ApiHandler::handle(Value& responseJson, std::string method, const Value& params)
		{
			auto alloc = mRootJson.GetAllocator();
			Value resultJson(kObjectType);

			if(method == "listCommunities") {
				listCommunities(resultJson);
				responseJson.AddMember("result", resultJson, alloc);
				return;
			}

#ifdef _DEBUG
			if (method != "puttransaction") {
				// Debugging
				StringBuffer buffer;
				PrettyWriter<StringBuffer> writer(buffer);
				params.Accept(writer);
				std::string requestJsonString(buffer.GetString());
				LOG_F(1, "incoming json-rpc request, params: %s", requestJsonString.data());
			}
#endif // DEBUG

			std::shared_ptr<Abstract> blockchain;
			std::string groupAlias;

			// load group for all requests
			const char* groupAliasKeys[] = { "groupAlias", "communityId", "topic" };
			for (auto key : groupAliasKeys) {
				if (getStringParameter(responseJson, params, key, groupAlias, true)) {
					break;
				}
			}
			printf("groupAlias: %s\n", groupAlias.data());
			if (groupAlias.length() == 0) {
				error(
					responseJson,
					JSON_RPC_ERROR_UNKNOWN_GROUP,
					"neither topic, groupAlias nor communityId were specified"
				);
				return;
			}
			blockchain = FileBasedProvider::getInstance()->findBlockchain(groupAlias);
			if (!blockchain) {
				error(responseJson, JSON_RPC_ERROR_UNKNOWN_GROUP, "community not known");
				return;
			}

			// load public key for nearly all requests
			memory::BlockPtr pubkey;
			std::string pubkeyHex;
			std::set<std::string> noNeedForPubkey = {
				"getLastTransaction", "getTransactions","getTransaction", "findUserByNameHash"
			};
			if (noNeedForPubkey.find(method) == noNeedForPubkey.end()) {
				if (!getStringParameter(responseJson, params, "pubkey", pubkeyHex)) {
					return;
				}
				pubkey = std::make_shared<memory::Block>(memory::Block::fromHex(pubkeyHex));
			}

			if (method == "getLastTransaction") {
				Profiler timeUsed;
				std::string format = "base64";
				getStringParameter(responseJson, params, "format", format);
				auto lastTransaction = blockchain->findOne(Filter::LAST_TRANSACTION);
				if (lastTransaction) {
					auto serializedTransaction = lastTransaction->getSerializedTransaction();
					if ("base64" == format) {
						auto base64Transaction = serializedTransaction->convertToBase64();
						resultJson.AddMember("transaction", Value(base64Transaction.data(), base64Transaction.size(), alloc), alloc);
					}
					else if ("json" == format) {
						resultJson.AddMember("transaction", toJson(*lastTransaction->getConfirmedTransaction(), alloc), alloc);
					}
					else {
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

				if (!getUInt64Parameter(responseJson, params, "fromTransactionId", transactionId) || 
					!getStringParameter(responseJson, params, "format", format)) { return; }
				getUIntParameter(responseJson, params, "maxResultCount", maxResultCount, true);
				//printf("group: %s, id: %d\n", groupAlias.data(), transactionId);
				FilterBuilder builder;
				auto filter = builder
					.setMinTransactionNr(transactionId)
					.setPagination({ maxResultCount })
					.build();
				findAllTransactions(resultJson, filter, blockchain, format);
			}
			else if (method == "getAddressBalance") {
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
			else if (method == "getAddressType") {
				getAddressType(resultJson, pubkey, blockchain);
			}
			else if (method == "getAddressTxids") {
				getAddressTxids(resultJson, pubkey, blockchain);
			}
			else if (method == "getTransaction") {
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

				getTransaction(resultJson, responseJson, blockchain, format, transactionId, iotaMessageId);
			}
			else if (method == "getCreationSumForMonth") {
				int month, year;
				if (!getIntParameter(responseJson, params, "month", month) ||
					!getIntParameter(responseJson, params, "year", year)) {
					return;
				}
				auto ymd = date::year_month_day(date::year(year), date::month(month), date::day(1));
				date::sys_days sysDays = date::sys_days(ymd);
				Timepoint targetDate(sysDays);
				std::string date_string;
				if (!getStringParameter(responseJson, params, "startSearchDate", date_string)) {
					return;
				}

				auto date = DataTypeConverter::dateTimeStringToTimePoint(date_string);
				getCreationSumForMonth(resultJson, pubkey, targetDate, date, blockchain);
			}
			// TODO: think about better name, explain that this is extra formatted for the gradido frontend, to mimic current graphql backend response
			else if (method == "listTransactions") {
				Filter f;
				f.pagination = Pagination(25, 1);
				f.involvedPublicKey = pubkey;
				if (params.HasMember("currentPage") && params["currentPage"].IsInt()) {
					f.pagination.page = params["currentPage"].GetInt();
				}
				if (params.HasMember("pageSize") && params["pageSize"].IsInt()) {
					f.pagination.size = params["pageSize"].GetInt();
				}
				f.searchDirection = SearchDirection::DESC;
				if (params.HasMember("orderDESC") && params["orderDESC"].IsBool() && !params["orderDESC"].GetBool()) {
					f.searchDirection = SearchDirection::ASC;
				}
				if (params.HasMember("onlyCreations") && params["onlyCreations"].IsBool() && params["onlyCreations"].GetBool()) {
					f.transactionType = TransactionType::CREATION;
				}

				listTransactions(resultJson, blockchain, f);
			}
			else if (method == "getTransactionsForAddress") {
				uint64_t firstTransactionNr = 1;
				uint32_t maxResultCount = 0;
				if (params.HasMember("maxResultCount") && params["maxResultCount"].IsUint()) {
					maxResultCount = params["maxResultCount"].GetUint();
				}
				if (params.HasMember("firstTransactionNr") && params["firstTransactionNr"].IsUint64()) {
					firstTransactionNr = params["firstTransactionNr"].GetUint64();
				}
				getTransactionsForAddress(resultJson, pubkey, firstTransactionNr, maxResultCount, blockchain);
			}
			else if (method == "findUserByNameHash") {
				std::string nameHashHex;
				if (!getStringParameter(responseJson, params, "nameHash", nameHashHex)) {
					return;
				}
				findUserByNameHash(
					resultJson,
					responseJson,
					std::make_shared<memory::Block>(memory::Block::fromHex(nameHashHex)),
					blockchain
				);
			}
			else {
				error(responseJson, JSON_RPC_ERROR_METHODE_NOT_FOUND, "method not known");
			}
			if (!responseJson.HasMember("error")) {
				responseJson.AddMember("result", resultJson, alloc);
			}
		}

		void ApiHandler::listCommunities(rapidjson::Value& resultJson)
		{
			Profiler timeUsed;
			auto alloc = mRootJson.GetAllocator();
			auto groups = FileBasedProvider::getInstance()->listCommunityIds();
			resultJson.AddMember("communities", toJson(groups, alloc), alloc);
			resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
		}

		void ApiHandler::findAllTransactions(
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
						jsonTransactionArray.PushBack(toJson(*(*it)->getConfirmedTransaction(), alloc), alloc);
					}
					else {
						auto base64TransactionString = transactionSerialized->convertToBase64();
						auto base64Transaction = Value(base64TransactionString.data(), base64TransactionString.size(), alloc);
						jsonTransactionArray.PushBack(base64Transaction, alloc);
					}
				}
			}
			// read gmw and auf balance
			Timepoint now = std::chrono::system_clock::now();
			calculateAccountBalance::Context calculateAddressBalance(blockchain);
			auto communityRootEntry = blockchain->findOne(Filter::FIRST_TRANSACTION);
			if (communityRootEntry) {
				auto communityRootBody = communityRootEntry->getTransactionBody();
				assert(communityRootBody->isCommunityRoot());
				auto communityRoot = communityRootBody->getCommunityRoot();
				auto gmwAddress = communityRoot->getGmwPubkey();
				auto aufAddress = communityRoot->getAufPubkey();
				auto gmwBalance = calculateAddressBalance.fromEnd(gmwAddress, now);
				auto aufBalance = calculateAddressBalance.fromEnd(aufAddress, now);
				resultJson.AddMember("gmwBalance", Value(gmwBalance.toString().data(), alloc), alloc);
				resultJson.AddMember("aufBalance", Value(aufBalance.toString().data(), alloc), alloc);
			} else {
				resultJson.AddMember("gmwBalance", Value("0", alloc), alloc);
				resultJson.AddMember("aufBalance", Value("0", alloc), alloc);
			}

			resultJson.AddMember("transactions", jsonTransactionArray, alloc);
			resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
		}

		void ApiHandler::getTransaction(
			Value& resultJson,
			Value& responseJson,
			std::shared_ptr<Abstract> blockchain,
			const std::string& format,
			uint64_t transactionId/* = 0*/,
			std::shared_ptr<memory::Block> iotaMessageId /* = nullptr */
		)
		{
			Profiler timeUsed;
			auto& alloc = mRootJson.GetAllocator();

			std::shared_ptr<const TransactionEntry> transactionEntry;
			if (transactionId) {
				transactionEntry = blockchain->getTransactionForId(transactionId);
			}
			else {
				transactionEntry = blockchain->findByMessageId(iotaMessageId);
			}
			if (!transactionEntry) {
				error(responseJson, JSON_RPC_ERROR_TRANSACTION_NOT_FOUND, "transaction not found");
				return;
			}

			auto transactionSerialized = transactionEntry->getSerializedTransaction();
			if (transactionSerialized->size() > 0) {
				if (format == "json") {
					resultJson.AddMember("transaction", toJson(*transactionEntry->getConfirmedTransaction(), alloc), alloc);
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

		void ApiHandler::getCreationSumForMonth(
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

		void ApiHandler::getAddressBalance(
			rapidjson::Value& resultJson,
			memory::ConstBlockPtr pubkey,
			Timepoint date,
			std::shared_ptr<gradido::blockchain::Abstract> blockchain,
			const std::string& coinCommunityId /* = "" */
		)
		{
			assert(blockchain);
			auto& alloc = mRootJson.GetAllocator();
			calculateAccountBalance::Context calculateAccountBalance(blockchain);
			// TODO: add coinCommunity�d Filter to calculateAccountBalance Context
			auto balanceString = calculateAccountBalance.fromEnd(pubkey, date, 0).toString();

			resultJson.AddMember("balance", Value(balanceString.data(), balanceString.size(), alloc), alloc);
		}

		void ApiHandler::getAddressType(Value& resultJson, memory::ConstBlockPtr pubkey, std::shared_ptr<gradido::blockchain::Abstract> blockchain)
		{
			assert(blockchain);
			auto& alloc = mRootJson.GetAllocator();
			auto typeString = enum_name(blockchain->getAddressType({ 0, 0, pubkey }));

			resultJson.AddMember("addressType", Value(typeString.data(), typeString.size(), alloc), alloc);
		}

		void ApiHandler::getAddressTxids(Value& resultJson, memory::ConstBlockPtr pubkey, std::shared_ptr<gradido::blockchain::Abstract> blockchain)
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



		void ApiHandler::listTransactions(
			Value& resultJson,
			std::shared_ptr<Abstract> blockchain,
			const Filter& filter
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
			auto& alloc = mRootJson.GetAllocator();

			model::Apollo::TransactionList transactionList(blockchain, filter.involvedPublicKey);
			Timepoint now = std::chrono::system_clock::now();

			auto transactionListValue = transactionList.generateList(now, filter, mRootJson);

			calculateAccountBalance::Context calculateAddressBalance(blockchain);
			auto balance = calculateAddressBalance.fromEnd(filter.involvedPublicKey, now);
			std::string balanceString = balance.toString();
			transactionListValue.AddMember("balance", Value(balanceString.data(), balanceString.size(), alloc), alloc);
			resultJson.AddMember("transactionList", transactionListValue, alloc);
			resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);
		}

		void ApiHandler::getTransactionsForAddress(
			Value& resultJson,
			memory::ConstBlockPtr pubkey,
			uint64_t firstTransactionNr,
			uint32_t maxResultCount,
			std::shared_ptr<gradido::blockchain::Abstract> blockchain
		)
		{
			Profiler timeUsed;
			Filter f;
			f.involvedPublicKey = pubkey;
			f.minTransactionNr = firstTransactionNr;
			f.pagination.size = maxResultCount;
			auto transactions = blockchain->findAll(f);
			auto& alloc = mRootJson.GetAllocator();
			Value jsonTransactionsArray(kArrayType);
			for (auto transaction : transactions) {
				auto serializedTransactionBase64 = transaction->getSerializedTransaction()->convertToBase64();
				jsonTransactionsArray.PushBack(Value(serializedTransactionBase64.data(), serializedTransactionBase64.size(), alloc), alloc);
			}
			resultJson.AddMember("transactions", jsonTransactionsArray, alloc);
			resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);
		}

		void ApiHandler::findUserByNameHash(
			Value& resultJson,
			Value& responseJson,
			memory::ConstBlockPtr nameHash,
			std::shared_ptr<gradido::blockchain::Abstract> blockchain
		)
		{
			Profiler timeUsed;
			Filter f;
			f.transactionType = data::TransactionType::REGISTER_ADDRESS;
			// std::function<FilterResult(const TransactionEntry&)> filterFunction;
			f.filterFunction = [nameHash](const TransactionEntry& entry) {
				auto body = entry.getTransactionBody();
				assert(body->isRegisterAddress());
				auto registerAddress = body->getRegisterAddress();
				if (registerAddress->getNameHash()->isTheSame(nameHash)) {
					return FilterResult::USE | FilterResult::STOP;
				}
				return FilterResult::DISMISS;
			};
			auto transactions = blockchain->findAll(f);
			auto& alloc = mRootJson.GetAllocator();
			resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc), alloc);

			if (transactions.size() > 0) {
				auto body = transactions.front()->getTransactionBody();
				assert(body);
				auto registerAddress = body->getRegisterAddress();
				assert(registerAddress);
				auto accountPubkey = registerAddress->getAccountPublicKey();
				assert(accountPubkey);
				resultJson.AddMember("pubkey", Value(accountPubkey->convertToHex().data(), alloc), alloc);
			}
			else {
				error(responseJson, JSON_RPC_ERROR_ADDRESS_NOT_FOUND, "user not found");
			}
		}
	}
}

