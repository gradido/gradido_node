#include "IotaMessageToTransactionTask.h"

#include "gradido_blockchain/lib/Profiler.h"

#include "../SingletonManager/OrderingManager.h"
#include "../blockchain/FileBasedProvider.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/http/IotaRequestExceptions.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/interaction/toJson/Context.h"
#include "gradido_blockchain/interaction/validate/Context.h"
#include "../ServerGlobals.h"

#include "loguru/loguru.hpp"
#include <thread>
#include <chrono>

using namespace gradido;
using namespace data;
using namespace blockchain;
using namespace interaction;

IotaMessageToTransactionTask::IotaMessageToTransactionTask(
    uint32_t milestoneIndex, Timepoint timestamp,
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
    std::pair<std::unique_ptr<std::string>, std::unique_ptr<std::string>> dataIndex;
    auto iotaMessageIdHex = mMessageId.toHex();
    LOG_F(1, "start processing transaction: %s", iotaMessageIdHex.data());
    try {
        Profiler getIndexDataTime;
        dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(iotaMessageIdHex);
        // LOG_F(INFO, "time for getting indexiation message from iota: %s", getIndexDataTime.string().data());
    }
    catch (...) {
        IotaRequest::defaultExceptionHandler(false);
        LOG_F(WARNING, "retry once after waiting 100 ms");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
		try {
			dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(iotaMessageIdHex);
		}
		catch (...) {
            // terminate application
            IotaRequest::defaultExceptionHandler(true);
		}
    }
    
    if (!dataIndex.first->size()) {
        LOG_F(INFO, "invalid gradido transaction, iota message ID: %s, empty body", iotaMessageIdHex.data());
        return 0;
    }
    auto blockchainProvider = gradido::blockchain::FileBasedProvider::getInstance();
    auto communityId = getGradidoGroupAlias(*dataIndex.second.get());
    auto blockchain = blockchainProvider->findBlockchain(communityId);
    
    if(!blockchain) {
         LOG_F(INFO, "transaction skipped because it came from unknown community, messageId: %s, communityId: %s",
            mMessageId.toHex().data(), communityId.data()
        );
        return 0;
    }

    ConstGradidoTransactionPtr transaction;
   
    auto hex = DataTypeConverter::binToHex((const unsigned char*)dataIndex.first.get()->data(), dataIndex.first.get()->size());
    // printf("[IotaMessageToTransactionTask::run] iota message id hex: %s\n", iotaMessageIdHex.data());
    // printf("[IotaMessageToTransactionTask::run] gradido transaction (%d): %s\n", dataIndex.first.get()->size(), hex.data());
    auto rawTransaction = std::make_shared<memory::Block>(*dataIndex.first.get());
    deserialize::Context deserializer(rawTransaction, deserialize::Type::GRADIDO_TRANSACTION);
    deserializer.run();
    if (deserializer.isGradidoTransaction()) {
        transaction = deserializer.getGradidoTransaction();
    } else {
        LOG_F(INFO, "invalid gradido transaction, iota message ID: %s", iotaMessageIdHex.data());
        LOG_F(INFO, "serialized: %s", rawTransaction->convertToHex().data());
        return 0;
    }

    // log transaction in json format with low verbosity level 1 = debug

    toJson::Context toJson(*transaction);    
    auto transactionAsJson = toJson.run(true);
    auto messageIdHex = mMessageId.toHex();
    LOG_F(
        2, 
        "%u %s %s\n%s",
        mMilestoneIndex, 
        DataTypeConverter::timePointToString(mTimestamp).data(),
        // dataIndex.second.get()->data(),
        messageIdHex.data(),
        transactionAsJson.data()
    );
    
    // if simple validation already failed, we can stop here
    try {
        validate::Context validator(*transaction);
        validator.run(validate::Type::SINGLE);
    } catch(GradidoBlockchainException& e) {
        LOG_F(ERROR, e.getFullString().data());
        notificateFailedTransaction(blockchain, *transaction, e.what());
        return 0;
    }
    
   
    // TODO: Cross Group Check
    // on inbound
    // check if we listen to other group 
    // try to find the pairing transaction with the messageId
    if (CrossGroupType::INBOUND == transaction->getTransactionBody()->getType()) {
        auto parentMessageIdHex = transaction->getParingMessageId()->convertToHex();
        ConstGradidoTransactionPtr pairingTransaction;
        auto otherGroup = blockchainProvider->findBlockchain(transaction->getTransactionBody()->getOtherGroup());
        if (otherGroup) {
            auto transactionEntry = otherGroup->findByMessageId(transaction->getParingMessageId());
            if (transactionEntry) {
                pairingTransaction = transactionEntry->getConfirmedTransaction()->getGradidoTransaction();
            }
        }
        // load from iota
        if (!pairingTransaction) {
			std::pair<std::unique_ptr<std::string>, std::unique_ptr<std::string>> dataIndex;
            
            try {
                dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(parentMessageIdHex);
                auto transactionRaw = std::make_shared <memory::Block>(*dataIndex.first);
                deserialize::Context deserializer(transactionRaw, deserialize::Type::GRADIDO_TRANSACTION);
                deserializer.run();
                if (deserializer.isGradidoTransaction()) {
                    pairingTransaction = deserializer.getGradidoTransaction();
                }
            } catch(...) {
                IotaRequest::defaultExceptionHandler(false);
                LOG_F(WARNING, "retry once after waiting 100 ms");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                try {
                    dataIndex = ServerGlobals::g_IotaRequestHandler->getIndexiationMessageDataIndex(parentMessageIdHex);
                    auto transactionRaw = std::make_shared <memory::Block>(*dataIndex.first);
                    deserialize::Context deserializer(transactionRaw, deserialize::Type::GRADIDO_TRANSACTION);
                    deserializer.run();
                    if (deserializer.isGradidoTransaction()) {
                        pairingTransaction = deserializer.getGradidoTransaction();
                    }
                }
                catch (...) {
                    IotaRequest::defaultExceptionHandler(false);
                }
            }
        }
        if (pairingTransaction) {
            if (!pairingTransaction->isPairing(*transaction)) {
                std::string message = "transaction skipped because pairing transaction wasn't found";
                notificateFailedTransaction(blockchain, *transaction, message);
				LOG_F(INFO, "%s, messageId: %s, pairing message id: %s",
                    message.data(), mMessageId.toHex().data(), parentMessageIdHex.data()
                );
                return 0;
            }
        }
    }

   
    // check if transaction already exist
    // if this transaction doesn't belong to us, we can quit here 
    // also if we already have this transaction
    auto fileBasedBlockchain = std::dynamic_pointer_cast<FileBased>(blockchain);
    assert(fileBasedBlockchain);
    if (fileBasedBlockchain->isTransactionAlreadyExist(*transaction)) {
        LOG_F(INFO, "transaction skipped because it was found in cache, messageId: %s",
            mMessageId.toHex().data()
        );
        return 0;
    }       
    auto lastTransaction = blockchain->findOne(Filter::LAST_TRANSACTION);
    if (lastTransaction && lastTransaction->getConfirmedTransaction()->getConfirmedAt() > mTimestamp) {
        // this transaction seems to be from the past, a transaction which happen after this was already added
        std::string message = "transaction skipped because it cames from the past";
        notificateFailedTransaction(blockchain, *transaction, message);
        LOG_F(INFO, "%s, messageId: %s", message.data(), mMessageId.toHex().data());
        return 0;
    }

    // hand over to OrderingManager
    //std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
    OrderingManager::getInstance()->pushTransaction(
        transaction, mMilestoneIndex,
        mTimestamp, blockchain->getCommunityId(), std::make_shared<memory::Block>(mMessageId.toMemoryBlock()));

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
    std::shared_ptr<gradido::blockchain::Abstract> blockchain,
	const gradido::data::GradidoTransaction transaction,
	const std::string& errorMessage
)
{
	if (blockchain) {
		auto communityServer = std::dynamic_pointer_cast<gradido::blockchain::FileBased>(blockchain)->getListeningCommunityServer();
		if (communityServer) {
			communityServer->notificateFailedTransaction(transaction, errorMessage, mMessageId.toHex());
		}
	}
}
