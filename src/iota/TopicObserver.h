#ifndef __GRADIDO_NODE_IOTA_TOPIC_LISTENER_H
#define __GRADIDO_NODE_IOTA_TOPIC_LISTENER_H

#include "Topic.h"
#include "IMessageObserver.h"
#include "MQTTAsync.h"

#include <list>
#include <memory>
#include <mutex>
#include <atomic>

namespace iota {
    /**
     * \brief will be called from MqttClient and forward new messages to all listener
    */
    class TopicObserver
    {
    public:         
        enum class State {
            SUBSCRIBED,
            UNSUBSCRIBED,
            PENDING
        };
        TopicObserver(const Topic& topic);

        // callbacks called from MqttClient
        //! called from MqttClient, notify all registered observers when a message arrives
        void messageArrived(MQTTAsync_message* message);        

        void subscribe(MQTTAsync& mqttClient);
        void unsubscribe(MQTTAsync& mqttClient);

        //! subscribe an observer to receive messages
        //! \return observer count after adding observer
        size_t addObserver(IMessageObserver* observer);

        //! unsubscribe an observer from receiving messages
        //! \return observer count after removing observer
        size_t removeObserver(IMessageObserver* observer);

        size_t getObserverCount();
        inline bool isPending() const { return mState == State::PENDING; }
        inline bool isSubscribed() const { return mState == State::SUBSCRIBED; }
        inline bool isUnsubscribed() const { return mState == State::UNSUBSCRIBED; }

        //! called from MqttClientWrapper on connection lost
        void setUnsubscribed();

    protected:
        std::string mTopicString;
        std::list<IMessageObserver*> mObservers;
        std::mutex mMutex;
        std::mutex mStateMutex;
        std::atomic<State> mState;
    };

}
#endif // __GRADIDO_NODE_IOTA_TOPIC_LISTENER_H