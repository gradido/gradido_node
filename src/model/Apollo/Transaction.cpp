#include "Transaction.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "magic_enum/magic_enum.hpp"
#include "loguru/loguru.hpp"

using namespace rapidjson;
using namespace gradido::interaction;
using namespace magic_enum;

namespace model {
	namespace Apollo {


		Transaction::Transaction(const gradido::data::ConfirmedTransaction& confirmedTransaction, memory::ConstBlockPtr pubkey)
			: mType(TransactionType::NONE), mId(0), mDecay(nullptr)
		{			
			auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
			auto transactionBody = gradidoTransaction->getTransactionBody();
			
			if (transactionBody->isCreation()) {
				mType = TransactionType::CREATE;
				auto creation = transactionBody->getCreation();
				mAmount = creation->getRecipient().getAmount();
				mFirstName = "Gradido";
				mLastName = "Akademie";
				mPubkey = gradidoTransaction->getSignatureMap().getSignaturePairs().front().getPublicKey()->convertToHex();
			}
			else if (
				transactionBody->isTransfer() 
				|| transactionBody->isDeferredTransfer() 
				|| transactionBody->isRedeemDeferredTransfer()
				) {
				auto transfer = getTransfer(transactionBody);
				bool isLink = transactionBody->isDeferredTransfer() || transactionBody->isRedeemDeferredTransfer();
				mAmount = transfer.getSender().getAmount();
				if (transfer.getRecipient()->isTheSame(pubkey)) {
					mType = isLink ? TransactionType::LINK_RECEIVE : TransactionType::RECEIVE;
					mPubkey = transfer.getSender().getPublicKey()->convertToHex();
				}
				else if (transfer.getSender().getPublicKey()->isTheSame(pubkey)) {
					mType = isLink ? TransactionType::LINK_SEND : TransactionType::SEND;
					mPubkey = transfer.getRecipient()->convertToHex();
					mAmount.negate();
				}
			}
			else {
				LOG_F(ERROR, "not implemented yet: %s", enum_name(transactionBody->getTransactionType()).data());
				throw std::runtime_error("transaction type not implemented yet");
			}
			
			auto& memos = transactionBody->getMemos();
			for (auto& memo : memos) {
				if (memo.getKeyType() == gradido::data::MemoKeyType::PLAIN) {
					mMemo = memo.getMemo()->copyAsString();
					return;
				}
			}
			if (mMemo.empty()) {
				if (memos.size()) {
					mMemo = "memo is encrypted";
				}
			}
			mId = confirmedTransaction.getId();
			mDate = confirmedTransaction.getConfirmedAt();
			mBalance = confirmedTransaction.getAccountBalance(pubkey).getBalance();
		}

		Transaction::Transaction(Timepoint decayStart, Timepoint decayEnd, GradidoUnit startBalance)
			: mType(TransactionType::DECAY), mId(-1), mDate(decayEnd), mDecay(nullptr)
		{
		
			calculateDecay(decayStart, decayEnd, startBalance);
			mAmount = mDecay->getDecayAmount();
			mBalance = startBalance + mDecay->getDecayAmount();
			mPreviousBalance = startBalance;
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

		Transaction::Transaction(Transaction&& parent) noexcept
			: mType(parent.mType),
			mAmount(parent.mAmount),
			mBalance(parent.mBalance),
			mPreviousBalance(parent.mPreviousBalance),
			mPubkey(std::move(parent.mPubkey)),
			mFirstName(std::move(parent.mFirstName)),
			mLastName(std::move(parent.mLastName)),
			mMemo(std::move(parent.mMemo)),
			mId(parent.mId),
			mDate(parent.mDate),
			mDecay(parent.mDecay)
		{
			parent.mDecay = nullptr;
		}
		Transaction::Transaction(const Transaction& parent)
			: mType(parent.mType),
			mAmount(parent.mAmount),
			mBalance(parent.mBalance),
			mPreviousBalance(parent.mPreviousBalance),
			mPubkey(parent.mPubkey),
			mFirstName(parent.mFirstName),
			mLastName(parent.mLastName),
			mMemo(parent.mMemo),
			mId(parent.mId),
			mDate(parent.mDate),
			mDecay(nullptr)	
		{
			if (parent.mDecay) {
				mDecay = new Decay(parent.mDecay);
			}
		}

		Transaction::~Transaction()
		{
			if (mDecay) {
				delete mDecay;
				mDecay = nullptr;
			}
		}

		Transaction& Transaction::operator=(const Transaction& other)
		{
			// Self-assignment check
			if (this == &other) {
				return *this;
			}

			// Kopiere einfache Member
			mType = other.mType;
			mAmount = other.mAmount;
			mBalance = other.mBalance;
			mPreviousBalance = other.mPreviousBalance;
			mPubkey = other.mPubkey;
			mFirstName = other.mFirstName;
			mLastName = other.mLastName;
			mMemo = other.mMemo;
			mId = other.mId;
			mDate = other.mDate;

			// Speicher von mDecay richtig verwalten
			if (mDecay) {
				delete mDecay;  // Alten Speicher freigeben
				mDecay = nullptr;
			}

			// Wenn other.mDecay nicht null ist, erstelle eine neue Kopie
			if (other.mDecay) {
				mDecay = new Decay(*other.mDecay);
			}

			return *this;
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
			transaction.AddMember("id", mId, alloc);
			transaction.AddMember("typeId", Value(enum_name(mType).data(), alloc), alloc);
			transaction.AddMember("amount", Value(mAmount.toString(2).data(), alloc), alloc);
			transaction.AddMember("balance", Value(mBalance.toString(2).data(), alloc), alloc);
			transaction.AddMember("previousBalance", Value(mPreviousBalance.toString(2).data(), alloc), alloc);
			transaction.AddMember("memo", Value(mMemo.data(), alloc), alloc);
			
			Value linkedUser(kObjectType);
			linkedUser.AddMember("pubkey", Value(mPubkey.data(), alloc), alloc);
			linkedUser.AddMember("firstName", Value(mFirstName.data(), alloc), alloc);
			linkedUser.AddMember("lastName", Value(mLastName.data(), alloc), alloc);
			linkedUser.AddMember("__typename", "User", alloc);
			transaction.AddMember("linkedUser", linkedUser, alloc);

			transaction.AddMember("balanceDate", Value(formatJsCompatible(mDate).data(), alloc), alloc);
			if (mDecay) {
				transaction.AddMember("decay", mDecay->toJson(alloc), alloc);
			}
			transaction.AddMember("__typename", "Transaction", alloc);
			return std::move(transaction);
		}

		const gradido::data::GradidoTransfer& Transaction::getTransfer(gradido::data::ConstTransactionBodyPtr transactionBody)
		{
			if (transactionBody->isTransfer()) {
				return *transactionBody->getTransfer();
			} else if (transactionBody->isDeferredTransfer()) {
				return transactionBody->getDeferredTransfer()->getTransfer();
			} else if (transactionBody->isRedeemDeferredTransfer()) {
				return transactionBody->getRedeemDeferredTransfer()->getTransfer();
			}
			auto type = transactionBody->getTransactionType();
			throw GradidoUnhandledEnum(
				"getTransfer is not implemented",
				enum_type_name<decltype(type)>().data(),
				enum_name(type).data()
			);
		}
	}
}