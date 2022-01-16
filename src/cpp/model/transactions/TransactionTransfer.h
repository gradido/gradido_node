#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_TRANSFER_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_TRANSFER_H

#include "TransactionBase.h"

#include "gradido/GradidoTransfer.pb.h"
#include "gradido/BasicTypes.pb.h"

// MAGIC NUMBER: time to wait for missing pairing transaction by cross group transactions
// iota make ca. every minute a new milestone so 2 minutes should be more than enough
// as long we wait, we block the processing of further transactions
// we shouldn't wait to long, because hacker can use this to easily block the node (for the specific group) with invalid cross group transactions
// we shouldn't wait to short because a delay in Iota can break or block chain 
#define MAGIC_NUMBER_TRANSFER_CROSS_GROUP_WAIT_ON_PAIR_SECONDS 10 //60 * 2
// MAGIC NUMBER: we don't need to wait all to long between calls, because the check call is really fast: mutex lock + map lookup 
#define MAGIC_NUMBER_TRANSFER_CROSS_GROUP_WAIT_ON_PAIR_SLEEPTIME_MILLISECONDS 150

namespace model {
	class TransactionTransfer : public TransactionBase
	{
	public:
		TransactionTransfer(const proto::gradido::GradidoTransfer& transfer,
			const proto::gradido::SignatureMap& sigMap);

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);
		std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer);

		inline bool isCrossGroupTransfer() const { return mProtoTransfer.has_inbound() || mProtoTransfer.has_outbound(); }
		inline bool isLocal() const { return mProtoTransfer.has_local(); }
		inline bool isInbound() const { return mProtoTransfer.has_inbound(); }
		inline bool isOutbound() const { return mProtoTransfer.has_outbound(); }

		std::string getOtherGroup() const;
		Poco::Timestamp getPairedTransactionId() const;
		// for checking if a paired transaction belong really to this transaction (same amount, sender and recipiant, other group)
		bool isBelongTo(const TransactionTransfer* paired) const;

		int64_t getGradidoDeltaForUser(const std::string& pubkey);
	
	protected:
		const proto::gradido::LocalTransfer getTransfer() const;
		const proto::gradido::CrossGroupTransfer getCrossGroupTransfer() const;

		const proto::gradido::GradidoTransfer& mProtoTransfer;
		const proto::gradido::SignatureMap& mSignatureMap;
	};
}

#endif // __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_TRANSFER_H