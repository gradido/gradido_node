#ifndef __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
#define __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_

#include "JsonRPCRequestHandler.h"
#include "Poco/DateTime.h"

#include "../controller/Group.h"

// TODO: write api doc and help on command
class JsonRPCHandler : public JsonRPCRequestHandler
{
public:
	void handle(rapidjson::Value& responseJson, std::string method, const rapidjson::Value& params);

protected:
	/*
	*
	curl -X POST -H "Content-Type: application/json" -d '{"jsonrpc": "2.0", "method": "getTransactions", "params": {"groupAlias": "e78c6a06f4efdde1a93d061fe2d89179e093d901a02379a8a2319822f4f3ce71","fromTransactionId": "1","format": "json"},"id":1 }' http://localhost:8340

	*/
	void getTransactions(
		rapidjson::Value& resultJson, 
		uint64_t fromTransactionId,
		uint32_t maxResultCount,
		std::shared_ptr<controller::Group> group, 
		const std::string& format
	);
	/*!
	* TODO: implement index for iota message id if it is used much
	* @param transactionId: this parameter or
	* @param iotaMessageId: this parameter for finding transaction
	*/
	void getTransaction(
		rapidjson::Value& resultJson,
		std::shared_ptr<controller::Group> group,
		const std::string& format,
		uint64_t transactionId = 0,
		MemoryBin* iotaMessageId = nullptr
	);
	//! \param searchStartDate start date for reverse search for creation transactions range -2 month from there
	void getCreationSumForMonth(
		rapidjson::Value& resultJson, 
		const std::string& pubkey, 
		int month, 
		int year, 
		Poco::DateTime searchStartDate, 
		std::shared_ptr<controller::Group> group
	);
	void listTransactions(
		rapidjson::Value& resultJson,
		std::shared_ptr<controller::Group> group,
		const std::string& publicKeyHex, 
		int currentPage = 1, 
		int pageSize = 25, 
		bool orderDESC = true, 
		bool onlyCreations = false
	);
	void listTransactionsForAddress(
		rapidjson::Value& resultJson,
		const std::string& userPublicKey,
		uint64_t firstTransactionNr,
		uint32_t maxResultCount,
		std::shared_ptr<controller::Group> group
	);
	
	void getAddressBalance(
		rapidjson::Value& resultJson,
		const std::string& pubkey,
		Poco::DateTime date, 
		std::shared_ptr<controller::Group> group,
		const std::string& coinGroupId = ""
	);
	void getAddressType(rapidjson::Value& resultJson, const std::string& pubkey, std::shared_ptr<controller::Group> group);
	void getAddressTxids(rapidjson::Value& resultJson, const std::string& pubkey, std::shared_ptr<controller::Group> group);
	void putTransaction(
		rapidjson::Value& resultJson,
		uint64_t transactionNr,
		std::unique_ptr<gradido::data::GradidoTransaction> transaction,
		std::shared_ptr<controller::Group> group
	);

	// helper
	

};

#endif // __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
