#include "IotaMessageToTransactionTask.h"

#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"

#include "../SingletonManager/LoggerManager.h"
#include "../SingletonManager/GroupManager.h"
#include "../SingletonManager/OrderingManager.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/http/IotaRequestExceptions.h"
#include "../ServerGlobals.h"

#include "Poco/Util/ServerApplication.h"

//#include "../lib/BinTextConverter.h"


IotaMessageToTransactionTask::IotaMessageToTransactionTask(
    uint32_t milestoneIndex, uint64_t timestamp, 
    iota::MessageId messageId)
: mMilestoneIndex(milestoneIndex), mTimestamp(timestamp), mMessageId(messageId)
{
#ifdef _UNI_LIB_DEBUG
    setName(messageId.toHex().data());
#endif
 
}

IotaMessageToTransactionTask::~IotaMessageToTransactionTask()
{
    OrderingManager::getInstance()->popMilestoneTaskObserver(mMilestoneIndex);
}

// TODO: has thrown a null pointer exception
int IotaMessageToTransactionTask::run()
{
    Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
    std::pair<std::unique_ptr<std::string>, std::unique_ptr<std::string>> dataIndex;
    try {
        dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(mMessageId.toHex());
    }
    catch (...) {
        IotaRequest::defaultExceptionHandler(errorLog, false);
        errorLog.warning("retry once after waiting 100 ms");
        Poco::Thread::sleep(100);
		try {
			dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(mMessageId.toHex());
		}
		catch (...) {
            // terminate application
            IotaRequest::defaultExceptionHandler(errorLog, true);
		}
    }

    auto gm = GroupManager::getInstance();
    auto group = gm->findGroup(getGradidoGroupAlias(*dataIndex.second.get()));
    
    auto transaction = std::make_unique<model::gradido::GradidoTransaction>(dataIndex.first.get());
        
    Poco::Logger& transactionLog = LoggerManager::getInstance()->mTransactionLog;
    transactionLog.information("%d %d %s %s\n%s", 
        (int)mMilestoneIndex, (int)mTimestamp, *dataIndex.second.get(), mMessageId.toHex(),
        transaction->toJson()
    );
    // if simple validation already failed, we can stop here
    try {
        transaction->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
        if (transaction->getTransactionBody()->isCreation()) {
            transaction->getTransactionBody()->getCreationTransaction()->validateTargetDate(mTimestamp);
        }
    } catch(model::gradido::TransactionValidationException& e) {
        errorLog.error(e.getFullString());
        return 0;
    }
   
    // TODO: Cross Group Check
    // on inbound
    // check if we listen to other group 
    // try to find the pairing transaction with the messageId
    if (transaction->getTransactionBody()->isInbound()) {
        auto parentMessageIdHex = transaction->getParentMessageId()->convertToHex();
        std::unique_ptr<model::gradido::GradidoTransaction> pairingTransaction;
        auto otherGroup = gm->findGroup(transaction->getTransactionBody()->getOtherGroup());
        if (!otherGroup.isNull()) {
            auto transactionEntry = otherGroup->findByMessageId(transaction->getParentMessageId(), true);
            pairingTransaction = std::make_unique<model::gradido::GradidoTransaction>(transactionEntry->getSerializedTransaction());
        }
        // load from iota
        if (!pairingTransaction) {
			std::pair<std::unique_ptr<std::string>, std::unique_ptr<std::string>> dataIndex;
            
			try {
				dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(*parentMessageIdHex.get());
                pairingTransaction = std::make_unique<model::gradido::GradidoTransaction>(dataIndex.first.get());
			}
			catch (...) {
				IotaRequest::defaultExceptionHandler(errorLog, false);
				errorLog.warning("retry once after waiting 100 ms");
				Poco::Thread::sleep(100);
				try {
					dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(*parentMessageIdHex.get());
					pairingTransaction = std::make_unique<model::gradido::GradidoTransaction>(dataIndex.first.get());
				}
				catch (...) {
					IotaRequest::defaultExceptionHandler(errorLog, false);                 
                }
			}
        }
        if (pairingTransaction) {
            if (!pairingTransaction->isBelongToUs(transaction.get())) {
                std::string parentMessageIdHexString(*parentMessageIdHex.get());
				errorLog.information("transaction skipped because pairing transaction wasn't found, messageId: %s, pairing message id: %s",
					mMessageId.toHex(), *parentMessageIdHex.get()
                );
                return 0;
            }
        }
    }
   
    // check if transaction already exist
    // if this transaction doesn't belong to us, we can quit here 
    // also if we already have this transaction
    if (group.isNull() || group->isSignatureInCache(transaction.get())) {
        errorLog.information("transaction skipped because it cames from other group or was found in cache, messageId: %s",
            mMessageId.toHex()
        );
        return 0;
    }       
    auto lastTransaction = group->getLastTransaction();
    if (lastTransaction && lastTransaction->getReceived() > mTimestamp) {
        // this transaction seems to be from the past, a transaction which happen after this was already added
        errorLog.information("transaction skipped because it cames from the past, messageId: %s", 
            mMessageId.toHex().data());
        return 0;
    }
        
    // hand over to OrderingManager
    //std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
    OrderingManager::getInstance()->pushTransaction(
        std::move(transaction), mMilestoneIndex,
        mTimestamp, *dataIndex.second.get(), mMessageId.toMemoryBin());

    return 0;
}

std::string IotaMessageToTransactionTask::getGradidoGroupAlias(const std::string& iotaIndex) const
{
	auto findPos = iotaIndex.find("GRADIDO");

	if (findPos != iotaIndex.npos) {
		return iotaIndex.substr(8);
	}
	return "";
}
