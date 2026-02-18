#include "gradido_blockchain/data/ConfirmedTransaction.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/interaction/deserialize/Type.h"
#include "gradido_blockchain/GradidoBlockchainException.h"

#include "DeserializeConfirmedTransactionTask.h"
#include "../ServerGlobals.h"

#include <loguru/loguru.hpp>

using gradido::data::ConfirmedTransaction;
using std::shared_ptr;
using Deserializer = gradido::interaction::deserialize::Context;
using DeserializerType = gradido::interaction::deserialize::Type;

namespace task {
    DeserializeConfirmedTransactionTask::DeserializeConfirmedTransactionTask(memory::ConstBlockPtr rawTransaction, uint32_t communityIdIndex)
        : CPUTask(ServerGlobals::g_CPUScheduler), mRawTransaction(rawTransaction), mCommunityIdIndex(communityIdIndex) {
    }
    
    DeserializeConfirmedTransactionTask::~DeserializeConfirmedTransactionTask() {
    }
    
    int DeserializeConfirmedTransactionTask::run() {
        Deserializer deserializer(mRawTransaction, DeserializerType::CONFIRMED_TRANSACTION);
        try {
            deserializer.run(mCommunityIdIndex);
        }
        catch (std::exception& e) {
            int zahl = 0;
            LOG_F(ERROR, "error on deserialize: %s", e.what());
            return -1;
        }
        if (!deserializer.isConfirmedTransaction()) {
            throw InvalidGradidoTransaction("invalid confirmed transaction", mRawTransaction);
        }
        mConfirmedTransaction = deserializer.getConfirmedTransaction();
        return 0;
    }
}
