#include "TransactionBody.h"
#include "TransactionCreation.h"
#include "TransactionTransfer.h"

namespace model {

	TransactionBody::TransactionBody(const std::string& transactionBinString, const proto::gradido::SignatureMap& signatureMap)
		: mTransactionSpecific(nullptr), mTransactionType(TRANSACTION_NONE)
	{
		if (0 == transactionBinString.size() || "" == transactionBinString) {
			addError(new Error(__FUNCTION__, "parameter empty"));
			return;
		}
		Poco::Mutex::ScopedLock lock(mWorkingMutex);
		if (!mProtoTransactionBody.ParseFromString(transactionBinString)) {
			addError(new Error(__FUNCTION__, "invalid transaction binary string"));
			return;
		}
		if (mProtoTransactionBody.has_creation()) {
			mTransactionSpecific = new TransactionCreation(mProtoTransactionBody.creation(), signatureMap);
			mTransactionType = TRANSACTION_CREATION;
		}
		else if (mProtoTransactionBody.has_transfer()) {
			mTransactionSpecific = new TransactionTransfer(mProtoTransactionBody.transfer(), signatureMap);
			mTransactionType = TRANSACTION_TRANSFER;
		}
	}

	TransactionBody::~TransactionBody()
	{
		if (mTransactionSpecific) {
			delete mTransactionSpecific;
			mTransactionSpecific = nullptr;
		}
	}

	bool TransactionBody::validate(TransactionValidationLevel level)
	{
		if (mProtoTransactionBody.memo().size() > 150) {
			addError(new Error(__FUNCTION__, "memo is to big"));
			return false;
		}
		if (!mTransactionSpecific) {
			addError(new Error(__FUNCTION__, "error no specific transaction found/recognized"));
			return false;
		}
		if (!mTransactionSpecific->validate(level)) {
			getErrors(mTransactionSpecific);
			return false;
		}


		return true;
	}

	void TransactionBody::setGroupRoot(Poco::SharedPtr<controller::Group> groupRoot)
	{
		mGroupRoot = groupRoot;
		if (mTransactionSpecific) {
			mTransactionSpecific->setGroupRoot(groupRoot);
			//parent->duplicate();
		}
	}

	void TransactionBody::setGradidoBlock(GradidoBlock* gradidoBlock)
	{
		mGradidoBlock = gradidoBlock;
		if (mTransactionSpecific) {
			mTransactionSpecific->setGradidoBlock(gradidoBlock);
		}
	}
}
