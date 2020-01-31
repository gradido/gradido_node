#ifndef __JSON_RPC_INTERFACE_JSON_PUT_TRANSACTION_
#define __JSON_RPC_INTERFACE_JSON_PUT_TRANSACTION_

#include "JsonRequestHandler.h"

class PutTransaction : public JsonRequestHandler
{
public:
	void handle(const jsonrpcpp::Request& request, Json& response);

protected:


};

#endif // __JSON_RPC_INTERFACE_JSON_PUT_TRANSACTION_