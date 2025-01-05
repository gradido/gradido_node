#ifndef __GRADIDO_NODE_IOTA_TOPIC_H
#define __GRADIDO_NODE_IOTA_TOPIC_H

#include "gradido_blockchain/data/iota/MessageId.h"
#include "gradido_blockchain/data/iota/TopicIndex.h"

namespace iota {
    enum class TopicType: uint8_t {
        MESSAGES_INDEXATION = 0, // topicMessagesIndexation = "messages/indexation/{index}"
        MILESTONES_CONFIRMED = 1, // topicMilestonesConfirmed = "milestones/confirmed"
        MILESTONES_LATEST = 2, // topicMilestonesLatest = "milestones/latest"
        MESSAGES_METADATA = 3 // topicMessagesMetadata = "messages/{messageId}/metadata"
    };

    class Topic 
    {
    public:
        Topic(TopicType type);
        Topic(const TopicIndex& topicIndex);
        Topic(const MessageId& messageId);    
           
        const std::string& getTopicString() const { return mTopicString; }
        TopicType getType() const { return mTopic; }
    protected:
        std::string mTopicString;
        TopicType mTopic;
    };
}

#endif //__GRADIDO_NODE_IOTA_TOPIC_H