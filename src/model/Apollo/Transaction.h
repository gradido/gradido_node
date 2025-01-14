#ifndef __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H
#define __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H

#include "Decay.h"

#include <string_view>

namespace gradido {
	namespace blockchain {
		class Abstract;
	}
	namespace data {
		class ConfirmedTransaction;
	}
}

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
			LINK_CHARGE,
			LINK_DELETE,
			LINK_CHANGE,
			LINK_TIMEOUT
		};
		class Transaction
		{
		public:
			Transaction(
				const gradido::data::ConfirmedTransaction& confirmedTransaction, 
				memory::ConstBlockPtr pubkey
			);
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
			inline bool hasChange() const { return mHasChange; }

			inline void setType(TransactionType type) { mType = type; }
			inline void setAmount(GradidoUnit amount) { mAmount = amount; }
			inline void setPubkey(std::shared_ptr<const memory::Block> publicKey) { mPubkey = publicKey->convertToHex(); }
			inline void setFirstName(std::string_view firstName) { mFirstName = firstName; }
			inline void setLastName(std::string_view lastName) { mFirstName = lastName; }

		protected:

			TransactionType mType;
			GradidoUnit     mAmount;
			GradidoUnit		  mBalance;
			GradidoUnit     mPreviousBalance;
			std::string     mPubkey;
			std::string		  mFirstName;
			std::string		  mLastName;
			std::string		  mMemo;
			int64_t			    mId;
			Timepoint       mDate;
			Decay*			    mDecay;
			bool	     			mHasChange;
		private:
			
		};
	}
}

#endif // __GRADIDO_NODE_MODEL_APOLLO_TRANSACTION_H