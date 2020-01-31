#include "PutTransaction.h"

void PutTransaction::handle(const jsonrpcpp::Request& request, Json& response)
{
	if (request.method == "putTransaction") {
		if (request.params.has("group") && request.params.has("base64")) {

		}
		else {
			response = { {"state", "error"}, {"msg", "missing param"} };
		}
	}
	else {
		response = { {"state", "error"}, {"msg", "method not known"} };
	}
}