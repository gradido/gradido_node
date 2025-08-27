#ifndef __GRADIDO_NODE_HIERO_I_MESSAGE_OBSERVER_H
#define __GRADIDO_NODE_HIERO_I_MESSAGE_OBSERVER_H

#include "gradido_blockchain/memory/Block.h"

namespace client {
    namespace grpc {

        //! Base Class for derivation to receive messages from grpc-server
        class IMessageObserver
        {
        public:
            // move message binary
            virtual void messageArrived(memory::Block&& message) = 0;

            // will be called from grpc client if connection was closed
            virtual void messagesStopped() = 0;
        protected:
        };
    }
}

#endif //__GRADIDO_NODE_HIERO_I_MESSAGE_OBSERVER_H