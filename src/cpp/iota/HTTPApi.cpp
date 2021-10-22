#include "HTTPApi.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/LoggerManager.h"
#include <stdexcept>

using namespace rapidjson;

namespace iotaHttp {
    // api endpoints: 
    // https://docs.rs/iota-client/1.0.1/iota_client/client/struct.Client.html

    iota::NodeInfo getNodeInfo()
    {
        iota::NodeInfo result;
        auto json = ServerGlobals::g_IotaRequestHandler->GET("info");
        if (!json.IsObject()) return result;
        try {
            Value& data = json["data"];
            result.version = data["version"].GetString(); // testet with 1.0.5
            result.isHealthy = data["isHealthy"].GetBool();
            result.messagesPerSecond = data["messagesPerSecond"].GetFloat();
            result.confirmedMilestoneIndex = data["confirmedMilestoneIndex"].GetInt();
            result.lastMilestoneIndex = data["latestMilestoneIndex"].GetInt();
            result.lastMilestoneTimestamp = data["latestMilestoneTimestamp"].GetInt64();
        }
        catch (std::exception& e) {
            throw new std::runtime_error("iota info result changed!");
        }
        return result;
    }

    iota::MessageId getMilestoneByIndex(int32_t milestoneIndex)
    {
        iota::MessageId result;
        // api/v1/milestones/909039
        return result;
    }
}