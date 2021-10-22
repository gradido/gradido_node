#include "HTTPApi.h"
#include "../ServerGlobals.h"

namespace iota {
    // api endpoints: 
    // https://docs.rs/iota-client/1.0.1/iota_client/client/struct.Client.html

    NodeInfo getNodeInfo()
    {
        auto json = ServerGlobals::g_IotaRequestHandler.GET("info");

    }

    MessageId getMilestoneByIndex(int32_t milestoneIndex)
    {
        // api/v1/milestones/909039
    }
}