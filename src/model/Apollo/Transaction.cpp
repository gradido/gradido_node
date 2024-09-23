#include "Transaction.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

using namespace rapidjson;
using namespace gradido::interaction;

namespace model {
	namespace Apollo {


		Transaction::Transaction(const gradido::data::ConfirmedTransaction& confirmedTransaction, memory::ConstBlockPtr pubkey)
			: mFirstTransaction(false)
		{			
			auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
			auto transactionBody = gradidoTransaction->getTransactionBody();
			
			if (transactionBody->getTransactionType() == gradido::data::TransactionType::CREATION) {
				mType = TRANSACTION_TYPE_CREATE;
				auto creation = transactionBody->getCreation();
				mAmount = creation->getRecipient().getAmount();
				mFirstName = "Gradido";
				mLastName = "Akademie";
				mPubkey = gradidoTransaction->getSignatureMap().getSignaturePairs().front().getPubkey()->convertToHex();
			}
			else if (transactionBody->getTransactionType() == gradido::data::TransactionType::TRANSFER) {
				auto transfer = transactionBody->getTransfer();
				mAmount = transfer->getSender().getAmount();
				if(transfer->getRecipient()->isTheSame(pubkey)) {
					mType = TRANSACTION_TYPE_RECEIVE;
					mPubkey = transfer->getSender().getPubkey()->convertToHex();
				}
				else if (transfer->getSender().getPubkey()->isTheSame(pubkey)) {
					mType = TRANSACTION_TYPE_SEND;
					mPubkey = transfer->getRecipient()->convertToHex();
					mAmount *= -1.0;
				}				
			}
			else if (transactionBody->getTransactionType() == gradido::data::TransactionType::DEFERRED_TRANSFER) {
				auto transfer = transactionBody->getDeferredTransfer()->getTransfer();
				mAmount = transfer.getSender().getAmount();
				if (transfer.getRecipient()->isTheSame(pubkey)) {
					mType = TRANSACTION_TYPE_RECEIVE;
					mPubkey = transfer.getSender().getPubkey()->convertToHex();
				}
				else if (transfer.getSender().getPubkey()->isTheSame(pubkey)) {
					mType = TRANSACTION_TYPE_SEND;
					mPubkey = transfer.getRecipient()->convertToHex();
					mAmount *= -1.0;
				}
			}
			else {
				throw std::runtime_error("transaction type not implemented yet");
			}
			mMemo = transactionBody->getMemo();
			mId = confirmedTransaction.getId();
			mDate = confirmedTransaction.getConfirmedAt();
		}

		Transaction::Transaction(Timepoint decayStart, Timepoint decayEnd, GradidoUnit startBalance)
			: mType(TRANSACTION_TYPE_DECAY), mId(-1), mDate(decayEnd), mFirstTransaction(false)
		{
		
			calculateDecay(decayStart, decayEnd, startBalance);
			mAmount = mDecay->getDecayAmount();
			mBalance = startBalance + mDecay->getDecayAmount();
		}

		/*
		* TransactionType mType;
			mpfr_ptr        mAmount;
			std::string     mName;
			std::string		mMemo;
			uint64_t		mTransactionNr;
			Timepoint mDate;
			Decay*			mDecay;
			bool			mFirstTransaction;
		*/

		Transaction::Transaction(Transaction&& parent)
			: mType(parent.mType),
			  mAmount(parent.mAmount),
		      mBalance(parent.mBalance),
			  mPubkey(std::move(parent.mPubkey)),
			  mFirstName(std::move(parent.mFirstName)),
			  mLastName(std::move(parent.mLastName)),
			  mMemo(std::move(parent.mMemo)),
			  mId(parent.mId),
			  mDate(parent.mDate),
			  mDecay(parent.mDecay),
			  mFirstTransaction(parent.mFirstTransaction)
		{
			parent.mDecay = nullptr;
		}

		Transaction::~Transaction()
		{
			if (mDecay) {
				delete mDecay;
				mDecay = nullptr;
			}
		}

		void Transaction::calculateDecay(Timepoint decayStart, Timepoint decayEnd, GradidoUnit startBalance)
		{
			if (mDecay) {
				delete mDecay;
				mDecay = nullptr;
			}
			mDecay = new Decay(decayStart, decayEnd, startBalance);
		}

		void Transaction::setBalance(GradidoUnit balance)
		{
			mBalance = balance;
		}

		Value Transaction::toJson(Document::AllocatorType& alloc)
		{
			Value transaction(kObjectType);
			transaction.AddMember("typeId", Value(transactionTypeToString(mType), alloc), alloc);
			transaction.AddMember("amount", Value(mAmount.toString().data(), alloc), alloc);
			transaction.AddMember("balance", Value(mBalance.toString().data(), alloc), alloc);
			transaction.AddMember("memo", Value(mMemo.data(), alloc), alloc);
			transaction.AddMember("id", mId, alloc);
			Value linkedUser(kObjectType);
			linkedUser.AddMember("pubkey", Value(mPubkey.data(), alloc), alloc);
			linkedUser.AddMember("firstName", Value(mFirstName.data(), alloc), alloc);
			linkedUser.AddMember("lastName", Value(mLastName.data(), alloc), alloc);
			linkedUser.AddMember("__typename", "User", alloc);
			transaction.AddMember("linkedUser", linkedUser, alloc);

			auto dateString = DataTypeConverter::timePointToString(mDate, jsDateTimeFormat);
			transaction.AddMember("balanceDate", Value(dateString.data(), alloc), alloc);
			if (mDecay) {
				transaction.AddMember("decay", mDecay->toJson(alloc), alloc);
			}
			transaction.AddMember("firstTransaction", mFirstTransaction, alloc);
			transaction.AddMember("__typename", "Transaction", alloc);
			return std::move(transaction);
		}

		const char* Transaction::transactionTypeToString(TransactionType type)
		{
			switch (type) {
			case TRANSACTION_TYPE_CREATE: return "CREATE";
			case TRANSACTION_TYPE_SEND: return "SEND";
			case TRANSACTION_TYPE_RECEIVE: return "RECEIVE";
			case TRANSACTION_TYPE_DECAY: return "DECAY";
			}
			return "<unknown>";
		}

		void Transaction::setFirstTransaction(bool firstTransaction) 
		{ 
			mFirstTransaction = firstTransaction; 
			setBalance(mAmount);
		}

	}
}