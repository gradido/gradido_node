#ifndef __GRADIDO_NODE_CLIENT_JSON_H
#define __GRADIDO_NODE_CLIENT_JSON_H

#include "Base.h"

namespace client
{
	class JsonRPC : public Base
	{
	public: 
		JsonRPC(const std::string& successUrl, const std::string& failedUrl, bool base64 = true);
		bool postRequest(const std::map<std::string, std::string>& parameterValuePairs, const std::string& url);
	};
}

#endif //__GRADIDO_NODE_CLIENT_JSON_H