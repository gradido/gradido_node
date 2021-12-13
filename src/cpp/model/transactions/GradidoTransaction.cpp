#include "GradidoTransaction.h"
#include <google/protobuf/util/json_util.h>
#include "sodium.h"

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
    }
    GradidoTransaction::~GradidoTransaction()
    {
        if (mTransactionBody) {
			delete mTransactionBody;
			mTransactionBody = nullptr;
		}
    }

	void GradidoTransaction::setGroupRoot(Poco::SharedPtr<controller::Group> groupRoot)
	{
		mGroupRoot = groupRoot;
		if (mTransactionBody) {
			mTransactionBody->setGroupRoot(groupRoot);
		}
	}
	void GradidoTransaction::setGradidoBlock(GradidoBlock* gradidoBlock)
	{
		mGradidoBlock = gradidoBlock;
		if (mTransactionBody) {
			mTransactionBody->setGradidoBlock(gradidoBlock);
		}
	}

    bool GradidoTransaction::validate(TransactionValidationLevel level /*= TRANSACTION_VALIDATION_SINGLE*/)
    {
		// check signatures
		auto sigmap = mProtoGradidoTransaction.sig_map();
		auto sigPairs = sigmap.sigpair();
		auto bodyBytes = mProtoGradidoTransaction.body_bytes();
		std::unordered_map<std::string, std::string> pubkeys_map;

		for (auto it = sigPairs.begin(); it != sigPairs.end(); it++) {
			auto pubkey = it->pubkey();

			if (pubkeys_map.find(pubkey) != pubkeys_map.end()) {
				addError(new Error(__FUNCTION__, "duplicate pubkey in signatures"));
				return false;
			}
			pubkeys_map.insert(std::pair<std::string, std::string>(pubkey, pubkey));
			auto signature = it->signature();

			/*std::string hex_bodyBytes = convertBinToHex(bodyBytes);
			//printf("body bytes try to verify signature: \n%s\n", hex_bodyBytes.data());
			std::string protoPrettyPrint;
			google::protobuf::TextFormat::PrintToString(mProtoTransaction, &protoPrettyPrint);
			printf("transaction pretty: \n%s\n", protoPrettyPrint.data());
			proto::gradido::TransactionBody transactionBody;
			transactionBody.MergeFromString(mProtoTransaction.bodybytes());
			google::protobuf::TextFormat::PrintToString(transactionBody, &protoPrettyPrint);
			printf("transaction body pretty: \n%s\n", protoPrettyPrint.data());
			*/
			if (0 != crypto_sign_verify_detached(
				(const unsigned char*)signature.data(),
				(const unsigned char*)bodyBytes.data(), bodyBytes.size(),
				(const unsigned char*)pubkey.data())) {
				addError(new Error(__FUNCTION__, "invalid signature"));
				return false;
			}
		}


		if (mTransactionBody) {
			return mTransactionBody->validate(level);
		}
		else {
			addError(new Error(__FUNCTION__, "invalid transaction body"));
			return false;
		}
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
