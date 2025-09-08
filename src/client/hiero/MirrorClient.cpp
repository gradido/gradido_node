#include "MirrorClient.h"

#include "gradido_blockchain/http/JsonRequest.h"
#include "gradido_blockchain/lib/RapidjsonHelper.h"

#include "loguru/loguru.hpp"

using namespace rapidjson;
using namespace rapidjson_helper;

namespace client {
    namespace hiero {
        MirrorClient::MirrorClient(std::string_view networkType)
            : mNetworkType(networkType)
        {
            
        }
        MirrorClient::~MirrorClient() 
        {

        }

        std::vector<::hiero::ConsensusTopicResponse> MirrorClient::listTopicMessagesById(
            const ::hiero::TopicId& topicId,
            uint32_t limit /* = 25*/,
            uint64_t sequenceNumber /* = 0*/ ,
            const std::string& order /* = "asc" */
        ) {
            std::vector<::hiero::ConsensusTopicResponse> results;
            // std::string url = getBasePath();
            // url += "topics/" + topicId.toString();
            JsonRequest request(getBasePath());
            std::string requestPath = "topics/" + topicId.toString();
            requestPath += "?limit=" + std::to_string(limit);
            requestPath += "&order=" + order;
            requestPath += "&sequencenumber" + std::to_string(sequenceNumber);
            auto resultJson = request.getRequest(requestPath.data());
            auto messages = resultJson["messages"].GetArray();
            results.reserve(messages.Size());
            for (Value::ConstValueIterator itr = messages.Begin(); itr != messages.End(); ++itr) {
                results.push_back(::hiero::ConsensusTopicResponse(*itr));
            }
            return results;
        }

        ::hiero::ConsensusTopicResponse MirrorClient::getTopicMessageBySequenceNumber(const ::hiero::TopicId& topicId, uint64_t sequenceNumber)
        {
            // https://testnet.mirrornode.hedera.com/api/v1/topics/0.0.2/messages/2
            JsonRequest request(getBasePath());
            std::string requestPath = "topics/" + topicId.toString();
            requestPath += "/messages/" + std::to_string(sequenceNumber);
            auto resultJson = request.getRequest(requestPath.data());
            if (resultJson.HasMember("_status")) {
                // seems something gone wrong
                const auto& status = resultJson["_status"];
                checkMember(status, "messages", MemberType::ARRAY, "_status");
                const auto& messages = status["messages"];
                std::string completeRequest = getBasePath() + requestPath;
                VLOG_SCOPE_F(-1, "error(s) in request: %s", completeRequest.data());
                for (auto itr = messages.Begin(); itr != messages.End(); ++itr) {
                    checkMember(*itr, "message", MemberType::STRING, "messages");
                    LOG_F(WARNING, "%s", (*itr)["message"].GetString());
                }
                return ::hiero::ConsensusTopicResponse();
            }
            return ::hiero::ConsensusTopicResponse(resultJson);
        }
    }
}
