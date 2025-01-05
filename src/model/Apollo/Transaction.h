#ifndef __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H
#define __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H

#include "Decay.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"

namespace model {
	namespace Apollo
	{
		enum class TransactionType
		{
			NONE,
			CREATE,
			SEND,
			RECEIVE,
			DECAY,
			LINK_SEND,
			LINK_RECEIVE,
			LINK_DELETE
		};
		class Transaction
		{
		public:
			Transaction(const gradido::data::ConfirmedTransaction& confirmedTransaction, memory::ConstBlockPtr pubkey);
			Transaction(Timepoint decayStart, Timepoint decayEnd, GradidoUnit startBalance);

			// Move constrcutor
			Transaction(Transaction&& parent) noexcept;
			// Copy constructor
			Transaction(const Transaction& parent);
			~Transaction();

			Transaction& operator=(const Transaction& other);  // copy

			void calculateDecay(Timepoint decayStart, Timepoint decayEnd, GradidoUnit startBalance);
			void setBalance(GradidoUnit balance);
			inline GradidoUnit getBalance() const {return mBalance;}
			inline void setPreviousBalance(GradidoUnit previousBalance) {mPreviousBalance = previousBalance;}

			rapidjson::Value toJson(rapidjson::Document::AllocatorType& alloc);

			inline Timepoint getDate() const { return mDate; }
			inline const GradidoUnit getAmount() const { return mAmount; }
			inline const Decay* getDecay() const { return mDecay; }

		protected:

			TransactionType mType;
			GradidoUnit     mAmount;
			GradidoUnit		mBalance;
			GradidoUnit   mPreviousBalance;
			std::string     mPubkey;
			std::string		mFirstName;
			std::string		mLastName;
			std::string		mMemo;
			int64_t			mId;
			Timepoint       mDate;
			Decay*			mDecay;
		private:
			
		};
	}
}

#endif // __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H