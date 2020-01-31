#include "JsonRequestHandlerFactory.h"

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTime.h"

#include "JsonUnknown.h"
#include "PutTransaction.h"
#include "JsonRPCHandler.h"

#include <sstream>

JsonRequestHandlerFactory::JsonRequestHandlerFactory()	
	: mRemoveGETParameters("^/([a-zA-Z0-9_-]*)"), mLogging(Poco::Logger::get("requestLog"))
{
}

Poco::Net::HTTPRequestHandler* JsonRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
	//std::string uri = request.getURI();
	//std::string url_first_part;
	//std::stringstream logStream;

	//mRemoveGETParameters.extract(uri, url_first_part);

	//std::string dateTimeString = Poco::DateTimeFormatter::format(Poco::DateTime(), "%d.%m.%y %H:%M:%S");
	//logStream << dateTimeString << " call " << uri;

	//mLogging.information(logStream.str());

	return new JsonRPCHandler;

	

	//return new JsonUnknown;
}
