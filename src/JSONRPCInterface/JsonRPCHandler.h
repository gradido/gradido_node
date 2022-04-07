#ifndef __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
#define __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_

#include "JsonRPCRequestHandler.h"
#include "gradido_blockchain/MemoryManager.h"

class JsonRPCHandler : public JsonRPCRequestHandler
{
public:
	void handle(std::string method, const rapidjson::Value& params);

protected:
	void putTransaction(const std::string& transactionBinary, const std::string& groupAlias);
	void getGroupDetails(const std::string& groupAlias);
	void getRandomUniqueCoinColor();
	void isGroupUnique(const std::string& groupAlias, uint32_t coinColor);
	void getTransactions(int64_t fromTransactionId, const std::string& groupAlias, const std::string& format);
	void listTransactions(
		const std::string& groupAlias, 
		const std::string& publicKeyHex, 
		int currentPage = 1, 
		int pageSize = 25, 
		bool orderDESC = true, 
		bool onlyCreations = false
	);

	// helper
	

};

#endif // __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
