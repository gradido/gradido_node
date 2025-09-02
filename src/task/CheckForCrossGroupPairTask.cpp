#include "CheckForCrossGroupPairTask.h"
#include "gradido_blockchain/data/GradidoTransaction.h"

namespace task {
    CheckForCrossGroupPairTask::CheckForCrossGroupPairTask(std::shared_ptr<const gradido::data::GradidoTransaction> transaction) 
    : mTransaction(transaction), mIsClosed(false), mIsSuccess(false), mMessageArrived(false)
    {

    }

    int CheckForCrossGroupPairTask::run()
    {
        return 0;
    }

    void CheckForCrossGroupPairTask::onMessageArrived(const hiero::TransactionGetReceiptResponseMessage& message)
    {
        hiero::TransactionGetReceiptResponse response(message);
        mMessageArrived = true;
    }

    // from MessageObserver
    // will be called from grpc client if connection was closed
    // so no more messageArrived calls
    void CheckForCrossGroupPairTask::onConnectionClosed()
    {

    }
}