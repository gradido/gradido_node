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
			Transaction(Poco::Timestamp decayStart, Poco::Timestamp decayEnd, const mpfr_ptr startBalance);

			// Move Constrcutor
			Transaction(Transaction&& parent);
			~Transaction();

			void calculateDecay(Poco::Timestamp decayStart, Poco::Timestamp decayEnd, const mpfr_ptr startBalance);
			void setBalance(mpfr_ptr balance);

			rapidjson::Value toJson(rapidjson::Document::AllocatorType& alloc);

			static const char* transactionTypeToString(TransactionType type);

			void setFirstTransaction(bool firstTransaction);
			inline bool isFirstTransaction() const { return mFirstTransaction; }

			inline Poco::Timestamp getDate() const { return mDate; }
			inline const mpfr_ptr getAmount() const { return mAmount; }
			inline const Decay* getDecay() const { return mDecay; }

		protected:

			TransactionType mType;
			mpfr_ptr        mAmount;
			mpfr_ptr		mBalance;
			std::string     mName;
			std::string		mMemo;
			int64_t			mId;
			Poco::Timestamp mDate;
			Decay*			mDecay;
			bool			mFirstTransaction;
		private: 
			// Disable copy constructor
			Transaction(const Transaction&) {};
		};
	}
}

#endif // __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H