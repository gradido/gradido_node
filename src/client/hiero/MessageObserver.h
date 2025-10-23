#ifndef __GRADIDO_NODE_CLIENT_HIERO_MESSAGE_OBSERVER_H
#define __GRADIDO_NODE_CLIENT_HIERO_MESSAGE_OBSERVER_H

#include "gradido_blockchain/memory/Block.h"
#include <grpcpp/client_context.h>

namespace client {
    namespace hiero {

        //! Base Class for derivation to receive messages from grpc-server
        template<class Message>
        class MessageObserver
        {
        public:
            virtual ~MessageObserver() = default;
            
            // move message binary
            virtual void onMessageArrived(const Message& obj) = 0;

            // will be called from grpc client if connection was closed
            // so no more messageArrived calls
            virtual void onConnectionClosed() = 0;
            ::grpc::ClientContext* getClientContextPtr() { return &mClientContext; }

        protected:
            // can be used to cancel call before it is complete
            ::grpc::ClientContext mClientContext;

        };
    }
}

#endif //__GRADIDO_NODE_CLIENT_HIERO_MESSAGE_OBSERVER_H