#include "PutTransaction.h"

Poco::JSON::Object* PutTransaction::handle(Poco::Dynamic::Var params)
{
	Poco::JSON::Object* result = new Poco::JSON::Object;

	result->set("state", "error");
	result->set("msg", "unknown call");



	return result;
}