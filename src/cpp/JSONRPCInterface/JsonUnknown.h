#ifndef __JSON_INTERFACE_JSON_UNKNOWN_
#define __JSON_INTERFACE_JSON_UNKNOWN_

#include "JsonRequestHandler.h"

class JsonUnknown : public JsonRequestHandler
{
public:
	void handle(const jsonrpcpp::Request& request, Json& response);

protected:


};

#endif // __JSON_INTERFACE_JSON_UNKNOWN_