#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_CREATION_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_CREATION_H

#include "TransactionBase.h"
#include "../../proto/gradido/TransactionCreation.pb.h"
#include "../../proto/gradido/BasicTypes.pb.h"

namespace model {
	class TransactionCreation : public TransactionBase
	{
	public:
		TransactionCreation(const model::messages::gradido::TransactionCreation& transaction,
						    const model::messages::gradido::SignatureMap& sigMap);

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);

		inline const std::string& getReceiverPubkey() const { return mProtoCreation.receiveramount().ed25519_receiver_pubkey(); }
		inline google::protobuf::int64 getReceiverAmount() const { return mProtoCreation.receiveramount().amount(); }

		std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer);


	protected:
		const model::messages::gradido::TransactionCreation& mProtoCreation;
		const model::messages::gradido::SignatureMap& mSignatureMap;
	};
}

#endif // __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_CREATION_H