#ifndef __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H
#define __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H

#include "Decay.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"


namespace model {
	namespace Apollo
	{
		enum TransactionType
		{
			TRANSACTION_TYPE_CREATE,
			TRANSACTION_TYPE_SEND,
			TRANSACTION_TYPE_RECEIVE,
			TRANSACTION_TYPE_DECAY,
		};
		class Transaction
		{
		public:
			Transaction(const model::gradido::GradidoBlock* gradidoBlock, const std::string& pubkey);
			~Transaction();

			void calculateDecay(Poco::Timestamp decayStart, Poco::Timestamp decayEnd, const mpfr_ptr startBalance);

			rapidjson::Value toJson(rapidjson::Document::AllocatorType& alloc);

			static const char* transactionTypeToString(TransactionType type);

		protected:
			TransactionType mType;
			mpfr_ptr        mBalance;
			std::string     mName;
			std::string		mMemo;
			uint64_t		mTransactionNr;
			Poco::Timestamp mDate;
			Decay*			mDecay;
		};
	}
}

#endif // __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H