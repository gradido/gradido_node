#ifndef __GRADIDO_NODE_TASK_HIERO_MESSAGE_TO_TRANSACTION_TASK
#define __GRADIDO_NODE_TASK_HIERO_MESSAGE_TO_TRANSACTION_TASK

#include "CPUTask.h"
#include "gradido_blockchain/data/GradidoTransaction.h"
#include "gradido_blockchain/data/hiero/TransactionId.h"
/*!
 * @author: einhornimmond
 * 
 * @date: 27.08.2025
 * 
 * @brief: Get confirmed transaction id from Hiero, process it
 * 
 * Get confirmed transaction id from Iota,
 * get the whole transaction from iota,
 * parse proto Object from it and 
 * hand over to OrderingManager
 */

namespace gradido {
    namespace blockchain {
        class Abstract;
    }
}

class HieroMessageToTransactionTask : public task::CPUTask
{
public:
    HieroMessageToTransactionTask(
        const gradido::data::Timestamp& consensusTimestamp,
        std::shared_ptr<const memory::Block> transactionRaw,
        const std::string_view communityId
    );

    const char* getResourceType() const {return "HieroMessageToTransactionTask";};
    int run();
    bool isSuccess() const { return mSuccess; }
    std::shared_ptr<const gradido::data::GradidoTransaction> getGradidoTransaction() { return mTransaction; }

    ~HieroMessageToTransactionTask();

protected:
    void notificateFailedTransaction(
        std::shared_ptr<gradido::blockchain::Abstract> blockchain,
        const gradido::data::GradidoTransaction transaction, 
        const std::string& errorMessage
    );    

    gradido::data::Timestamp mConsensusTimestamp;
    std::shared_ptr<const memory::Block> mTransactionRaw;
    std::string mCommunityId;
    std::shared_ptr<const gradido::data::GradidoTransaction> mTransaction;    
    std::atomic<bool> mSuccess;
};

#endif //__GRADIDO_NODE_TASK_HIERO_MESSAGE_TO_TRANSACTION_TASK