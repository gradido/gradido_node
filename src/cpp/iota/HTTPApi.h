#ifndef GRADIDO_NODE_IOTA_HTTP_API
#define GRADIDO_NODE_IOTA_HTTP_API

#include "IotaWrapper.h"

namespace iotaHttp {
    iota::Milestone getMilestone(iota::MessageId milestoneMessageId);
    iota::MessageId getMilestoneByIndex(int32_t milestoneIndex);
    iota::NodeInfo getNodeInfo();
}

#endif //GRADIDO_NODE_IOTA_HTTP_API