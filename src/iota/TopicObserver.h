#ifndef __GRADIDO_NODE_IOTA_TOPIC_LISTENER_H
#define __GRADIDO_NODE_IOTA_TOPIC_LISTENER_H

#include "Topic.h"
#include "MQTTAsync.h"
#include <list>
#include <memory>
#include <mutex>


namespace iota {
    class IMessageObserver;
    /**
     * \brief will be called from MqttClient and forward new messages to all listener
    */
    class TopicObserver
    {
    public:         
        TopicObserver(const Topic& topic);

        //! called from MqttClient, notify all registered observers when a message arrives
        void messageArrived(MQTTAsync_message* message);

         //! subscribe an observer to receive messages
        void addObserver(std::shared_ptr<IMessageObserver> observer);

        //! unsubscribe an observer from receiving messages
        void removeObserver(std::shared_ptr<IMessageObserver> observer);

    protected:
        std::string mTopicString;
        std::list<std::shared_ptr<IMessageObserver>> mObservers;
        std::mutex mMutex;
    };

}
#endif // __GRADIDO_NODE_IOTA_TOPIC_LISTENER_H