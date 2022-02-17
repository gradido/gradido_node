#include "HTTPApi.h"
#include "../ServerGlobals.h"
#include "../SingletonManager/LoggerManager.h"
#include "../lib/BinTextConverter.h"
#include <stdexcept>

#include "rapidjson/pointer.h"

using namespace rapidjson;

namespace iota {
    // api endpoints: 
    // https://docs.rs/iota-client/1.0.1/iota_client/client/struct.Client.html

    NodeInfo getNodeInfo()
    {
        iota::NodeInfo result;
        auto json = ServerGlobals::g_IotaRequestHandler->GET("info");
        if (!json.IsObject()) return result;
        try {
            Value& data = json["data"];
            result.version = data["version"].GetString(); // testet with 1.0.5
            result.isHealthy = data["isHealthy"].GetBool();
            result.messagesPerSecond = data["messagesPerSecond"].GetFloat();
            result.confirmedMilestoneIndex = data["confirmedMilestoneIndex"].GetUint();
            result.lastMilestoneIndex = data["latestMilestoneIndex"].GetInt();
            result.lastMilestoneTimestamp = data["latestMilestoneTimestamp"].GetInt64();
        }
        catch (std::exception& e) {
            throw std::runtime_error("iota info result changed!");
        }
        return result;
    }

    MessageId getMilestoneByIndex(int32_t milestoneIndex)
    {
        MessageId result;
        // api/v1/milestones/909039
        std::stringstream ss;
        ss << "milestones/" << std::to_string(milestoneIndex);
        auto json = ServerGlobals::g_IotaRequestHandler->GET(ss.str().data());
        if (!json.IsObject() || json.FindMember("data") == json.MemberEnd()) return result;
        try {
            Value& data = json["data"];
            std::string messageIdHex = data["messageId"].GetString();
            //printf("hex before: %s, length: %d\n", messageIdHex.data(), messageIdHex.size());
            result.fromHex(messageIdHex);
            //std::string hexAfter = result.toHex();
            //printf("hex after: %s, length: %d\n", hexAfter.data(), hexAfter.size());
            //printf("hex compare: %d %d\n", messageIdHex == hexAfter, result == result);

        }
        catch (std::exception& e) {
            throw std::runtime_error("iota milestones result changed!");
        }
        return result;
    }

    uint64_t getMilestoneTimestamp(int32_t milestoneIndex)
    {
		// api/v1/milestones/909039
		std::stringstream ss;
		ss << "milestones/" << std::to_string(milestoneIndex);
		auto json = ServerGlobals::g_IotaRequestHandler->GET(ss.str().data());
		if (!json.IsObject() || json.FindMember("data") == json.MemberEnd()) return 0;
	
		Value& data = json["data"];
        if (!data.HasMember("timestamp")) {
            return 0;
        }
		return data["timestamp"].GetUint64();
    }

    Document getMessageJson(MessageId messageId)
    {
		// GET /api/v1/messages/{messageId} 
		std::stringstream ss;
		ss << "messages/" << messageId.toHex();

		return ServerGlobals::g_IotaRequestHandler->GET(ss.str().data());
    }

    std::pair<std::string, std::string> getIndexiationMessageDataIndex(MessageId messageId)
    {
        auto json = getMessageJson(messageId);
        if (!json.IsObject() || json.FindMember("data") == json.MemberEnd()) return { "", "" };
		auto payload = Pointer("/data/payload").Get(json);
		if (!payload->HasMember("data") || !payload->HasMember("index")) {
            return { "", "" };
		}
        return { (*payload)["data"].GetString(), convertHexToBin((*payload)["index"].GetString()) };
    }

    uint32_t getMessageMilestoneId(iota::MessageId messageId)
    {
		// GET /api/v1/messages/{messageId}/metadata
		std::stringstream ss;
		ss << "messages/" << messageId.toHex() << "/metadata";
        auto json = ServerGlobals::g_IotaRequestHandler->GET(ss.str().data());

        if (!json.IsObject() || json.FindMember("data") == json.MemberEnd()) return 0;
        Value& data = json["data"];
        auto referencedByMilestoneIt = data.FindMember("referencedByMilestoneIndex");
        if (referencedByMilestoneIt != data.MemberEnd()) {
            return referencedByMilestoneIt->value.GetUint();
        }
        return 0;
		
    }

    std::vector<MessageId> findByIndex(std::string index)
    {
        // GET /api/v1/messages?index=4752414449444f2e7079746861676f726173
        std::stringstream ss;
        ss << "messages?index=" << convertBinToHex(index);

        std::vector<MessageId> result;

        auto json = ServerGlobals::g_IotaRequestHandler->GET(ss.str().data());
        if (!json.IsObject() || !json.HasMember("data")) return result;
        
        Value& data = json["data"];
        if(!data.HasMember("messageIds") || !data["messageIds"].IsArray()) return result;
        auto messageIds = data["messageIds"].GetArray();
		for (auto it = messageIds.Begin(); it != messageIds.End(); ++it) {
			MessageId messageId;
			messageId.fromHex(it->GetString());
			result.push_back(messageId);
		}
        
        return result;
    }
}