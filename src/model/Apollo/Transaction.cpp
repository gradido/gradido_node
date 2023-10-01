#include "Transaction.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"

using namespace rapidjson;

namespace model {
	namespace Apollo {


		Transaction::Transaction(const model::gradido::ConfirmedTransaction* gradidoBlock, const std::string& pubkey)
			: mAmount(nullptr), mBalance(nullptr), mDecay(nullptr), mFirstTransaction(false)
		{			
			auto mm = MemoryManager::getInstance();
			mBalance = mm->getMathMemory();
			mAmount = mm->getMathMemory();
			auto gradidoTransaction = gradidoBlock->getGradidoTransaction();
			auto transactionBody = gradidoTransaction->getTransactionBody();
			
			if (transactionBody->getTransactionType() == model::gradido::TRANSACTION_CREATION) {
				mType = TRANSACTION_TYPE_CREATE;
				auto creation = transactionBody->getCreationTransaction();
				if (mpfr_set_str(mAmount, creation->getAmount().data(), 10, gDefaultRound)) {
					throw model::gradido::TransactionValidationInvalidInputException("amount cannot be parsed to a number", "amount", "string");
				}
				mFirstName = "Gradido";
				mLastName = "Akademie";
				auto pubkeys = gradidoTransaction->getPublicKeysfromSignatureMap(true);
				mPubkey = DataTypeConverter::binToHex(pubkeys.front());
				for (auto it = pubkeys.begin(); it != pubkeys.end(); it++) {
					mm->releaseMemory(*it);
				}
				pubkeys.clear();
			}
			else if (transactionBody->getTransactionType() == model::gradido::TRANSACTION_TRANSFER ||
				transactionBody->getTransactionType() == model::gradido::TRANSACTION_DEFERRED_TRANSFER) {
				auto transfer = transactionBody->getTransferTransaction();
				if (mpfr_set_str(mAmount, transfer->getAmount().data(), 10, gDefaultRound)) {
					throw model::gradido::TransactionValidationInvalidInputException("amount cannot be parsed to a number", "amount", "string");
				}
				if (transfer->getRecipientPublicKeyString() == pubkey) {
					mType = TRANSACTION_TYPE_RECEIVE;
					mPubkey = DataTypeConverter::binToHex(transfer->getSenderPublicKeyString());
				}
				else if (transfer->getSenderPublicKeyString() == pubkey) {
					mType = TRANSACTION_TYPE_SEND;
					mPubkey = DataTypeConverter::binToHex(transfer->getRecipientPublicKeyString());
					mpfr_neg(mAmount, mAmount, gDefaultRound);
				}				
			}
			else {
				throw std::runtime_error("transaction type not implemented yet");
			}
			mMemo = transactionBody->getMemo();
			mId = gradidoBlock->getID();
			mDate = gradidoBlock->getReceivedAsTimestamp();
		}

		Transaction::Transaction(Poco::Timestamp decayStart, Poco::Timestamp decayEnd, const mpfr_ptr startBalance)
			: mType(TRANSACTION_TYPE_DECAY), mAmount(nullptr), mBalance(nullptr), mId(-1), mDate(decayEnd), mDecay(nullptr), mFirstTransaction(false)
		{
			auto mm = MemoryManager::getInstance();
			mBalance = mm->getMathMemory();
			mAmount = mm->getMathMemory();
			calculateDecay(decayStart, decayEnd, startBalance);
			mpfr_set(mAmount, mDecay->getDecayAmount(), gDefaultRound);
			mpfr_add(mBalance, startBalance, mDecay->getDecayAmount(), gDefaultRound);
		}

		/*
		* TransactionType mType;
			mpfr_ptr        mAmount;
			std::string     mName;
			std::string		mMemo;
			uint64_t		mTransactionNr;
			Poco::Timestamp mDate;
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
			parent.mAmount = nullptr;
			parent.mBalance = nullptr;
		}

		Transaction::~Transaction()
		{
			if (mDecay) {
				delete mDecay;
				mDecay = nullptr;
			}
			if (mAmount) {
				MemoryManager::getInstance()->releaseMathMemory(mAmount);
				mAmount = nullptr;
			}
			if (mBalance) {
				MemoryManager::getInstance()->releaseMathMemory(mBalance);
				mBalance = nullptr;
			}
		}

		void Transaction::calculateDecay(Poco::Timestamp decayStart, Poco::Timestamp decayEnd, const mpfr_ptr startBalance)
		{
			if (mDecay) {
				delete mDecay;
				mDecay = nullptr;
			}
			mDecay = new Decay(decayStart, decayEnd, startBalance);
		}

		void Transaction::setBalance(mpfr_ptr balance)
		{
			mpfr_set(mBalance, balance, gDefaultRound);
		}

		Value Transaction::toJson(Document::AllocatorType& alloc)
		{
			Value transaction(kObjectType);
			transaction.AddMember("typeId", Value(transactionTypeToString(mType), alloc), alloc);
			std::string tempString;
			model::gradido::TransactionBase::amountToString(&tempString, mAmount);
			transaction.AddMember("amount", Value(tempString.data(), alloc), alloc);
			tempString.clear();
			model::gradido::TransactionBase::amountToString(&tempString, mBalance);
			transaction.AddMember("balance", Value(tempString.data(), alloc), alloc);
			transaction.AddMember("memo", Value(mMemo.data(), alloc), alloc);
			transaction.AddMember("id", mId, alloc);
			Value linkedUser(kObjectType);
			linkedUser.AddMember("pubkey", Value(mPubkey.data(), alloc), alloc);
			linkedUser.AddMember("firstName", Value(mFirstName.data(), alloc), alloc);
			linkedUser.AddMember("lastName", Value(mLastName.data(), alloc), alloc);
			linkedUser.AddMember("__typename", "User", alloc);
			transaction.AddMember("linkedUser", linkedUser, alloc);

			auto dateString = Poco::DateTimeFormatter::format(mDate, jsDateTimeFormat);
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