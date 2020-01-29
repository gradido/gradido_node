#ifndef __JSON_INTERFACE_JSON_UNKNOWN_
#define __JSON_INTERFACE_JSON_UNKNOWN_

#include "JsonRequestHandler.h"

class JsonUnknown : public JsonRequestHandler
{
public:
	Json handle(jsonrpcpp::request_ptr request) ;

protected:


};

#endif // __JSON_INTERFACE_JSON_UNKNOWN_