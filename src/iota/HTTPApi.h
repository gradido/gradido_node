#ifndef GRADIDO_NODE_IOTA_HTTP_API
#define GRADIDO_NODE_IOTA_HTTP_API

#include "MessageId.h"

#include "rapidjson/document.h"

#include <vector>

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
    rapidjson::Document getMessageJson(MessageId messageId);
	std::pair<std::string, std::string> getIndexiationMessageDataIndex(MessageId messageId);
	//! use metadata call to check if it is referenced by a milestone
		//! \return return 0 if not, else return milestone id
	uint32_t getMessageMilestoneId(MessageId messageId);
    std::vector<MessageId> findByIndex(std::string index);
    MessageId getMilestoneByIndex(int32_t milestoneIndex);
	uint64_t getMilestoneTimestamp(int32_t milestoneIndex);
    NodeInfo getNodeInfo();
}

#endif //GRADIDO_NODE_IOTA_HTTP_API