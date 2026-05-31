#ifndef __GRADIDO_NODE_CLIENT_GRAPHQL_H
#define __GRADIDO_NODE_CLIENT_GRAPHQL_H

#include "Base.h"

namespace client {
	class GraphQL : public Base
	{
	public:
		GraphQL(const std::string& successUrl, const std::string& failedUrl);
		~GraphQL();

		bool postRequest(const std::map<std::string, std::string>& parameterValuePairs, const std::string& url);
	protected:

	};
}

#endif //__GRADIDO_NODE_CLIENT_GRAPHQL_H