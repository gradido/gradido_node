#include "Topic.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

#include "magic_enum/magic_enum.hpp"

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
            throw GradidoUnhandledEnum("need more data for this topic type", "TopicType", magic_enum::enum_name(type).data());
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