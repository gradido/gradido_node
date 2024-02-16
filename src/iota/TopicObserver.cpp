#include "TopicObserver.h"
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

         
    void TopicObserver::addObserver(std::shared_ptr<IMessageObserver> observer)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mObservers.push_back(observer);
    }

    void TopicObserver::removeObserver(std::shared_ptr<IMessageObserver> observer)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = std::find(mObservers.begin(), mObservers.end(), observer);
        if (it != mObservers.end()) {
            mObservers.erase(it);
        }
    }
}