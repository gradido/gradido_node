#ifndef __GRADIDO_NODE_CLIENT_HIERO_MIRROR_CLIENT_H
#define __GRADIDO_NODE_CLIENT_HIERO_MIRROR_CLIENT_H

#include "MirrorNetworkEndpoints.h"
#include "gradido_blockchain/data/hiero/TopicId.h"
#include "../../hiero/ConsensusTopicResponse.h"

#include <string_view>
#include <string>
#include <vector>

namespace client {
    namespace hiero {

        class MirrorClient
        {
        public:
            //! \param networkType testnet, previewnet, mainnet are the subdomains for mirror nodes
            MirrorClient(std::string_view networkType);
            ~MirrorClient();

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
            inline std::string getProtocolHost() const { return "https://" + std::string(getHost()); }
            inline const char* getHost() const { return MirrorNetworkEndpoints::getByEndpointName(mNetworkType.data()); }

        protected: 
            enum RequestResponseTypes {
                OK = 0,
                NOT_FOUND,
                PARAMETER_ERROR
            };
            std::string timestampToString(gradido::data::Timestamp timestamp);
            rapidjson::Document requestAndCatch(const std::string& requestPath);
            RequestResponseTypes isErrorInRequest(const rapidjson::Document& responseJson);
            inline std::string getBasePath() const { return "/api/v1"; }
            std::string mNetworkType;
        };
    }
}

#endif // __GRADIDO_NODE_CLIENT_HIERO_MIRROR_CLIENT_H