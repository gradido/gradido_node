#ifndef __GRADIDO_NODE_TASK_BATCH_DESERIALIZE_CONFIRMED_TRANSACTIONTASK_H
#define __GRADIDO_NODE_TASK_BATCH_DESERIALIZE_CONFIRMED_TRANSACTIONTASK_H

#include "CPUTask.h"
#include <memory>
#include <vector>

namespace gradido {
    namespace data {
        class ConfirmedTransaction;
    }
}

namespace memory {
    class Block;
    typedef std::shared_ptr<const Block> ConstBlockPtr;
}

namespace task {

    class BatchDeserializeConfirmedTransactionTask : public CPUTask {
    public:
        BatchDeserializeConfirmedTransactionTask(std::vector<memory::ConstBlockPtr>&& rawTransactions, uint32_t communityIdIndex);
        virtual ~BatchDeserializeConfirmedTransactionTask();

        const char* getResourceType() const override { return "BatchDeserializeConfirmedTransactionTask"; };
		    int run() override;

        inline const std::vector<std::shared_ptr<const gradido::data::ConfirmedTransaction>>& getConfirmedTransactions() const { return mConfirmedTransactions; }
        inline const std::vector<memory::ConstBlockPtr>& getRawTransactions() const { return mRawTransactions; }

    private:
        std::vector<memory::ConstBlockPtr> mRawTransactions;
        std::vector<std::shared_ptr<const gradido::data::ConfirmedTransaction>> mConfirmedTransactions;
        uint32_t mCommunityIdIndex;
    };
}

#endif // __GRADIDO_NODE_TASK_BATCH_DESERIALIZE_CONFIRMED_TRANSACTIONTASK_H