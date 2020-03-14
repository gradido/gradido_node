#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_TRANSFER_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_TRANSFER_H

#include "TransactionBase.h"

#include "../../proto/gradido/Transfer.pb.h"
#include "../../proto/gradido/BasicTypes.pb.h"

namespace model {
	class TransactionTransfer : public TransactionBase
	{
	public:
		TransactionTransfer(const model::messages::gradido::Transfer& transfer,
			const model::messages::gradido::SignatureMap& sigMap);

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);
		std::vector<uint32_t> getInvolvedAddressIndices(controller::AddressIndex* addressIndexContainer);
	
	protected:

		const model::messages::gradido::Transfer& mProtoTransfer;
		const model::messages::gradido::SignatureMap& mSignatureMap;
	};
}

#endif // __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_TRANSFER_H