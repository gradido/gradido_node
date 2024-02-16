#ifndef __GRADIDO_NODE_IOTA_TOPIC_H
#define __GRADIDO_NODE_IOTA_TOPIC_H

#include "TopicIndex.h"
#include "MessageId.h"

namespace iota {

    enum class TopicType: uint8_t {
        MESSAGES_INDEXATION = 0, // topicMessagesIndexation = "messages/indexation/{index}"
        MILESTONES_CONFIRMED = 1, // topicMilestonesConfirmed = "milestones/confirmed"
        MESSAGES_METADATA = 2 // topicMessagesMetadata = "messages/{messageId}/metadata"
    };

    class Topic 
    {
    public:
        Topic(TopicType type);
        Topic(const TopicIndex& topicIndex);
        Topic(const MessageId& messageId);    
           
        const std::string& getTopicString() const { return mTopicString; }
    protected:
        std::string mTopicString;
    };
}

#endif //__GRADIDO_NODE_IOTA_TOPIC_H