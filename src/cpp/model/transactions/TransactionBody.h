#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BODY_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BODY_H

#include "TransactionBase.h"
#include "TransactionCreation.h"
#include "TransactionTransfer.h"
#include "gradido/TransactionBody.pb.h"

namespace model {

	enum TransactionType {
		TRANSACTION_NONE,
		TRANSACTION_CREATION,
		TRANSACTION_TRANSFER
	};

	class TransactionBody : public TransactionBase
	{
	public:
		TransactionBody(const std::string& transactionBinString, const proto::gradido::SignatureMap& signatureMap);
		~TransactionBody();

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);
		void setGroupRoot(Poco::SharedPtr<controller::Group> groupRoot);
		void setGradidoBlock(GradidoBlock* gradidoBlock);

		inline TransactionType getType() { return mTransactionType; }
		inline TransactionCreation* getCreation() { return dynamic_cast<TransactionCreation*>(mTransactionSpecific); }
		inline TransactionTransfer* getTransfer() { return dynamic_cast<TransactionTransfer*>(mTransactionSpecific); }

		std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer) {
			if (mTransactionSpecific) return mTransactionSpecific->getInvolvedAddressIndices(addressIndexContainer);
			return std::vector<uint32_t>();
		}
		inline proto::gradido::TransactionBody* getProto() {return &mProtoTransactionBody;}


	protected:
		proto::gradido::TransactionBody mProtoTransactionBody;

		TransactionBase* mTransactionSpecific;
		TransactionType mTransactionType;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_BODY_H
