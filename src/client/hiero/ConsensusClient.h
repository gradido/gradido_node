#ifndef __GRADIDO_NODE_CLIENT_HIERO_CONSENSUS_CLIENT_H
#define __GRADIDO_NODE_CLIENT_HIERO_CONSENSUS_CLIENT_H

#include "../../hiero/ConsensusGetTopicInfoResponse.h"
#include "../../hiero/TransactionGetReceiptResponse.h"
#include "MessageObserver.h"

#include "gradido_blockchain/memory/Block.h"

#include <memory>
#include <string>

namespace grpc {
    class ChannelCredentials;
    class Client;    
}

namespace hiero {
    class TopicId;
    class TransactionId;
}

namespace client {
    namespace hiero {

        // Client for hiero/hedera Consensus Services
        class ConsensusClient 
        {
        public:
            ~ConsensusClient();

            // throw on error
            //! \param targetUrl full url with port and protocol
            static std::shared_ptr<ConsensusClient> createForTarget(
                const std::string& targetUrl,
                bool isTransportSecurity,
                memory::ConstBlockPtr certificateHash
            );

            void getTransactionReceipts(
                const ::hiero::TransactionId& transactionId,
                std::shared_ptr<MessageObserver<::hiero::TransactionGetReceiptResponseMessage>> responseListener
            );

            void getTopicInfo(
                const ::hiero::TopicId& topicId,
                std::shared_ptr<MessageObserver<::hiero::ConsensusGetTopicInfoResponseMessage>> responseListener
            );

        private: 
            ConsensusClient(std::shared_ptr<grpc::Channel> channel);
            static std::shared_ptr<grpc::ChannelCredentials> getTlsChannelCredentials(memory::ConstBlockPtr certificateHash);

            std::shared_ptr<grpc::Channel> mChannel;
        };
    }
}
#endif // __GRADIDO_NODE_CLIENT_HIERO_CONSENSUS_CLIENT_H