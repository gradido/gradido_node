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
	void getTransactions(int64_t fromTransactionId, const std::string& groupAlias, const std::string& format);

};

#endif // __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
