#include "Topic.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

namespace iota {
    Topic::Topic(TopicType type)
    {
        if(type == TopicType::MILESTONES_CONFIRMED) {
            mTopicString = "milestones/confirmed";
        }
        throw GradidoUnhandledEnum("need more data for this topic type", "TopicType", static_cast<int>(type));
    }

    Topic::Topic(const TopicIndex& topicIndex)
    {
        mTopicString = "messages/indexation/" + topicIndex.getTopicIndexHex();
    }

    Topic::Topic(const MessageId& messageId)
    {
        mTopicString = "messages/" + messageId.toHex() + "/metadata";
    }    
}