#ifndef __GRADIDO_NODE_TASK_SYNC_TOPIC_ON_STARTUP_H
#define __GRADIDO_NODE_TASK_SYNC_TOPIC_ON_STARTUP_H

#include "CPUTaskGRPCReactor.h"
#include "../hiero/ConsensusGetTopicInfoResponse.h"
#include "gradido_blockchain/data/GradidoTransaction.h"

namespace gradido {
    namespace blockchain {
        class FileBased;
    }
}
// how many hiero messages should be requested from a topic with each request
// TODO: put it into config
#define MAGIC_NUMBER_MIRROR_API_GET_TOPIC_MESSAGES_BULK_SIZE 25

namespace task {
    class SyncTopicOnStartup: public CPUTaskGRPCReactor<hiero::ConsensusGetTopicInfoResponse, hiero::ConsensusGetTopicInfoResponseMessage> {
    public:
        SyncTopicOnStartup(
            uint64_t lastKnownSequenceNumber, 
            hiero::TopicId lastKnowTopicId,
            std::shared_ptr<gradido::blockchain::FileBased> blockchain
        );
        virtual ~SyncTopicOnStartup();

        int run() override;
        const char* getResourceType() const override { return "SyncTopicOnStartup"; }

    protected:
        bool checkLastTransaction() const; 
        uint32_t loadTransactionsFromMirrorNode(hiero::TopicId topicId, uint64_t startSequenceNumber);

        uint64_t mLastKnownSequenceNumber;
        hiero::TopicId mLastKnowTopicId;
        std::shared_ptr<gradido::blockchain::FileBased> mBlockchain;
    };
}

#endif // __GRADIDO_NODE_TASK_SYNC_TOPIC_ON_STARTUP_H