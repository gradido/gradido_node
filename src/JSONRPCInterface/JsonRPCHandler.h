#ifndef __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
#define __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_

#include "JsonRPCRequestHandler.h"
#include "gradido_blockchain/MemoryManager.h"
#include "Poco/DateTime.h"

#include "../controller/Group.h"

// TODO: write api doc and help on command
class JsonRPCHandler : public JsonRPCRequestHandler
{
public:
	void handle(std::string method, const rapidjson::Value& params);

protected:
	void getTransactions(int64_t fromTransactionId, const std::string& groupAlias, const std::string& format);
	void getCreationSumForMonth(const std::string& pubkey, int month, int year, Poco::DateTime searchStartDate, Poco::SharedPtr<controller::Group> group);
	void listTransactions(
		const std::string& groupAlias, 
		const std::string& publicKeyHex, 
		int currentPage = 1, 
		int pageSize = 25, 
		bool orderDESC = true, 
		bool onlyCreations = false
	);
	
	void getAddressBalance(const std::string& pubkey, Poco::DateTime date, Poco::SharedPtr<controller::Group> group, const std::string& coinGroupId = "");
	void getAddressType(const std::string& pubkey, Poco::SharedPtr<controller::Group> group);
	void getAddressTxids(const std::string& pubkey, Poco::SharedPtr<controller::Group> group);

	// helper
	

};

#endif // __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
