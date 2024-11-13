#ifndef __GRADIDO_NODE_IOTA_I_MESSAGE_OBSERVER_H
#define __GRADIDO_NODE_IOTA_I_MESSAGE_OBSERVER_H

#include "MQTTAsync.h"
#include "Topic.h"

namespace iota {
     enum class ObserverReturn {
        CONTINUE = 0, // do nothing
        UNSUBSCRIBE = 1, // remove from observer list
    };

    //! Base Class for derivation to receive mqtt messages from the topic
    class IMessageObserver
    {
    public:
        virtual ObserverReturn messageArrived(MQTTAsync_message* message, TopicType type) = 0;
    protected:
    };
}

#endif //__GRADIDO_NODE_IOTA_I_MESSAGE_OBSERVER_H