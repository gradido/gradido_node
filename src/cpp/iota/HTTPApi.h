#ifndef GRADIDO_NODE_IOTA_HTTP_API
#define GRADIDO_NODE_IOTA_HTTP_API

#include "IotaWrapper.h"

namespace iota {
    Milestone getMilestone(MessageId milestoneMessageId);
    MessageId getMilestoneByIndex(int32_t milestoneIndex);
    NodeInfo getNodeInfo();
}

#endif //GRADIDO_NODE_IOTA_HTTP_API