#ifndef __GRADIDO_NODE_TASK_CHECK_FOR_CROSS_GROUP_PAIR_TASK_H
#define __GRADIDO_NODE_TASK_CHECK_FOR_CROSS_GROUP_PAIR_TASK_H

#include "CPUTask.h"
#include "../client/grpc/MessageObserver.h"
#include "../hiero/TransactionGetReceiptResponse.h"

#include <atomic>

namespace gradido {
    namespace data {
        class GradidoTransaction;
    }
}

namespace task {
    class CheckForCrossGroupPairTask : public CPUTask, public client::grpc::MessageObserver<hiero::TransactionGetReceiptResponseMessage>
    {
    public:
        CheckForCrossGroupPairTask(std::shared_ptr<const gradido::data::GradidoTransaction> transaction);
        virtual ~CheckForCrossGroupPairTask() = default;

        // deserialize request result and check for cross group pair
        virtual int run() override;
        const char* getResourceType() const override { return "CheckForCrossGroupPairTask"; }
        bool isReady() override {return mMessageArrived; }

        // from MessageObserver
        // will be schedule task
        virtual void onMessageArrived(const hiero::TransactionGetReceiptResponseMessage& message) override;

        // from MessageObserver
        // will be called from grpc client if connection was closed
        // so no more messageArrived calls
        virtual void onConnectionClosed() override;

        inline bool isSuccess() const { return mIsSuccess; }

    protected:
        std::shared_ptr<const gradido::data::GradidoTransaction> mTransaction;
        
        std::atomic<bool> mIsClosed;
        std::atomic<bool> mIsSuccess;
        std::atomic<bool> mMessageArrived;
    };
}

#endif //__GRADIDO_NODE_TASK_CHECK_FOR_CROSS_GROUP_PAIR_TASK_H