#include "IotaMessageToTransactionTask.h"

#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"
#include "gradido_blockchain/model/protobufWrapper/ProtobufExceptions.h"
#include "gradido_blockchain/lib/Profiler.h"

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
    auto iotaMessageIdHex = mMessageId.toHex();
    try {
        Profiler getIndexDataTime;
        dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(iotaMessageIdHex);
        LoggerManager::getInstance()->mSpeedLogging.error("time for getting indexiation message from iota: %s", getIndexDataTime.string());
    }
    catch (...) {
        IotaRequest::defaultExceptionHandler(errorLog, false);
        errorLog.warning("retry once after waiting 100 ms");
        Poco::Thread::sleep(100);
		try {
			dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(iotaMessageIdHex);
		}
		catch (...) {
            // terminate application
            IotaRequest::defaultExceptionHandler(errorLog, true);
		}
    }
    
    if (!dataIndex.first->size()) {
        errorLog.information("invalid gradido transaction, iota message ID: %s, empty body", iotaMessageIdHex);
        return 0;
    }
    auto gm = GroupManager::getInstance();
    auto group = gm->findGroup(getGradidoGroupAlias(*dataIndex.second.get()));
    
    std::unique_ptr<gradido::data::GradidoTransaction> transaction;
    try {
        auto hex = DataTypeConverter::binToHex((const unsigned char*)dataIndex.first.get()->data(), dataIndex.first.get()->size());
        // printf("[IotaMessageToTransactionTask::run] iota message id hex: %s\n", iotaMessageIdHex.data());
        // printf("[IotaMessageToTransactionTask::run] gradido transaction (%d): %s\n", dataIndex.first.get()->size(), hex.data());
        transaction = std::make_unique<gradido::data::GradidoTransaction>(dataIndex.first.get());
    }
    catch (ProtobufParseException& ex) {
        errorLog.information("invalid gradido transaction, iota message ID: %s", iotaMessageIdHex);
        auto hex = DataTypeConverter::binToHex((const unsigned char *)dataIndex.first.get()->data(), dataIndex.first.get()->size());
        errorLog.information("serialized: %s", hex);
        return 0;
    }
           
    Poco::Logger& transactionLog = LoggerManager::getInstance()->mTransactionLog;
    transactionLog.information("%d %d %s %s\n%s", 
        (int)mMilestoneIndex, (int)mTimestamp, *dataIndex.second.get(), mMessageId.toHex(),
        transaction->toJson()
    );

    // if simple validation already failed, we can stop here
    try {
        transaction->validate(gradido::data::TRANSACTION_VALIDATION_SINGLE);
        auto body = transaction->getTransactionBody();
        if (body->isCreation()) {
            body->getCreationTransaction()->validateTargetDate(body->getCreatedSeconds());
        }
    } catch(gradido::data::TransactionValidationException& e) {
        errorLog.error(e.getFullString());
        notificateFailedTransaction(group, transaction.get(), e.what());
        return 0;
    }
   
    // TODO: Cross Group Check
    // on inbound
    // check if we listen to other group 
    // try to find the pairing transaction with the messageId
    if (transaction->getTransactionBody()->isInbound()) {
        auto parentMessageIdHex = transaction->getParentMessageId()->convertToHex();
        std::unique_ptr<gradido::data::GradidoTransaction> pairingTransaction;
        auto otherGroup = gm->findGroup(transaction->getTransactionBody()->getOtherGroup());
        if (!otherGroup.isNull()) {
            auto transactionEntry = otherGroup->findByMessageId(transaction->getParentMessageId(), true);
            if (!transactionEntry.isNull()) {
                pairingTransaction = std::make_unique<gradido::data::GradidoTransaction>(transactionEntry->getSerializedTransaction());
            }
        }
        // load from iota
        if (!pairingTransaction) {
			std::pair<std::unique_ptr<std::string>, std::unique_ptr<std::string>> dataIndex;
            
			try {
				dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(*parentMessageIdHex.get());
                pairingTransaction = std::make_unique<gradido::data::GradidoTransaction>(dataIndex.first.get());
			}
			catch (...) {
				IotaRequest::defaultExceptionHandler(errorLog, false);
				errorLog.warning("retry once after waiting 100 ms");
				Poco::Thread::sleep(100);
				try {
					dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(*parentMessageIdHex.get());
					pairingTransaction = std::make_unique<gradido::data::GradidoTransaction>(dataIndex.first.get());
				}
				catch (...) {
					IotaRequest::defaultExceptionHandler(errorLog, false);                 
                }
			}
        }
        if (pairingTransaction) {
            if (!pairingTransaction->isBelongToUs(transaction.get())) {
                std::string parentMessageIdHexString(*parentMessageIdHex.get());
                std::string message = "transaction skipped because pairing transaction wasn't found";
                notificateFailedTransaction(group, transaction.get(), message);
				errorLog.information("%s, messageId: %s, pairing message id: %s",
                    message, mMessageId.toHex(), *parentMessageIdHex.get()
                );
                return 0;
            }
        }
    }

   
    // check if transaction already exist
    // if this transaction doesn't belong to us, we can quit here 
    // also if we already have this transaction
    if (group.isNull() || group->isTransactionAlreadyExist(transaction.get())) {
        errorLog.information("transaction skipped because it cames from other group or was found in cache, messageId: %s",
            mMessageId.toHex()
        );
        return 0;
    }       
    auto lastTransaction = group->getLastTransaction();
    if (lastTransaction && lastTransaction->getReceived() > mTimestamp) {
        // this transaction seems to be from the past, a transaction which happen after this was already added
        std::string message = "transaction skipped because it cames from the past";
        notificateFailedTransaction(group, transaction.get(), message);
        errorLog.information("%s, messageId: %s", message, mMessageId.toHex());
        return 0;
    }

    // hand over to OrderingManager
    //std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
    OrderingManager::getInstance()->pushTransaction(
        std::move(transaction), mMilestoneIndex,
        mTimestamp, group->getGroupAlias(), mMessageId.toMemoryBin());

    return 0;
}

std::string IotaMessageToTransactionTask::getGradidoGroupAlias(const std::string& iotaIndex) const
{
	auto findPos = iotaIndex.find("GRADIDO");

	if (findPos != iotaIndex.npos) {
		return iotaIndex.substr(8);
	}
    // it is a binary index from node js gradido implementation
    if (iotaIndex.size() == 32) {
        return DataTypeConverter::binToHex(iotaIndex).substr(0, 64);
    }
	return "";
}

void IotaMessageToTransactionTask::notificateFailedTransaction(
    Poco::SharedPtr<controller::Group> group,
	const gradido::data::GradidoTransaction* transaction,
	const std::string& errorMessage
)
{
	if (!group.isNull()) {
		auto communityServer = group->getListeningCommunityServer();
		if (communityServer) {
			communityServer->notificateFailedTransaction(transaction, errorMessage, mMessageId.toHex());
		}
	}
}
