#ifndef __GRADIDO_NODE_CLIENT_JSON_H
#define __GRADIDO_NODE_CLIENT_JSON_H

#include "Base.h"

namespace client
{
	class JsonRPC : public Base
	{
	public: 
		JsonRPC(const std::string& uri, bool base64 = true);
		bool postRequest(const std::map<std::string, std::string>& parameterValuePairs);
	};
}

#endif //__GRADIDO_NODE_CLIENT_JSON_H