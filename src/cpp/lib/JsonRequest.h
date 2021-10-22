/*!
 *
 * \author: Dario Rekowski
 * 
 * \date: 13.12.2019
 * 
 * \brief: Class for Json Requests to php server
 *
*/

#include "ErrorList.h"
#include "Poco/Net/NameValueCollection.h"
#include "rapidjson/document.h"

#ifndef __GRADIDO_LOGIN_SERVER_LIB_JSON_REQUEST_
#define __GRADIDO_LOGIN_SERVER_LIB_JSON_REQUEST_

enum JsonRequestReturn 
{
	JSON_REQUEST_RETURN_OK,
	JSON_REQUEST_RETURN_PARSE_ERROR,
	JSON_REQUEST_RETURN_ERROR,
	JSON_REQUEST_PARAMETER_ERROR,
	JSON_REQUEST_CONNECT_ERROR
};

class JsonRequest : public ErrorList
{
public:
	//! \param urlPath path between host and method name, for iota: /api/v1/
	JsonRequest(const std::string& serverHost, int serverPort, const std::string& urlPath);
	~JsonRequest();

	rapidjson::Document GET(const char* methodName);

protected:
	int mServerPort;
	std::string mServerHost;
	std::string mUrlPath;
};


#endif //__GRADIDO_LOGIN_SERVER_LIB_JSON_REQUEST_