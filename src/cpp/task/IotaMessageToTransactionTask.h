#ifndef __GRADIDO_NODE_TASK_IOTA_MESSAGE_TO_TRANSACTION_TASK
#define __GRADIDO_NODE_TASK_IOTA_MESSAGE_TO_TRANSACTION_TASK

#include "CPUTask.h"
#include "../model/transactions/GradidoTransaction.h"
#include "../iota/IotaWrapper.h"

/*!
 * @author: einhornimmond
 * 
 * @date: 20.10.2021
 * 
 * @brief: Get confirmed transaction id from Iota, process it
 * 
 * Get confirmed transaction id from Iota,
 * get the whole transaction from iota,
 * parse proto Object from it and 
 * hand over to OrderingManager
 */


class IotaMessageToTransactionTask : public UniLib::controller::CPUTask
{
public:
    IotaMessageToTransactionTask(uint32_t milestoneIndex, uint64_t timestamp, const iota::MessageId& messageId);

    const char* getResourceType() const {return "MessageToTransactionTask";};
    int run();

    ~IotaMessageToTransactionTask();

protected:
    uint32_t mMilestoneIndex;
    uint64_t mTimestamp;
    iota::MessageId mMessageId;
    Poco::AutoPtr<model::GradidoTransaction> mTransaction;
};

#endif //__GRADIDO_NODE_TASK_IOTA_MESSAGE_TO_TRANSACTION_TASK