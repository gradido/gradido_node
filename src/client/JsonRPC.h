#ifndef __GRADIDO_NODE_CLIENT_JSON_H
#define __GRADIDO_NODE_CLIENT_JSON_H

#include "Base.h"

namespace client
{
	class JsonRPC : public Base
	{
	public: 
		JsonRPC(const Poco::URI& uri, bool base64 = true);
		bool postRequest(const Poco::Net::NameValueCollection& parameterValuePairs);
	};
}

#endif //__GRADIDO_NODE_CLIENT_JSON_H