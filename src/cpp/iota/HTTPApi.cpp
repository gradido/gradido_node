#include "HTTPApi.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/LoggerManager.h"
#include "../lib/BinTextConverter.h"
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
            throw std::runtime_error("iota info result changed!");
        }
        return result;
    }

    iota::MessageId getMilestoneByIndex(int32_t milestoneIndex)
    {
        iota::MessageId result;
        // api/v1/milestones/909039
        std::stringstream ss;
        ss << "milestones/" << std::to_string(milestoneIndex);
        auto json = ServerGlobals::g_IotaRequestHandler->GET(ss.str().data());
        if (!json.IsObject()) return result;
        try {
            Value& data = json["data"];
            std::string messageIdHex = data["messageId"].GetString();
            result.fromHex(messageIdHex);
        }
        catch (std::exception& e) {
            throw std::runtime_error("iota milestones result changed!");
        }
        return result;
    }

    iota::Milestone getMilestone(iota::MessageId milestoneMessageId)
    {
        // GET /api/v1/messages/{messageId} 
		std::stringstream ss;
        ss << "messages/" << milestoneMessageId.toHex();

        iota::Milestone result;

		auto json = ServerGlobals::g_IotaRequestHandler->GET(ss.str().data());
		if (!json.IsObject()) return result;
		try {
			Value& data = json["data"];
            Value& payload = data["payload"];
            int type = payload["type"].GetInt();
            if (type != 1) {
                throw std::runtime_error("message isn't milestone message!");
            }
            result.id = payload["index"].GetInt();
            result.timestamp = payload["timestamp"].GetInt64();
            auto parentMessageIds = data["parentMessageIds"].GetArray();
            result.messageIds.reserve(parentMessageIds.Size());
            for (auto it = parentMessageIds.Begin(); it != parentMessageIds.End(); ++it) {
                iota::MessageId messageId;
                messageId.fromHex(it->GetString());
                result.messageIds.push_back(messageId);
            }
		}
		catch (std::exception& e) {
			throw new std::runtime_error("iota milestone message result changed!");
		}

        return result;
    }

    std::string getIndexiation(iota::MessageId indexiationMessageId)
    {
		// GET /api/v1/messages/{messageId} 
		std::stringstream ss;
		ss << "messages/" << indexiationMessageId.toHex();

		std::string result;

		auto json = ServerGlobals::g_IotaRequestHandler->GET(ss.str().data());
		if (!json.IsObject()) return result;
		try {
			Value& data = json["data"];
			Value& payload = data["payload"];
			int type = payload["type"].GetInt();
			if (type != 2) {
				throw std::runtime_error("message isn't indexiation message!");
			}
            result = convertHexToBin(payload["data"].GetString());
		}
		catch (std::exception& e) {
			throw new std::runtime_error("iota milestone message result changed!");
		}

		return result;
    }
}