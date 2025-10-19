#include "MirrorClient.h"
#include "TopicMessageQuery.h"

#include "gradido_blockchain/http/JsonRequest.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/lib/RapidjsonHelper.h"


#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"

#include "grpcpp/generic/generic_stub.h"
#include "grpcpp/security/credentials.h"
#include "grpcpp/support/channel_arguments.h"
#include "grpcpp/support/stub_options.h"
#include "grpcpp/channel.h"

#include <chrono>

using namespace rapidjson;
using namespace rapidjson_helper;
using namespace magic_enum;
using namespace std::chrono;

namespace client {
    namespace hiero {
        MirrorClient::MirrorClient(std::string_view networkType)
            : mNetworkType(networkType)
        {
            // for grpc api
            grpc::ChannelArguments channelArguments;
            channelArguments.SetInt(GRPC_ARG_ENABLE_RETRIES, 0);
            channelArguments.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);
            channelArguments.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

            auto mirrorCredentials = ::grpc::experimental::TlsCredentials(::grpc::experimental::TlsChannelCredentialsOptions());
            mChannel = ::grpc::CreateCustomChannel(getHost(), mirrorCredentials, channelArguments);
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
                requestPath += confirmedAt.toString();
            }
            auto resultJson = requestAndCatch(requestPath);
            if (resultJson.IsNull()) return results;
            checkMember(resultJson, "messages", MemberType::ARRAY, "root");
            
            auto messages = resultJson["messages"].GetArray();
            results.reserve(messages.Size());
            for (auto& msg : resultJson["messages"].GetArray()) {
                results.push_back(::hiero::ConsensusTopicResponse(msg));
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
            requestPath += "/messages/" + confirmedAt.toString();
            
            auto resultJson = requestAndCatch(requestPath);
            if (resultJson.IsNull()) {
                return ::hiero::ConsensusTopicResponse();
            }
            return ::hiero::ConsensusTopicResponse(resultJson);
        }

        void MirrorClient::subscribeTopic(std::shared_ptr<TopicMessageQuery> responseListener) {
            if (!responseListener) {
                throw GradidoNullPointerException("missing response listener", "TopicMessageQuery", __FUNCTION__);
            }

            grpc::StubOptions options;
            grpc::TemplatedGenericStub<grpc::ByteBuffer, grpc::ByteBuffer> mirrorStub(mChannel);
            auto readerWriter = std::move(mirrorStub.PrepareCall(
                responseListener->getClientContextPtr(),
                "/com.hedera.mirror.api.proto.ConsensusService/subscribeTopic",
                responseListener->getCompletionQueuePtr()
            ));
            readerWriter->StartCall(responseListener->getCallStatusPtr());
            responseListener->setResponseReader(readerWriter);
        }

        Document MirrorClient::requestAndCatch(const std::string& requestPath)
        {
            JsonRequest request(getProtocolHost());
            std::string fullRequestPath(getBasePath());
            fullRequestPath += requestPath;

            try {
                LOG_F(1, "call: %s%s", getProtocolHost().data(), fullRequestPath.data());
                auto resultJson = request.getRequest(fullRequestPath.data());
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
