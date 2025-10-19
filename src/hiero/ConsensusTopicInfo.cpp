#include "gradido_blockchain/interaction/deserialize/TimestampRole.h"
#include "ConsensusTopicInfo.h"
#include "gradido_blockchain/interaction/deserialize/HieroAccountIdRole.h"

using namespace gradido::interaction;

namespace hiero {
	ConsensusTopicInfo::ConsensusTopicInfo()
		: mSequenceNumber(0)
	{

	}
	
	ConsensusTopicInfo::ConsensusTopicInfo(const ConsensusTopicInfoMessage& message)
	{
		if (message["memo"_f].has_value()) {
			mMemo = message["memo"_f].value();
		}
		if (message["runningHash"_f].has_value()) {
			mRunningHash = std::make_shared<const memory::Block>(message["runningHash"_f].value());
		}
		if (message["sequenceNumber"_f].has_value()) {
			mSequenceNumber = message["sequenceNumber"_f].value();
		}
		mExpirationTime = deserialize::TimestampRole(message["expirationTime"_f].value());
		if (message["adminKey"_f].has_value()) {
			mAdminKey = Key(message["adminKey"_f].value());
		}
		if (message["submitKey"_f].has_value()) {
			mSubmitKey = Key(message["submitKey"_f].value());
		}
		if (message["autoRenewPeriod"_f].has_value()) {
			const auto& duration = message["autoRenewPeriod"_f].value();
			if (duration["seconds"_f].has_value()) {
				mAutoRenewPeriod = Duration(duration["seconds"_f].value());
			}
		}
		if (message["autoRenewAccount"_f].has_value()) {
			mAutoRenewAccountID = deserialize::HieroAccountIdRole(message["autoRenewAccount"_f].value());
		}
		if (message["ledger_id"_f].has_value()) {
			mLedgerId = std::make_shared<const memory::Block>(message["ledger_id"_f].value());
		}
	}

	ConsensusTopicInfo::~ConsensusTopicInfo()
	{

	}
}