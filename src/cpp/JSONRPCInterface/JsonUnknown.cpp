#include "JsonUnknown.h"

Json JsonUnknown::handle(jsonrpcpp::request_ptr request)
{
	jsonrpcpp::response_ptr response(new jsonrpcpp::Response(*request, result));

	Json result;
	result["state"] = "error";
	result["msg"] = "unknown call";
	
	return result;
}