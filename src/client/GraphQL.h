#ifndef __GRADIDO_NODE_CLIENT_GRAPHQL_H
#define __GRADIDO_NODE_CLIENT_GRAPHQL_H

#include "Base.h"

namespace client {
	class GraphQL : public Base
	{
	public:
		GraphQL(const Poco::URI& uri);
		~GraphQL();

		bool postRequest(const Poco::Net::NameValueCollection& parameterValuePairs);
	protected:

	};
}

#endif //__GRADIDO_NODE_CLIENT_GRAPHQL_H