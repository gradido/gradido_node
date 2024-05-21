#include "Topic.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

namespace iota {
    Topic::Topic(TopicType type)
        : mTopic(type)
    {
        if(type == TopicType::MILESTONES_CONFIRMED) {
            mTopicString = "milestones/confirmed";
        } else if(type == TopicType::MILESTONES_LATEST) {
            mTopicString = "milestones/latest";
        }
        else {
            throw GradidoUnhandledEnum("need more data for this topic type", "TopicType", static_cast<int>(type));
        }
    }

    Topic::Topic(const TopicIndex& topicIndex)
        : mTopic(TopicType::MESSAGES_INDEXATION)
    {
        mTopicString = "messages/indexation/" + topicIndex.getHexString();
    }

    Topic::Topic(const MessageId& messageId)
        : mTopic(TopicType::MESSAGES_METADATA)
    {
        mTopicString = "messages/" + messageId.toHex() + "/metadata";
    }    
}