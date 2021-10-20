#include "GradidoTransaction.h"

namespace model {
    GradidoTransaction::GradidoTransaction(const std::string& transactionBinString)
    : mTransactionBody(nullptr)
    {
        if (0 == transactionBinString.size() || "" == transactionBinString) {
			addError(new Error(__FUNCTION__, "parameter empty"));
			return;
		}
		Poco::Mutex::ScopedLock lock(mWorkingMutex);
		if (!mProtoGradidoTransaction.ParseFromString(transactionBinString)) {
			addError(new Error(__FUNCTION__, "invalid transaction binary string"));
			return;
		}
		mTransactionBody = new TransactionBody(mProtoGradidoTransaction.body_bytes(), mProtoGradidoBlock.sig_map());
		if (mTransactionBody->errorCount() > 0) {
			mTransactionBody->getErrors(this);
			delete mTransactionBody;
			mTransactionBody = nullptr;
		}
		mTransactionBody->setParent(this);
		duplicate();
    }
    GradidoTransaction::~GradidoTransaction()
    {
        if (mTransactionBody) {
			delete mTransactionBody;
			mTransactionBody = nullptr;
		}
    }

    bool GradidoTransaction::validate(TransactionValidationLevel level /*= TRANSACTION_VALIDATION_SINGLE*/)
    {   
        if(!mTransactionBody) {
            return false;
        }
        return mTransactionBody->validate(level);
    }		

}