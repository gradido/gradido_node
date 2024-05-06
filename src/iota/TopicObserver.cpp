#include "TopicObserver.h"
#include "MqttClientWrapper.h"
#include "MqttExceptions.h"

#include <algorithm>


using namespace std;

namespace iota {
    
    TopicObserver::TopicObserver(const Topic& topic)
      : mTopicString(topic.getTopicString())
    {

    }
    
    void TopicObserver::messageArrived(MQTTAsync_message* message)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        // Notify all registered observers
        for (const auto& observer : mObservers) {
            observer->messageArrived(message);
        }
    }

    void TopicObserver::subscribe(MQTTAsync& mqttClient)
    {
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            if(!isUnsubscribed()) return;
            mState = State::PENDING;
        }
        
        MQTTAsync_responseOptions options = MQTTAsync_responseOptions_initializer;
        options.context = this;
        options.onSuccess = [](void* context, MQTTAsync_successData* response) {
            auto topicObserver = static_cast<TopicObserver*>(context);
            auto& logger = MqttClientWrapper::getInstance()->getLogger();
            logger.information("successfully subscribed to topic: %s", topicObserver->mTopicString);
            topicObserver->mState = State::SUBSCRIBED;
        };
        options.onFailure = [](void* context, MQTTAsync_failureData* response) {
            auto topicObserver = static_cast<TopicObserver*>(context);
            auto& logger = MqttClientWrapper::getInstance()->getLogger();
            topicObserver->mState = State::UNSUBSCRIBED;
            logger.error("error subscribing to topic: %s", topicObserver->mTopicString);
            delete[] context;
            if (response->message) {
			    logger.error(
                    "token: %d, code: %d, error: %s", 
                    response->token, response->code, std::string(response->message)
                );
		    }
		    else {
			    logger.error(
                    "token: %d, code: %d, no error message", 
                    response->token, response->code
                );
		    }
        };
        auto rc = MQTTAsync_subscribe(mqttClient, mTopicString.data(), 1, &options);

        if (rc != MQTTASYNC_SUCCESS) {
            throw MqttSubscribeException("error subscribing", rc, mTopicString);
        }
				
    }

    void TopicObserver::unsubscribe(MQTTAsync& mqttClient)
    {
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            if(!isSubscribed()) return;
            mState = State::PENDING;
        }
        MQTTAsync_responseOptions options = MQTTAsync_responseOptions_initializer;
        options.context = this;
        options.onSuccess = [](void* context, MQTTAsync_successData* response) {
            auto topicObserver = static_cast<TopicObserver*>(context);
            auto& logger = MqttClientWrapper::getInstance()->getLogger();
            logger.information("successfully unsubscribed from topic: %s", topicObserver->mTopicString);
            topicObserver->mState = State::UNSUBSCRIBED;
            MqttClientWrapper::getInstance()->removeTopicObserver(topicObserver->mTopicString);
        };
        options.onFailure = [](void* context, MQTTAsync_failureData* response) {
            auto topicObserver = static_cast<TopicObserver*>(context);
            auto& logger = MqttClientWrapper::getInstance()->getLogger();
            topicObserver->mState = State::UNSUBSCRIBED;
            logger.error("error subscribing to topic: %s", topicObserver->mTopicString);
            if (response->message) {
			    logger.error(
                    "token: %d, code: %d, error: %s", 
                    response->token, response->code, std::string(response->message)
                );
		    }
		    else {
			    logger.error(
                    "token: %d, code: %d, no error message", 
                    response->token, response->code
                );
		    }
        };
        auto rc = MQTTAsync_unsubscribe(mqttClient, mTopicString.data(), &options);

        if (rc != MQTTASYNC_SUCCESS) {
            throw MqttSubscribeException("error unsubscribing", rc, mTopicString);
        }
    }

         
    size_t TopicObserver::addObserver(IMessageObserver* observer)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mObservers.push_back(observer);
        return mObservers.size();
    }

    size_t TopicObserver::removeObserver(IMessageObserver* observer)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = std::find(mObservers.begin(), mObservers.end(), observer);
        if (it != mObservers.end()) {
            mObservers.erase(it);
        }
        return mObservers.size();
    }

    size_t TopicObserver::getObserverCount()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mObservers.size();
    }

    void TopicObserver::setUnsubscribed()
    {
        mState = State::UNSUBSCRIBED;
    }
}