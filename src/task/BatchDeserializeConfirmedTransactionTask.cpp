#include "gradido_blockchain/data/ConfirmedTransaction.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/interaction/deserialize/Type.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

#include "BatchDeserializeConfirmedTransactionTask.h"
#include "../ServerGlobals.h"

#include "loguru/loguru.hpp"

using gradido::data::ConfirmedTransaction;
using std::shared_ptr;
using Deserializer = gradido::interaction::deserialize::Context;
using DeserializerType = gradido::interaction::deserialize::Type;

namespace task {
    BatchDeserializeConfirmedTransactionTask::BatchDeserializeConfirmedTransactionTask(std::vector<memory::ConstBlockPtr>&& rawTransactions, uint32_t communityIdIndex)
        : CPUTask(ServerGlobals::g_CPUScheduler), mRawTransactions(std::move(rawTransactions)), mCommunityIdIndex(communityIdIndex)
    {
    }
    
    BatchDeserializeConfirmedTransactionTask::~BatchDeserializeConfirmedTransactionTask() 
    {    
    }
    
    int BatchDeserializeConfirmedTransactionTask::run() 
    {
        mConfirmedTransactions.clear();
        mConfirmedTransactions.resize(mRawTransactions.size(), nullptr);
        for (size_t i = 0; i < mRawTransactions.size(); i++) {
            Deserializer deserializer(mRawTransactions[i], DeserializerType::CONFIRMED_TRANSACTION);
            try {
                deserializer.run(mCommunityIdIndex);
            }
            catch (std::exception& e) {
                int zahl = 0;
                LOG_F(ERROR, "error on deserialize: %s", e.what());
                return -1;
            }
            if (!deserializer.isConfirmedTransaction()) {
                throw InvalidGradidoTransaction("invalid confirmed transaction", mRawTransactions[i]);
            }
            mConfirmedTransactions[i] = deserializer.getConfirmedTransaction();
            // printf("%d ", deserializer.getConfirmedTransaction()->getId());
        }        
        return 0;
    }
}
