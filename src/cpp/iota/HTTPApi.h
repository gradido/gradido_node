#ifndef GRADIDO_NODE_IOTA_HTTP_API
#define GRADIDO_NODE_IOTA_HTTP_API

#include "Message.h"
#include "rapidjson/document.h"

namespace iota {
	struct NodeInfo
	{
		std::string version;
		bool isHealthy;
		float messagesPerSecond;
		int32_t lastMilestoneIndex;
		int64_t lastMilestoneTimestamp;
		int32_t confirmedMilestoneIndex;
	};

    //iota::Milestone getMilestone(iota::MessageId milestoneMessageId);
    //std::string     getIndexiation(iota::MessageId indexiationMessageId);
    rapidjson::Document getMessageJson(iota::MessageId messageId);
    std::vector<MessageId> findByIndex(std::string index);
    MessageId getMilestoneByIndex(int32_t milestoneIndex);
    NodeInfo getNodeInfo();
}

#endif //GRADIDO_NODE_IOTA_HTTP_API