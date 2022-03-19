#include "Transaction.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"

using namespace rapidjson;

namespace model {
	namespace Apollo {


		Transaction::Transaction(const model::gradido::GradidoBlock* gradidoBlock, const std::string& pubkey)
			: mDecay(nullptr)
		{
			auto gradidoTransaction = gradidoBlock->getGradidoTransaction();
			auto transactionBody = gradidoTransaction->getTransactionBody();
			mBalance = MemoryManager::getInstance()->getMathMemory();
			if (transactionBody->getTransactionType() == model::gradido::TRANSACTION_CREATION) {
				mType = TRANSACTION_TYPE_CREATE;
				auto creation = transactionBody->getCreationTransaction();
				if (mpfr_set_str(mBalance, creation->getAmount().data(), 10, gDefaultRound)) {
					throw model::gradido::TransactionValidationInvalidInputException("amount cannot be parsed to a number", "amount", "string");
				}
				mName = "Gradido Akademie";
			}
			else if (transactionBody->getTransactionType() == model::gradido::TRANSACTION_TRANSFER ||
				transactionBody->getTransactionType() == model::gradido::TRANSACTION_DEFERRED_TRANSFER) {
				auto transfer = transactionBody->getTransferTransaction();
				if (transfer->getRecipientPublicKeyString() == pubkey) {
					mType = TRANSACTION_TYPE_RECEIVE;
					mName = DataTypeConverter::binToHex(transfer->getSenderPublicKeyString());
				}
				else if (transfer->getSenderPublicKeyString() == pubkey) {
					mType = TRANSACTION_TYPE_SEND;
					mName = DataTypeConverter::binToHex(transfer->getRecipientPublicKeyString());
				}
				if (mpfr_set_str(mBalance, transfer->getAmount().data(), 10, gDefaultRound)) {
					throw model::gradido::TransactionValidationInvalidInputException("amount cannot be parsed to a number", "amount", "string");
				}
			}
			else {
				throw std::runtime_error("transaction type not implemented yet");
			}
			mMemo = transactionBody->getMemo();
			mTransactionNr = gradidoBlock->getID();
			mDate = gradidoBlock->getReceivedAsTimestamp();
		}

		Transaction::~Transaction()
		{
			if (mDecay) {
				delete mDecay;
				mDecay = nullptr;
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
			}
			mDecay = new Decay(decayStart, decayEnd, startBalance);
		}

		Value Transaction::toJson(Document::AllocatorType& alloc)
		{
			if (mType == TRANSACTION_TYPE_DECAY) {
				assert(mDecay);
				auto transaction = mDecay->toJson(alloc);
				transaction.AddMember("type", "decay", alloc);
				transaction.AddMember("__typename", "Transaction", alloc);
				return std::move(transaction);
			}
			Value transaction(kObjectType);
			transaction.AddMember("type", Value(transactionTypeToString(mType), alloc), alloc);
			std::string balanceString;
			model::gradido::TransactionBase::amountToString(&balanceString, mBalance);
			transaction.AddMember("balance", Value(balanceString.data(), alloc), alloc);
			transaction.AddMember("memo", Value(mMemo.data(), alloc), alloc);
			transaction.AddMember("transactionId", mTransactionNr, alloc);
			transaction.AddMember("name", Value(mName.data(), alloc), alloc);

			auto dateString = Poco::DateTimeFormatter::format(mDate, "%Y-%m-%dT%H:%M:%S.%i%z");
			transaction.AddMember("date", Value(dateString.data(), alloc), alloc);
			if (mDecay) {
				transaction.AddMember("decay", mDecay->toJson(alloc), alloc);
			}
			transaction.AddMember("__typename", "Transaction", alloc);
			return std::move(transaction);
		}

		const char* Transaction::transactionTypeToString(TransactionType type)
		{
			switch (type) {
			case TRANSACTION_TYPE_CREATE: return "create";
			case TRANSACTION_TYPE_SEND: return "send";
			case TRANSACTION_TYPE_RECEIVE: return "receive";
			case TRANSACTION_TYPE_DECAY: return "decay";
			}
		}
		
	}
}