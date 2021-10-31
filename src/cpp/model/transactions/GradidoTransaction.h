#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_GRADIDO_TRANSACTION_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_GRADIDO_TRANSACTION_H

#include "TransactionBase.h"
#include "TransactionBody.h"
#include "Poco/DateTime.h"
#include "gradido/GradidoTransaction.pb.h"



namespace model {
	class GradidoTransaction : public TransactionBase
	{
	public:
		GradidoTransaction(const std::string& transactionBinString, Poco::SharedPtr<controller::Group> parent);
		~GradidoTransaction();

		inline TransactionBody* getTransactionBody() { return mTransactionBody; }
		inline std::string getSerialized() { return mProtoGradidoTransaction.SerializeAsString(); }
		std::string getJson();
		const proto::gradido::GradidoTransaction& getProto() const { return mProtoGradidoTransaction; }

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);

		inline std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer) {
			return mTransactionBody->getInvolvedAddressIndices(addressIndexContainer);
		}

		void setGroupRoot(Poco::SharedPtr<controller::Group> groupRoot);

	protected:
		proto::gradido::GradidoTransaction mProtoGradidoTransaction;
		TransactionBody* mTransactionBody;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_GRADIDO_TRANSACTION_H
