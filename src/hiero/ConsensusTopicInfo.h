#ifndef __GRADIDO_NODE_HIERO_CONSENSUS_TOPIC_INFO_H
#define __GRADIDO_NODE_HIERO_CONSENSUS_TOPIC_INFO_H

#include "Key.h"
#include "gradido_blockchain/memory/Block.h"
#include "gradido_blockchain/data/Timestamp.h"
#include "gradido_blockchain/data/DurationSeconds.h"
#include "gradido_blockchain/data/hiero/AccountId.h"

#include <string>
#include <memory>

namespace hiero {

    using DurationMessage = message<
        int64_field<"seconds", 1>
    >;

    using ConsensusTopicInfoMessage = message<
        string_field<"memo", 1>,
        bytes_field<"runningHash", 2>,
        uint64_field<"sequenceNumber", 3>,
        message_field<"expirationTime", 4, gradido::interaction::deserialize::TimestampMessage>,
        message_field<"adminKey", 5, KeyMessage>,
        message_field <"submitKey", 6, KeyMessage>,
        message_field<"autoRenewPeriod", 7, DurationMessage>,
        message_field<"autoRenewAccount", 8, gradido::interaction::deserialize::HieroAccountIdMessage>,
        bytes_field<"ledger_id", 9>
    >;

    class ConsensusTopicInfo 
    {
    public:
        ConsensusTopicInfo();
        ConsensusTopicInfo(const ConsensusTopicInfoMessage& message);
        ~ConsensusTopicInfo();

        // getter
        const std::string& getMemo() const { return mMemo; }
        memory::ConstBlockPtr getRunningHash() const { return mRunningHash; }
        uint64_t getSequenceNumber() const { return mSequenceNumber; }
        const gradido::data::Timestamp& getExpirationTime() const { return mExpirationTime; }
        const Key& getAdminKey() const { return mAdminKey; }
        const Key& getSubmitKey() const { return mSubmitKey; }
        const gradido::data::DurationSeconds& getAutoRenewPeriod() const { return mAutoRenewPeriod; }
        const AccountId& getAutoRenewAccountId() const { return mAutoRenewAccountID; }
        memory::ConstBlockPtr getLedgerId() const { return mLedgerId; }

    private:
        std::string mMemo;
        memory::ConstBlockPtr  mRunningHash;
        uint64_t mSequenceNumber;
        gradido::data::Timestamp mExpirationTime;
        Key mAdminKey;
        Key mSubmitKey;
        gradido::data::DurationSeconds mAutoRenewPeriod;
        AccountId mAutoRenewAccountID;
        memory::ConstBlockPtr mLedgerId;
        // ignore custom fees for the time beeing
        
    };
}

#endif //__GRADIDO_NODE_HIERO_CONSENSUS_TOPIC_INFO_H