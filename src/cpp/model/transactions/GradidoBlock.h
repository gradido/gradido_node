#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_H

#include "TransactionBase.h"
#include "TransactionBody.h"
#include "Poco/DateTime.h"
#include "gradido/GradidoBlock.pb.h"



namespace model {
	class GradidoBlock : public TransactionBase
	{
	public:
		GradidoBlock(const std::string& transactionBinString);
		~GradidoBlock();

		inline int getID() const { return mProtoGradidoBlock.id(); }
		inline const std::string& getTxHash() const { return mProtoGradidoBlock.running_hash(); }
		inline Poco::DateTime getReceived() const { return Poco::Timestamp(mProtoGradidoBlock.received().seconds()); }

		inline TransactionBody* getTransactionBody() { return mTransactionBody; }
		inline std::string getSerialized() { return mProtoGradidoBlock.SerializeAsString(); }

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);
		bool validate(Poco::AutoPtr<GradidoBlock> previousTransaction);

		inline std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer) {
			return mTransactionBody->getInvolvedAddressIndices(addressIndexContainer);
		}
	protected:
		proto::gradido::GradidoBlock mProtoGradidoBlock;
		TransactionBody* mTransactionBody;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_TRANSACTION_H