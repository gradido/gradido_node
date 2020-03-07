#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BODY_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BODY_H

#include "TransactionBase.h"
#include "TransactionCreation.h"
#include "TransactionTransfer.h"
#include "../../proto/gradido/TransactionBody.pb.h"

namespace model {

	enum TransactionType {
		TRANSACTION_NONE,
		TRANSACTION_CREATION,
		TRANSACTION_TRANSFER
	};

	class TransactionBody : public TransactionBase
	{
	public:
		TransactionBody(const std::string& transactionBinString, const model::messages::gradido::SignatureMap& signatureMap);
		~TransactionBody();

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);
		void setParent(Transaction* parent);

		inline TransactionType getType() { return mTransactionType; }
		inline TransactionCreation* getCreation() { return dynamic_cast<TransactionCreation*>(mTransactionSpecific); }
		inline TransactionTransfer* getTransfer() { return dynamic_cast<TransactionTransfer*>(mTransactionSpecific); }

	protected:
		model::messages::gradido::TransactionBody mProtoTransactionBody;

		TransactionBase* mTransactionSpecific;
		TransactionType mTransactionType;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BODY_H
