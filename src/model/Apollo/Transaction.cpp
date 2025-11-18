#include "Transaction.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/blockchain/Abstract.h"

#include "magic_enum/magic_enum.hpp"
#include "loguru/loguru.hpp"

using namespace rapidjson;
using namespace gradido;
using namespace interaction;
using namespace blockchain;
using namespace magic_enum;

namespace model {
	namespace Apollo {
		Transaction::Transaction(
				const data::ConfirmedTransaction& confirmedTransaction,
				memory::ConstBlockPtr pubkey
				) : mType(TransactionType::NONE), mId(0), mDecay(nullptr), mHasChange(false)
		{
			auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
			auto transactionBody = gradidoTransaction->getTransactionBody();

			auto& memos = transactionBody->getMemos();
			for (auto& memo : memos) {
				if (memo.getKeyType() == gradido::data::MemoKeyType::PLAIN) {
					mMemo = memo.getMemo().copyAsString();
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
			mBalance = confirmedTransaction.getAccountBalance(pubkey, "").getBalance();
			printf("getBalance: %s for id: %lu\n", mBalance.toString().data(), mId);
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
			mDecay(parent.mDecay),
			mHasChange(parent.mHasChange),
			mChangeAmount(parent.mChangeAmount),
			mChangePubkey(std::move(parent.mChangePubkey))
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
			mDecay(nullptr),
			mHasChange(parent.mHasChange),
			mChangeAmount(parent.mChangeAmount),
			mChangePubkey(parent.mChangePubkey)
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
			mHasChange = other.mHasChange;
			mChangeAmount = other.mChangeAmount;
			mChangePubkey = other.mChangePubkey;

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

			if (!mPubkey.empty() || !mFirstName.empty() || !mLastName.empty()) {
				Value linkedUser(kObjectType);
				linkedUser.AddMember("pubkey", Value(mPubkey.data(), alloc), alloc);
				linkedUser.AddMember("firstName", Value(mFirstName.data(), alloc), alloc);
				linkedUser.AddMember("lastName", Value(mLastName.data(), alloc), alloc);
				linkedUser.AddMember("__typename", "User", alloc);
				transaction.AddMember("linkedUser", linkedUser, alloc);
			} else {
				transaction.AddMember("linkedUser", Value(kNullType), alloc);
			}
			if(mHasChange) {
				Value changeObj(kObjectType);
				printf("Transaction::toJson adding change amount: %s, pubkey: %s\n", mChangeAmount.toString().data(), mChangePubkey.data());
				changeObj.AddMember("amount", Value(mChangeAmount.toString(2).data(), alloc), alloc);
				changeObj.AddMember("pubkey", Value(mChangePubkey.data(), alloc), alloc);
				changeObj.AddMember("__typename", "Change", alloc);
				transaction.AddMember("change", changeObj, alloc);
			} else {
				transaction.AddMember("change", Value(kNullType), alloc);
			}

			transaction.AddMember("balanceDate", Value(formatJsCompatible(mDate).data(), alloc), alloc);
			if (mDecay) {
				transaction.AddMember("decay", mDecay->toJson(alloc), alloc);
			}
			transaction.AddMember("__typename", "Transaction", alloc);
			return std::move(transaction);
		}
	}
}