#include "TransactionReceipt.h"
#include "gradido_blockchain/interaction/deserialize/HieroAccountIdRole.h"
#include "gradido_blockchain/interaction/deserialize/HieroTopicIdRole.h"
#include "gradido_blockchain/interaction/serialize/HieroAccountIdRole.h"
#include "gradido_blockchain/interaction/serialize/HieroTopicIdRole.h"

using namespace gradido::interaction;

namespace hiero {
	TransactionReceipt::TransactionReceipt()
		: mStatus(hiero::ResponseCodeEnum::UNKNOWN), mTopicSequenceNumber(0), mTopicRunningHashVersion(0)
	{

	}

	TransactionReceipt::TransactionReceipt(const TransactionReceiptMessage& message)
	{
		mStatus = message["status"_f].value();
		if (message["accountID"_f].has_value()) {
			mAccountID = deserialize::HieroAccountIdRole(message["accountID"_f].value());
		}
		if (message["topicID"_f].has_value()) {
			mTopicID = deserialize::HieroTopicIdRole(message["topicID"_f].value());
		}
		if (message["topicSequenceNumber"_f].has_value()) {
			mTopicSequenceNumber = message["topicSequenceNumber"_f].value();
		}
		if (message["topicRunningHash"_f].has_value()) {
			mTopicRunningHash = std::make_shared<const memory::Block>(message["topicRunningHash"_f].value());
		}
		if (message["topicRunningHashVersion"_f].has_value()) {
			mTopicRunningHashVersion = message["topicRunningHashVersion"_f].value();
		}
	}
	TransactionReceipt::~TransactionReceipt()
	{

	}

	TransactionReceiptMessage TransactionReceipt::getMessage() const
	{
		TransactionReceiptMessage message;
		message["status"_f] = mStatus;
		if (!mAccountID.empty()) {
			message["accountID"_f] = serialize::HieroAccountIdRole(mAccountID).getMessage();
		}
		if (!mTopicID.empty()) {
			message["topicID"_f] = serialize::HieroTopicIdRole(mTopicID).getMessage(); 
		}
		if (mTopicSequenceNumber) {
			message["topicSequenceNumber"_f] = mTopicSequenceNumber;
		}
		if (mTopicRunningHash) {
			message["topicRunningHash"_f] = mTopicRunningHash->copyAsVector();
		}
		if (mTopicRunningHashVersion) {
			message["topicRunningHashVersion"_f] = mTopicRunningHashVersion;
		}
		return message;
	}
	
}