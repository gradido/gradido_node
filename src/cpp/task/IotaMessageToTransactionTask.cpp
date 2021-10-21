#include "IotaMessageToTransactionTask.h"

#include "../model/transactions/GradidoTransaction.h"
#include "../SingletonManager/LoggerManager.h"
#include "../lib/DataTypeConverter.h"

IotaMessageToTransactionTask(uint32_t milestoneIndex, uint64_t timestamp, const iota::MessageId& messageId)
: mMilestoneIndex(milestoneIndex), mTimestamp(timestamp), mMessageId(messageId)
{

}

IotaMessageToTransactionTask::~IotaMessageToTransactionTask()
{

}

int IotaMessageToTransactionTask::run()
{
    // get the whole transaction from iota
    auto bin = iota::getIndexiationMessage(mMessageId);
    // parse proto Object from it and

    Poco::AutoPtr<model::GradidoTransaction> transaction(new model::GradidoTransaction(std::string(bin.data(), bin.size())));
    if(transaction.errorCount() > 0) {
        auto errorLog = LoggerManager::getInstance()->mErrorLogging;
        auto errors = transaction->getErrorsArray();
        errorLog.error("Error by parsing transaction");
        errorLog.error("Transaction as base64: %s", DataTypeConverter::binToBase64(bin));
        for(auto it = errors.begin(); it != errors.end();  it++) {
            errorLog.error(*it);
        }
        MemoryManager::getInstance()->freeMemory(bin);
    } else {
        // hand over to OrderingManager
        std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
        MemoryManager::getInstance()->freeMemory(bin);
    }

    return 0;
}
