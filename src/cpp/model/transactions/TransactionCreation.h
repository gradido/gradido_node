#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_CREATION_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_CREATION_H

#include "TransactionBase.h"
#include "gradido/GradidoCreation.pb.h"
#include "gradido/BasicTypes.pb.h"

namespace model {
	class TransactionCreation : public TransactionBase
	{
	public:
		TransactionCreation(const proto::gradido::GradidoCreation& transaction,
						    const proto::gradido::SignatureMap& sigMap);

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);

		inline const std::string& getRecipiantPubkey() const { return mProtoCreation.recipiant().pubkey(); }
		inline google::protobuf::int64 getRecipiantAmount() const { return mProtoCreation.recipiant().amount(); }

		std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer);


	protected:
		const proto::gradido::GradidoCreation& mProtoCreation;
		const proto::gradido::SignatureMap& mSignatureMap;
	};
}

#endif // __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_CREATION_H