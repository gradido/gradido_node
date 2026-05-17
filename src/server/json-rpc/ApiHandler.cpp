#include "ApiHandler.h"
#include "fromJson.h"
#include "WireFilter.h"

// need to be here, else it produce a linker error, or more precisly the member function generateList
// TODO: fix the reason
#include "../../model/Apollo/TransactionList.h"
#include "gradido_blockchain/AppContext.h"
#include "gradido_blockchain/blockchain/CompactFilter.h"
#include "gradido_blockchain/blockchain/FilterBuilder.h"
#include "gradido_blockchain/data/adapter/byteArray.h"
#include "gradido_blockchain/data/adapter/publicKey.h"
#include "gradido_blockchain/data/compact/ConfirmedGradidoTx.h"
#include "gradido_blockchain/data/compact/PublicKeyIndex.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"
#include "gradido_blockchain/data/LedgerAnchor.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"
#include "gradido_blockchain/interaction/calculateAccountBalance/Context.h"
#include "gradido_blockchain/interaction/calculateCreationSum/Context.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/interaction/serialize/Context.h"
#include "gradido_blockchain/interaction/validate/Context.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/MonotonicTimer.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/serialization/toJson.h"

#include "../../blockchain/FileBased.h"
#include "../../blockchain/FileBasedProvider.h"
#include "../../blockchain/NodeTransactionEntry.h"

#include <rapidjson/document.h>
#include "rapidjson/prettywriter.h"
#include "magic_enum/magic_enum.hpp"
#include "loguru/loguru.hpp"

#include <optional>
#include <set>

using namespace rapidjson;
using namespace gradido;
using namespace blockchain;
using namespace interaction;
using namespace serialization;
using namespace data;
using namespace magic_enum;

