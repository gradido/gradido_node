#ifndef __GRADIDO_NODE_TASK_DESERIALIZE_CONFIRMED_TRANSACTIONTASK_H
#define __GRADIDO_NODE_TASK_DESERIALIZE_CONFIRMED_TRANSACTIONTASK_H

#include "CPUTask.h"
#include <memory>

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
    class DeserializeConfirmedTransactionTask : public CPUTask {
    public:
        DeserializeConfirmedTransactionTask(memory::ConstBlockPtr rawTransaction, uint32_t communityIdIndex);
        virtual ~DeserializeConfirmedTransactionTask();

        const char* getResourceType() const override { return "DeserializeConfirmedTransactionTask"; };
		int run() override;

        inline  std::shared_ptr<const gradido::data::ConfirmedTransaction> getConfirmedTransaction() const { return mConfirmedTransaction; }

    private:
        memory::ConstBlockPtr mRawTransaction;
        std::shared_ptr<const gradido::data::ConfirmedTransaction> mConfirmedTransaction;
        uint32_t mCommunityIdIndex;
    };
}

#endif // __GRADIDO_NODE_TASK_DESERIALIZE_CONFIRMED_TRANSACTIONTASK_H