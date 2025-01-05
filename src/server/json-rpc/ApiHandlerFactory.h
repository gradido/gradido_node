#ifndef __GRADIDO_NODE_SERVER_JSON_RPC_API_HANDLER_FACTORY_H
#define __GRADIDO_NODE_SERVER_JSON_RPC_API_HANDLER_FACTORY_H

#include "gradido_blockchain/http/AbstractResponseHandlerFactory.h"
#include "ApiHandler.h"

namespace server {
	namespace json_rpc {
		class ApiHandlerFactory : public AbstractResponseHandlerFactory
		{
		public:
			virtual std::unique_ptr<AbstractResponseHandler> getResponseHandler(MethodType method) {
				return std::make_unique<ApiHandler>();
			};
			//! \return check if factory has a response handler for a specific method, else server didn't need to listen for requests
			virtual bool has(MethodType method) {
				switch (method) {
				case MethodType::GET:
				case MethodType::POST:
				case MethodType::OPTIONS: return true;
				}
				return false;
			};
		};
	}
}

#endif //__GRADIDO_NODE_SERVER_JSON_RPC_API_HANDLER_FACTORY_H