#ifndef __GRADIDO_NODE_HIERO_TRANSACTION_RECEIPT_H
#define __GRADIDO_NODE_HIERO_TRANSACTION_RECEIPT_H

#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/data/hiero/AccountId.h"
#include "gradido_blockchain/data/hiero/TopicId.h"
#include "ResponseCodeEnum.h"

namespace hiero {
	// is only a part of it, but luckly the protobuf definition allows us to only partly deserialize what we really need
	using TransactionReceiptMessage = message<
		enum_field<"status", 1, ResponseCodeEnum>,
		message_field<"accountID", 2, gradido::interaction::deserialize::HieroAccountIdMessage>,
		// skip fields in between because currently not needed in gradido node
		message_field<"topicID", 6, gradido::interaction::deserialize::HieroTopicIdMessage>,
		uint64_field<"topicSequenceNumber", 7>,
		bytes_field<"topicRunningHash", 8>,
		uint64_field<"topicRunningHashVersion", 9>
	>;

	class TransactionReceipt
	{
	public:
		TransactionReceipt();
		TransactionReceipt(const TransactionReceiptMessage& message);
		~TransactionReceipt();

		TransactionReceiptMessage getMessage() const;
		
		inline ResponseCodeEnum getStatus() const { return mStatus; }
		inline const AccountId& getAccountID() const { return mAccountID; }
		inline const TopicId& getTopicID() const { return mTopicID; }
		inline uint64_t getTopicSequenceNumber() const { return mTopicSequenceNumber; }
		inline memory::ConstBlockPtr getTopicRunningHash() const { return mTopicRunningHash; }
		inline uint64_t getTopicRunningHashVersion() const { return mTopicRunningHashVersion; }

	protected:
		ResponseCodeEnum mStatus;
		AccountId mAccountID;
		TopicId mTopicID;
		uint64_t mTopicSequenceNumber;
		memory::ConstBlockPtr mTopicRunningHash;
		uint64_t mTopicRunningHashVersion;
	};
}
#endif //__GRADIDO_NODE_HIERO_TRANSACTION_RECEIPT_H