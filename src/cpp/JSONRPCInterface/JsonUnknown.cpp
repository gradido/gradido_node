#include "JsonUnknown.h"

void JsonUnknown::handle(const jsonrpcpp::Request& request, Json& response)
{
	response["state"] = "error";
	response["msg"] = "unknown call";
	
}