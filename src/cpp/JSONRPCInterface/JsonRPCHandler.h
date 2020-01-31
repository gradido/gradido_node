#ifndef __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_
#define __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_

#include "JsonRequestHandler.h"
#include "../SingletonManager/MemoryManager.h"

class JsonRPCHandler : public JsonRequestHandler
{
public:
	void handle(const jsonrpcpp::Request& request, Json& response);

protected:
	void putTransaction(MemoryBin* transactionBinary, MemoryBin* groupPublicBinary, Json& response);


};

#endif // __JSON_RPC_INTERFACE_JSON_JSON_RPC_HANDLER_