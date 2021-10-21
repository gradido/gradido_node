#include "GradidoTransaction.h"
#include <google/protobuf/util/json_util.h>

namespace model {
    GradidoTransaction::GradidoTransaction(const std::string& transactionBinString, Poco::SharedPtr<controller::Group> groupRoot)
    : mTransactionBody(nullptr)
    {
        if (0 == transactionBinString.size() || "" == transactionBinString) {
			addError(new Error(__FUNCTION__, "parameter empty"));
			return;
		}
		mGroupRoot = groupRoot;

		Poco::Mutex::ScopedLock lock(mWorkingMutex);
		if (!mProtoGradidoTransaction.ParseFromString(transactionBinString)) {
			addError(new Error(__FUNCTION__, "invalid transaction binary string"));
			return;
		}
		mTransactionBody = new TransactionBody(mProtoGradidoTransaction.body_bytes(), mProtoGradidoTransaction.sig_map());
		if (mTransactionBody->errorCount() > 0) {
			mTransactionBody->getErrors(this);
			delete mTransactionBody;
			mTransactionBody = nullptr;
		}
		mTransactionBody->setGroupRoot(groupRoot);
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

	std::string GradidoTransaction::getJson()
	{
		static const char* function_name = "GradidoTransaction::getJson";
		std::string json;
		google::protobuf::util::JsonPrintOptions options;
		options.add_whitespace = true;
		options.always_print_primitive_fields = true;

		auto status = google::protobuf::util::MessageToJsonString(*mTransactionBody->getProto(), &json, options);
		if (!status.ok()) {
			addError(new ParamError(function_name, "error by parsing transaction body message to json", status.ToString()));
			return "";
		}
		return json;
	}

}
