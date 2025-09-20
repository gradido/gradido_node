#include "MirrorClient.h"

#include "gradido_blockchain/http/JsonRequest.h"
#include "gradido_blockchain/http/RequestExceptions.h"
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
            gradido::data::Timestamp confirmedAt /*  = gradido::data::Timestamp() */,
            uint32_t limit /* = 25*/,
            const std::string& order /* = "asc" */
        ) {
            std::vector<::hiero::ConsensusTopicResponse> results;

            std::string requestPath = "/topics/" + topicId.toString();
            requestPath += "/messages";
            requestPath += "?limit=" + std::to_string(limit);
            requestPath += "&order=" + order;
            if (!confirmedAt.empty()) {
                requestPath += "&timestamp=";
                if (order == "asc" || order == "ASC") {
                    requestPath += "gt:";
                }
                else if (order == "desc" || order == "DESC") {
                    requestPath += "lt:";
                }
                requestPath += timestampToString(confirmedAt);
            }
            auto resultJson = requestAndCatch(requestPath);
            if (resultJson.IsNull()) return results;
            checkMember(resultJson, "messages", MemberType::ARRAY, "root");
            
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
            std::string requestPath = "/topics/" + topicId.toString();
            requestPath += "/messages/" + std::to_string(sequenceNumber);
                    
            auto resultJson = requestAndCatch(requestPath);
            if (resultJson.IsNull()) {
                return ::hiero::ConsensusTopicResponse();
            }
            return ::hiero::ConsensusTopicResponse(resultJson);            
        }

        ::hiero::ConsensusTopicResponse MirrorClient::getTopicMessageByConsensusTimestamp(gradido::data::Timestamp confirmedAt)
        {
            // https://testnet.mirrornode.hedera.com/api/v1/topics/0.0.2/messages/1762812.121
            std::string requestPath = "/topics";
            requestPath += "/messages/" + timestampToString(confirmedAt);
            
            auto resultJson = requestAndCatch(requestPath);
            if (resultJson.IsNull()) {
                return ::hiero::ConsensusTopicResponse();
            }
            return ::hiero::ConsensusTopicResponse(resultJson);
        }

        std::string MirrorClient::timestampToString(gradido::data::Timestamp timestamp)
        {
            return std::to_string(timestamp.getSeconds()) + '.' + std::to_string(timestamp.getNanos());
        }

        Document MirrorClient::requestAndCatch(const std::string& requestPath)
        {
            JsonRequest request(getProtocolHost());
            std::string fullRequestPath(getBasePath());
            fullRequestPath += requestPath;

            try {
                LOG_F(1, "call: %s%s", getProtocolHost().data(), fullRequestPath.data());
                auto resultJson = request.getRequest(requestPath.data());
                if (!isErrorInRequest(resultJson)) {
                    return resultJson;
                }                
            }
            catch (HttplibRequestException& ex) {
                if (ex.getError() != "Success") {
                    LOG_F(ERROR, "error requesting %s: %s", fullRequestPath.data(), ex.getFullString().data());
                }
                else {
                    LOG_F(1, "%s return status: %d but error: Success", fullRequestPath.data(), ex.getStatus());
                }
            } catch (RapidjsonParseErrorException& ex) {
                LOG_F(WARNING, "%s response isn't valid json: %s", fullRequestPath.data(), ex.getFullString().data());
            }
            catch (std::exception& ex) {
                LOG_F(ERROR, "%s result in unexpected exception: %s", fullRequestPath.data(), ex.what());
            }
            return Document(kNullType);
        }

        MirrorClient::RequestResponseTypes MirrorClient::isErrorInRequest(const Document& responseJson)
        {
            if (responseJson.HasMember("_status")) {
                // seems something gone wrong
                const auto& status = responseJson["_status"];
                checkMember(status, "messages", MemberType::ARRAY, "_status");
                const auto& messages = status["messages"];
                if (messages.Size() == 1 && 0 == strcmp(messages["message"].GetString(), "Not found")) {
                    return RequestResponseTypes::NOT_FOUND;
                }

                VLOG_SCOPE_F(-1, "error(s) in request");                
                for (auto itr = messages.Begin(); itr != messages.End(); ++itr) {
                    checkMember(*itr, "message", MemberType::STRING, "messages");
                    LOG_F(WARNING, "%s", (*itr)["message"].GetString());
                }
                return RequestResponseTypes::PARAMETER_ERROR;
            }
            return RequestResponseTypes::OK;
        }
    }
}