using std::optional, std::nullopt;
using gradido::g_appContext;
using gradido::data::compact::PublicKeyIndex, gradido::data::compact::ConfirmedTxs;

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
			// printf("groupAlias: %s\n", groupAlias.data());
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
			PublicKeyIndex publickKeyIndex;

			std::string pubkeyHex;
			std::set<std::string> noNeedForPubkey = {
				"getLastTransaction", "getTransactions","getTransaction", "findUserByNameHash"
			};
			if (noNeedForPubkey.find(method) == noNeedForPubkey.end()) {
				if (!getStringParameter(responseJson, params, "pubkey", pubkeyHex)) {
					return;
				}
				pubkey = std::make_shared<memory::Block>(memory::Block::fromHex(pubkeyHex));
				publickKeyIndex = adapter::toPublicKeyIndex(pubkey, blockchain->getCommunityIdIndex());
			}

			if (method == "getLastTransaction") {
				MonotonicTimer timeUsed;
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
				WireFilter filter;
				auto result = fromJson(params, filter);
				if (JsonParseResultType::Ok != result.type) {
					error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, result.error.c_str());
				}
				findAllTransactions(resultJson, filter.toCompactFilter(*g_appContext), blockchain, filter.format);
			}
			else if (method == "getAddressBalance") {
				std::string date_string;
				if (!getStringParameter(responseJson, params, "date", date_string)) {
					return;
				}

				auto date = DataTypeConverter::dateTimeStringToTimePoint(date_string);
				optional<uint32_t> coinCommunityId = nullopt;
				if (params.HasMember("coinCommunityId") && params["coinCommunityId"].IsString()) {
					auto coinCommunityIdIndexOptional = g_appContext->getCommunityIds().getIndexForData(params["coinCommunityId"].GetString());
					if (coinCommunityIdIndexOptional) {
						coinCommunityId = static_cast<uint32_t>(coinCommunityIdIndexOptional);
					}
				}
				getAddressBalance(resultJson, pubkey, date, blockchain, coinCommunityId);
			}
			else if (method == "getAddressType") {
				getAddressType(resultJson, pubkey, blockchain);
			}
			else if (method == "getTransaction") {
				std::string format;
				uint64_t transactionId = 0;
				std::string hieroTransactionIdString;
				LedgerAnchor ledgerAnchor;
				std::shared_ptr<const memory::Block> iotaMessageId;

				if (!getStringParameter(responseJson, params, "format", format)) {
					return;
				}
				getUInt64Parameter(responseJson, params, "transactionId", transactionId, true);
				getStringParameter(responseJson, params, "hieroTransactionId", hieroTransactionIdString, true);
				getBinaryFromHexStringParameter(responseJson, params, "iotaMessageId", iotaMessageId, true);
				if (iotaMessageId) {
					error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, "iotaMessageId is not longer supported");
				}
				if (!hieroTransactionIdString.empty()) {
					ledgerAnchor = LedgerAnchor(hiero::TransactionId(hieroTransactionIdString));
				}
				if (!transactionId && !iotaMessageId && ledgerAnchor.empty()) {
					error(responseJson, JSON_RPC_ERROR_INVALID_PARAMS, "transactionId or hieroTransactionId needed");
					return;
				}

				getTransaction(resultJson, responseJson, blockchain, format, transactionId, &ledgerAnchor);
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
				getCreationSumForMonth(resultJson, publickKeyIndex, targetDate, date, blockchain);
			}
			// TODO: think about better name, explain that this is extra formatted for the gradido frontend, to mimic current graphql backend response
			else if (method == "listTransactions") {
				Filter f;
				f.pagination = Pagination(25, 1);
				f.updatedBalancePublicKey = pubkey;
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
			MonotonicTimer timeUsed;
			auto& alloc = mRootJson.GetAllocator();
			const auto& groupIndex = FileBasedProvider::getInstance()->getGroupIndex();
			Value communities(kArrayType);
			groupIndex->iterate(
				[&communities, &alloc](const cache::CommunityIndexEntry& comInfos) -> bool
				{
					Value community(kObjectType);
					community.AddMember("communityId", toJson(comInfos.communityId, alloc), alloc);
					community.AddMember("alias", toJson(comInfos.alias, alloc), alloc);
					communities.PushBack(community, alloc);
					return true;
				}
			);
			resultJson.AddMember("communities", communities, alloc);
			resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
		}

		void ApiHandler::findAllTransactions(
			rapidjson::Value& resultJson,
			const CompactFilter& filter,
			std::shared_ptr<gradido::blockchain::Abstract> blockchain,
			WireOutputFormat format
		)
		{
			MonotonicTimer timeUsed;
			auto& alloc = mRootJson.GetAllocator();

			// count for pagination
			CompactFilter countFilter = filter;
			countFilter.pagination = Pagination(); // remove pagination for count
			countFilter.minTransactionNr = 0; // remove minTransactionNr for count
			countFilter.maxTransactionNr = 0; // remove maxTransactionNr for count
			auto totalCount = blockchain->countAll(countFilter);

			resultJson.AddMember("totalCount", totalCount, alloc);

			auto transactions = blockchain->findAll(filter);

			if (WireOutputFormat::Json == format) {
				resultJson.AddMember("type", "json", alloc);
			}
			else {
				resultJson.AddMember("type", "base64", alloc);
			}
			Value jsonTransactionArray(kArrayType);
			for (auto it = transactions.begin(); it != transactions.end(); it++) {
				auto legacyTx = blockchain->getTransactionForId((*it)->txNr);
				auto transactionSerialized = legacyTx->getSerializedTransaction();
				if (transactionSerialized->size() > 0) {
					if (WireOutputFormat::Json == format) {
						jsonTransactionArray.PushBack(toJson(*legacyTx->getConfirmedTransaction(), alloc), alloc);
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
			CompactFilter communityRootFindFilter;
			communityRootFindFilter.searchDirection = SearchDirection::ASC;
			communityRootFindFilter.pagination.size = 1;
			auto communityRootEntry = blockchain->findOne(communityRootFindFilter);
			if (communityRootEntry) {
				auto& tx = *communityRootEntry;
				assert(tx.isCommunityRoot());
				auto gmwBalance = calculateAddressBalance.fromEnd(tx.getGmw(), now, blockchain->getCommunityIdIndex());
				auto aufBalance = calculateAddressBalance.fromEnd(tx.getAuf(), now, blockchain->getCommunityIdIndex());
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
			gradido::data::LedgerAnchor* ledgerAnchor/* = nullptr */
		)
		{
			MonotonicTimer timeUsed;
			auto& alloc = mRootJson.GetAllocator();

			std::shared_ptr<const TransactionEntry> transactionEntry;
			if (transactionId) {
				transactionEntry = blockchain->getTransactionForId(transactionId);
			}
			else {
				if (ledgerAnchor && !ledgerAnchor->empty()) {
					transactionEntry = blockchain->findByLedgerAnchor(*ledgerAnchor);
				}
			}
			if (!transactionEntry) {
				printf("not found after: %s\n", timeUsed.string().c_str());
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
			gradido::data::compact::PublicKeyIndex publicKeyIndex,
			Timepoint targetDate,
			Timepoint transactionCreationDate,
			std::shared_ptr<gradido::blockchain::Abstract> blockchain
		)
		{
			MonotonicTimer timeUsed;
			auto& alloc = mRootJson.GetAllocator();
			assert(blockchain);

			calculateCreationSum::Context calculateCreationSum(transactionCreationDate, targetDate, publicKeyIndex);
			auto sumString = calculateCreationSum.run(*blockchain).toString();
			resultJson.AddMember("sum", Value(sumString.data(), sumString.size(), alloc), alloc);
			resultJson.AddMember("timeUsed", Value(timeUsed.string().data(), alloc).Move(), alloc);
		}

		void ApiHandler::getAddressBalance(
			rapidjson::Value& resultJson,
			memory::ConstBlockPtr pubkey,
			Timepoint date,
			std::shared_ptr<gradido::blockchain::Abstract> blockchain,
			optional<uint32_t> coinCommunityIdIndex /* = nullopt */
		)
		{
			assert(blockchain);
			auto& alloc = mRootJson.GetAllocator();
			calculateAccountBalance::Context calculateAccountBalance(blockchain);
			auto balanceString = calculateAccountBalance.fromEnd(pubkey, date, coinCommunityIdIndex).toString();

			resultJson.AddMember("balance", Value(balanceString.data(), balanceString.size(), alloc), alloc);
		}

		void ApiHandler::getAddressType(Value& resultJson, memory::ConstBlockPtr pubkey, std::shared_ptr<gradido::blockchain::Abstract> blockchain)
		{
			assert(blockchain);
			auto& alloc = mRootJson.GetAllocator();
			auto typeString = enum_name(blockchain->getAddressType({ 0, 0, pubkey }));

			resultJson.AddMember("addressType", Value(typeString.data(), typeString.size(), alloc), alloc);
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
			MonotonicTimer timeUsed;
			auto& alloc = mRootJson.GetAllocator();

			model::Apollo::TransactionList transactionList(blockchain, filter.updatedBalancePublicKey);
			Timepoint now = std::chrono::system_clock::now();

			auto transactionListValue = transactionList.generateList(now, filter, mRootJson);

			calculateAccountBalance::Context calculateAddressBalance(blockchain);
			auto balance = calculateAddressBalance.fromEnd(filter.updatedBalancePublicKey, now, filter.coinCommunityIdIndex);
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
			MonotonicTimer timeUsed;
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
			MonotonicTimer timeUsed;
			Filter f;
			auto nameHashId = g_appContext->getUserNameHashs().getIndexForData(adapter::toByteArray<32>(nameHash));
			if (!nameHashId) {
				error(responseJson, JSON_RPC_ERROR_ADDRESS_NOT_FOUND, "user not found");
				return;
			} 
			f.transactionType = data::TransactionType::REGISTER_ADDRESS;
			// std::function<FilterResult(const TransactionEntry&)> filterFunction;
			f.filterFunction = [nameHashId](const TransactionEntry& entry) {
				auto body = entry.getTransactionBody();
				assert(body->isRegisterAddress());
				auto registerAddress = body->getRegisterAddress();
				if (nameHashId == static_cast<size_t>(registerAddress->nameHashIndex)) {
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
				const auto& dict = blockchain->getPublicKeyDictionary();

				resultJson.AddMember("pubkey", toJson(dict.getDataForIndexOrThrow(registerAddress->accountPublicKeyIndex).convertToHex(), alloc), alloc);
			}
			else {
				error(responseJson, JSON_RPC_ERROR_ADDRESS_NOT_FOUND, "user not found");
			}
		}
	}
}

