#include "HieroMessageToTransactionTask.h"

#include "gradido_blockchain/lib/Profiler.h"

#include "../blockchain/FileBasedProvider.h"
#include "../controller/SimpleOrderingManager.h"
#include "gradido_blockchain/blockchain/Filter.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/interaction/validate/Context.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/serialization/toJsonString.h"
#include "gradido_blockchain/const.h"
#include "../ServerGlobals.h"

#include "loguru/loguru.hpp"
#include <thread>
#include <chrono>

using namespace gradido;
using namespace data;
using namespace blockchain;
using namespace interaction;
using namespace std::chrono_literals;

namespace task {

    HieroMessageToTransactionTask::HieroMessageToTransactionTask(
        const gradido::data::Timestamp& consensusTimestamp,
        std::shared_ptr<const memory::Block> transactionRaw,
        const std::string_view communityId
    ) : mConsensusTimestamp(consensusTimestamp), mTransactionRaw(transactionRaw), mCommunityId(communityId), mSuccess(false)
    {
#ifdef _UNI_LIB_DEBUG
        setName(DataTypeConverter::timePointToString(consensusTimestamp.getAsTimepoint()).data());
#endif
    }

    HieroMessageToTransactionTask::~HieroMessageToTransactionTask()
    {
    }

    int HieroMessageToTransactionTask::run()
    {
        if (loguru::g_stderr_verbosity >= 1) {
            LOG_F(1, "start processing transaction: %s", mConsensusTimestamp.toString().data());
        }

        auto blockchainProvider = gradido::blockchain::FileBasedProvider::getInstance();
        auto blockchain = blockchainProvider->findBlockchain(mCommunityId);

        if (!blockchain) {
            LOG_F(INFO, "Transaction skipped (unknown community), communityId: %s", mCommunityId.data());
            return 0;
        }

        // deserialize
        deserialize::Context deserializer(mTransactionRaw, deserialize::Type::GRADIDO_TRANSACTION);
        deserializer.run();
        if (deserializer.isGradidoTransaction()) {
            mTransaction = deserializer.getGradidoTransaction();
        }
        else {
            if (loguru::g_stderr_verbosity >= 0) {
                LOG_F(INFO, "Transaction skipped (invalid): %s", mConsensusTimestamp.toString().data());
                if (loguru::g_stderr_verbosity >= 2) {
                    LOG_F(2, "serialized: %s", mTransactionRaw->convertToHex().data());
                }
            }
            return 0;
        }

        // log transaction in json format with low verbosity level 1 = debug
        if (loguru::g_stderr_verbosity >= 2) {
            LOG_F(
                2,
                "%s\n%s",
                mConsensusTimestamp.toString().data(),
                serialization::toJsonString(*mTransaction, true).data()
            );
        }

        // check if transaction already exist
        // if this transaction doesn't belong to us, we can quit here 
        // also if we already have this transaction
        auto fileBasedBlockchain = std::dynamic_pointer_cast<FileBased>(blockchain);
        assert(fileBasedBlockchain);
        if (fileBasedBlockchain->isTransactionExist(mTransaction)) {
            LOG_F(INFO, "Transaction skipped (cached): %s", mConsensusTimestamp.toString().data());
            return 0;
        }

        // if simple validation already failed, we can stop here
        try {
            validate::Context validator(*mTransaction);
            validator.run(validate::Type::SINGLE);
        }
        catch (GradidoBlockchainException& e) {
            LOG_F(ERROR, e.getFullString().data());
            notificateFailedTransaction(blockchain, e.what());
            return 0;
        }

        auto lastTransaction = blockchain->findOne(Filter::LAST_TRANSACTION);
        if (lastTransaction && lastTransaction->getConfirmedTransaction()->getConfirmedAt() > mConsensusTimestamp) {
            // this transaction seems to be from the past, a transaction which happen after this was already added
            notificateFailedTransaction(blockchain, "Transaction skipped (from past)");
            return 0;
        }

        // hand over to OrderingManager
        //std::clog << "transaction: " << std::endl << transaction->getJson() << std::endl;
        // OrderingManager::getInstance()->pushTransaction(mTransaction, mConsensusTimestamp, mCommunityId);
        // blockchain->createAndAddConfirmedTransaction(mTransaction, std::make_shared<memory::Block>(0), mConsensusTimestamp.getAsTimepoint());
        mSuccess = true;
        static_cast<gradido::blockchain::FileBased*>(blockchain.get())->getOrderingManager()->condSignal();
        return 0;
    }

    void HieroMessageToTransactionTask::notificateFailedTransaction(
        std::shared_ptr<gradido::blockchain::Abstract> blockchain,
        const std::string& errorMessage
    )
    {
        LOG_F(INFO, "%s: %s", errorMessage.data(), mConsensusTimestamp.toString().data());
        if (blockchain) {
            auto communityServer = std::dynamic_pointer_cast<gradido::blockchain::FileBased>(blockchain)->getListeningCommunityServer();
            if (communityServer) {
                communityServer->notificateFailedTransaction(*mTransaction, errorMessage, mConsensusTimestamp.toString());
            }
        }
    }
}
