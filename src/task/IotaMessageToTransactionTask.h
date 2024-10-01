#ifndef __GRADIDO_NODE_TASK_IOTA_MESSAGE_TO_TRANSACTION_TASK
#define __GRADIDO_NODE_TASK_IOTA_MESSAGE_TO_TRANSACTION_TASK

#include "CPUTask.h"
#include "gradido_blockchain/data/GradidoTransaction.h"
#include "gradido_blockchain/data//iota/MessageId.h"
#include "../controller/Group.h"
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


class IotaMessageToTransactionTask : public task::CPUTask
{
public:
    IotaMessageToTransactionTask(uint32_t milestoneIndex, uint64_t timestamp, iota::MessageId message);

    const char* getResourceType() const {return "IotaMessageToTransactionTask";};
    int run();

    ~IotaMessageToTransactionTask();
    inline uint32_t getMilestoneIndex() const { return mMilestoneIndex; }

protected:
    std::string getGradidoGroupAlias(const std::string& iotaIndex) const;
    void notificateFailedTransaction(
        std::shared_ptr<controller::Group> group,
        const gradido::data::GradidoTransaction* transaction, 
        const std::string& errorMessage
    );

    uint32_t mMilestoneIndex;
    //! seconds
    uint64_t mTimestamp;
    iota::MessageId mMessageId;
};

#endif //__GRADIDO_NODE_TASK_IOTA_MESSAGE_TO_TRANSACTION_TASK