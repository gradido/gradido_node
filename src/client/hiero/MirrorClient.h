#ifndef __GRADIDO_NODE_CLIENT_HIERO_MIRROR_CLIENT_H
#define __GRADIDO_NODE_CLIENT_HIERO_MIRROR_CLIENT_H

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
            MirrorClient(std::string_view networkType);
            ~MirrorClient();

            /*
             * https://mainnet.mirrornode.hedera.com/api/v1/topics/{topicId}/messages
             * https://mainnet.mirrornode.hedera.com/api/v1/docs/#/
             * @param topicId
             * @param limit 
             * @param offset sequence number
             * @param order desc or asc
             * @return list of serialized messages in base64
             */
            std::vector<::hiero::ConsensusTopicResponse> listTopicMessagesById(
                const ::hiero::TopicId& topicId,
                uint32_t limit = 25,
                uint64_t sequenceNumber = 0,
                const std::string& order = "asc"
            ); 
            ::hiero::ConsensusTopicResponse getTopicMessageBySequenceNumber(const ::hiero::TopicId& topicId, uint64_t sequenceNumber);

        protected:
            inline std::string getBasePath() const { return "https://" + mNetworkType + ".mirrornode.hedera.com/api/v1/"; }
            std::string mNetworkType;
        };
    }
}

#endif // __GRADIDO_NODE_CLIENT_HIERO_MIRROR_CLIENT_H