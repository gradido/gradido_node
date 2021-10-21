#ifndef __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
#define __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_

#include "JsonRequestHandler.h"
#include "../SingletonManager/MemoryManager.h"

class JsonRPCHandler : public JsonRequestHandler
{
public:
	rapidjson::Document handle(std::string method, const rapidjson::Value& params);

protected:
	rapidjson::Document putTransaction(const std::string& transactionBinary, const std::string& groupAlias);


};

#endif // __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
