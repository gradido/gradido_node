#ifndef __GRADIDO_NODE_MODEL_TRANSACTIONS_GRADIDO_BLOCK_H
#define __GRADIDO_NODE_MODEL_TRANSACTIONS_GRADIDO_BLOCK_H

#include "TransactionBase.h"
#include "GradidoTransaction.h"
#include "Poco/DateTime.h"
#include "gradido/GradidoBlock.pb.h"

#include "../../SingletonManager/MemoryManager.h"

#define GRADIDO_PROTOCOL_VERSION 1

namespace model {
	class GradidoBlock : public TransactionBase
	{
	public:
		GradidoBlock(const std::string& transactionBinString, Poco::SharedPtr<controller::Group> groupRoot);
		GradidoBlock(uint32_t receivedSeconds, std::string iotaMessageId, Poco::AutoPtr<model::GradidoTransaction> gradidoTransaction, Poco::AutoPtr<GradidoBlock> previousTransaction);
		~GradidoBlock();

		inline uint64_t getID() const { return mProtoGradidoBlock.id(); }
		inline const std::string& getTxHash() const { return mProtoGradidoBlock.running_hash(); }
		// convert from seconds to microseconds
		inline Poco::DateTime getReceived() const { return Poco::Timestamp(mProtoGradidoBlock.received().seconds() * 1000000); }
		inline int64_t getReceivedSeconds() const { return mProtoGradidoBlock.received().seconds(); }

		inline GradidoTransaction* getGradidoTransaction() { return mGradidoTransaction; }
		inline std::string getSerialized() { return mProtoGradidoBlock.SerializeAsString(); }

		void setGroupRoot(Poco::SharedPtr<controller::Group> groupRoot);

		bool validate(TransactionValidationLevel level = TRANSACTION_VALIDATION_SINGLE);
		bool validate(Poco::AutoPtr<GradidoBlock> previousTransaction);

		inline std::vector<uint32_t> getInvolvedAddressIndices(Poco::SharedPtr<controller::AddressIndex> addressIndexContainer) {
			return mGradidoTransaction->getInvolvedAddressIndices(addressIndexContainer);
		}

		//! \return called must free return value
		MemoryBin* calculateTxHash(Poco::AutoPtr<GradidoBlock> previousTransaction);
	protected:
		

		proto::gradido::GradidoBlock mProtoGradidoBlock;
		GradidoTransaction* mGradidoTransaction;
	};
}

#endif //__GRADIDO_NODE_MODEL_TRANSACTIONS_GRADIDO_BLOCK_H