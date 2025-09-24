#ifndef __GRADIDO_NODE_CLIENT_HIERO_MIRROR_CLIENT_H
#define __GRADIDO_NODE_CLIENT_HIERO_MIRROR_CLIENT_H

#include "MirrorNetworkEndpoints.h"
#include "gradido_blockchain/data/hiero/TopicId.h"
#include "../../hiero/ConsensusTopicResponse.h"

#include <string_view>
#include <string>
#include <vector>

namespace grpc {
    class Channel;
}

namespace client {
    namespace hiero {

        class TopicMessageQuery;

        // Client for Hiero/Hedera Mirror Services
        class MirrorClient
        {
        public:
            //! \param networkType testnet, previewnet, mainnet are the subdomains for mirror nodes
            MirrorClient(std::string_view networkType);
            // MAYBE TODO: think and test if neccessary to track all open grpc connections and delete mirror channel after all connections are closed
            ~MirrorClient();

            // REST API

            /*
             * https://mainnet.mirrornode.hedera.com/api/v1/topics/{topicId}/messages
             * https://mainnet.mirrornode.hedera.com/api/v1/docs/#/
             * @param topicId
             * @param confirmedAt consensus timestamp as filter, on asc search for messages with consensus timestamp > as this, < on desc
             * @param limit              
             * @param order desc or asc
             * @return list of serialized messages in base64
             */
            std::vector<::hiero::ConsensusTopicResponse> listTopicMessagesById(
                const ::hiero::TopicId& topicId,
                gradido::data::Timestamp confirmedAt = gradido::data::Timestamp(),
                uint32_t limit = 25,
                // TODO: use enum for order
                const std::string& order = "asc"
            ); 
            ::hiero::ConsensusTopicResponse getTopicMessageBySequenceNumber(const ::hiero::TopicId& topicId, uint64_t sequenceNumber);
            ::hiero::ConsensusTopicResponse getTopicMessageByConsensusTimestamp(gradido::data::Timestamp confirmedAt);

            // gRPC API
            void subscribeTopic(std::shared_ptr<TopicMessageQuery> responseListener);

            inline std::string getProtocolHost() const { return "https://" + std::string(getHost()); }
            inline const char* getHost() const { return MirrorNetworkEndpoints::getByEndpointName(mNetworkType.data()); }

        protected: 
            enum RequestResponseTypes {
                OK = 0,
                NOT_FOUND,
                PARAMETER_ERROR
            };
            rapidjson::Document requestAndCatch(const std::string& requestPath);
            RequestResponseTypes isErrorInRequest(const rapidjson::Document& responseJson);
            inline std::string getBasePath() const { return "/api/v1"; }
            std::string mNetworkType;

            // for grpc
            std::shared_ptr<grpc::Channel> mChannel;
        };
    }
}

#endif // __GRADIDO_NODE_CLIENT_HIERO_MIRROR_CLIENT_H