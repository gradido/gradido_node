#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_TRANSFER_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_TRANSFER_H

#include "TransactionBase.h"

#include "gradido/GradidoTransfer.pb.h"
#include "gradido/BasicTypes.pb.h"

namespace model {
	class TransactionTransfer : public TransactionBase
	{
	public:
		TransactionTransfer(const proto::gradido::GradidoTransfer& transfer,
			const proto::gradido::SignatureMap& sigMap);

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);
		std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer);
	
	protected:
		const proto::gradido::LocalTransfer getTransfer();

		const proto::gradido::GradidoTransfer& mProtoTransfer;
		const proto::gradido::SignatureMap& mSignatureMap;
	};
}

#endif // __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_TRANSFER_H