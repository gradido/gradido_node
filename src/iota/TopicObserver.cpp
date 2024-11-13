#include "TopicObserver.h"
#include "MqttClientWrapper.h"
#include "MqttExceptions.h"

#include "loguru/loguru.hpp"

#include <algorithm>

using namespace std;

namespace iota
{

    TopicObserver::TopicObserver(const Topic &topic)
        : mTopicString(topic.getTopicString()), mType(topic.getType()), mState(State::UNSUBSCRIBED)
    {
    }

    TopicObserver::~TopicObserver()
    {

    }

    ObserverReturn TopicObserver::messageArrived(MQTTAsync_message *message)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        // Notify all registered observers
        for (auto it = mObservers.begin(); it != mObservers.end(); ++it) {
            auto result = (*it)->messageArrived(message, mType);
            if(result == ObserverReturn::UNSUBSCRIBE) {
                it = mObservers.erase(it);
            }
        }
        return mObservers.size() ? ObserverReturn::CONTINUE : ObserverReturn::UNSUBSCRIBE;
    }

    void TopicObserver::subscribe(MQTTAsync &mqttClient)
    {
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            if (!isUnsubscribed())
                return;
            mState = State::PENDING;
        }

        MQTTAsync_responseOptions options = MQTTAsync_responseOptions_initializer;
        options.context = mTopicString.data();
        options.onSuccess = [](void *context, MQTTAsync_successData *response)
        {
            auto mCW = MqttClientWrapper::getInstance();
            auto topicObserver = mCW->findTopicObserver((const char *)context);
            if (!topicObserver) {
                std::string topicString((const char *)context);
                LOG_F(ERROR, "subscribed topic no longer exist: %s", topicString.data());
                return;
            }
            LOG_F(INFO, "subscribed to topic: %s", topicObserver->mTopicString.data());
            topicObserver->mState = State::SUBSCRIBED;
        };
        options.onFailure = [](void *context, MQTTAsync_failureData *response)
        {
            auto topicObserver = static_cast<TopicObserver *>(context);
            topicObserver->mState = State::UNSUBSCRIBED;
            LOG_F(ERROR, "error subscribing to topic: %s", topicObserver->mTopicString.data());
            TopicObserver::logResponseError(response);
        };
        auto rc = MQTTAsync_subscribe(mqttClient, mTopicString.data(), 1, &options);

        if (rc != MQTTASYNC_SUCCESS) {
            throw MqttSubscribeException("error subscribing", rc, mTopicString);
        }
    }

    void TopicObserver::unsubscribe(MQTTAsync &mqttClient)
    {
        {
            std::lock_guard<std::mutex> lock(mStateMutex);
            if (!isSubscribed())
                return;
            mState = State::PENDING;
        }
        MQTTAsync_responseOptions options = MQTTAsync_responseOptions_initializer;
        options.context = this;
        options.onSuccess = [](void *context, MQTTAsync_successData *response)
        {
            auto topicObserver = static_cast<TopicObserver *>(context);
            LOG_F(INFO, "unsubscribed from topic: %s", topicObserver->mTopicString.data());
            topicObserver->mState = State::UNSUBSCRIBED;
            MqttClientWrapper::getInstance()->removeTopicObserver(topicObserver->mTopicString);
        };
        options.onFailure = [](void *context, MQTTAsync_failureData *response)
        {
            auto topicObserver = static_cast<TopicObserver *>(context);
            topicObserver->mState = State::UNSUBSCRIBED;
            LOG_F(ERROR, "error unsubscribing to topic: %s", topicObserver->mTopicString.data());
            TopicObserver::logResponseError(response);
        };
        auto rc = MQTTAsync_unsubscribe(mqttClient, mTopicString.data(), &options);

        if (rc != MQTTASYNC_SUCCESS) {
            throw MqttSubscribeException("error unsubscribing", rc, mTopicString);
        }
    }

    size_t TopicObserver::addObserver(IMessageObserver *observer)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mObservers.push_back(observer);
        return mObservers.size();
    }

    size_t TopicObserver::removeObserver(IMessageObserver *observer)
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

    void TopicObserver::logResponseError(MQTTAsync_failureData* response)
    {
        if (response->message) {
            LOG_F(ERROR, "token: %d, code: %d, error: %s", response->token, response->code, response->message);
        }
        else {
            LOG_F(ERROR, "token: %d, code: %d, no error message", response->token, response->code);
        }
    }
}