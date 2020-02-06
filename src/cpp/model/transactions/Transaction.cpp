#include "Transaction.h"

namespace model {

	Transaction::Transaction(const std::string& transactionBinString)
	{
		if (0 == transactionBinString.size() || "" == transactionBinString) {
			addError(new Error(__FUNCTION__, "parameter empty"));
			return;
		}
		Poco::Mutex::ScopedLock lock(mWorkingMutex);
		if (!mProtoTransaction.ParseFromString(transactionBinString)) {
			addError(new Error(__FUNCTION__, "invalid transaction binary string"));
			return;
		}
	}

	bool Transaction::validate()
	{

	}

}