#include "Transaction.h"
#include "sodium.h"
#include "../../SingletonManager/MemoryManager.h"

#include "../../lib/BinTextConverter.h"

#include "Poco/DateTimeFormatter.h"
//#include "Poco/Exception.h"

#include <unordered_map>

#include <google/protobuf/text_format.h>

namespace model {

	Transaction::Transaction(const std::string& transactionBinString)
		: mTransactionBody(nullptr)
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
		mTransactionBody = new TransactionBody(mProtoTransaction.bodybytes(), mProtoTransaction.sigmap());
		if (mTransactionBody->errorCount() > 0) {
			mTransactionBody->getErrors(this);
			delete mTransactionBody;
			mTransactionBody = nullptr;
		}
		mTransactionBody->setParent(this);
		duplicate();
	}

	Transaction::~Transaction()
	{
		if (mTransactionBody) {
			delete mTransactionBody;
			mTransactionBody = nullptr;
		}
	}

	bool Transaction::validate(TransactionValidationLevel level)
	{
		// check signatures
		auto sigmap = mProtoTransaction.sigmap();
		auto sigPairs = sigmap.sigpair();
		auto bodyBytes = mProtoTransaction.bodybytes();
		std::unordered_map<std::string, std::string> pubkeys_map;

		for (auto it = sigPairs.begin(); it != sigPairs.end(); it++) {
			auto pubkey = it->pubkey();
			
			if (pubkeys_map.find(pubkey) != pubkeys_map.end()) {
				addError(new Error(__FUNCTION__, "duplicate pubkey in signatures"));
				return false;
			}
			pubkeys_map.insert(std::pair<std::string, std::string>(pubkey, pubkey));
			auto signature = it->ed25519();

			/*std::string hex_bodyBytes = convertBinToHex(bodyBytes);
			//printf("body bytes try to verify signature: \n%s\n", hex_bodyBytes.data());
			std::string protoPrettyPrint;
			google::protobuf::TextFormat::PrintToString(mProtoTransaction, &protoPrettyPrint);
			printf("transaction pretty: \n%s\n", protoPrettyPrint.data());
			model::messages::gradido::TransactionBody transactionBody;
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
			return mTransactionBody->validate();
		}
		else {
			addError(new Error(__FUNCTION__, "invalid transaction body"));
			return false;
		}

	

		return true;
	}

	bool Transaction::validate(Poco::AutoPtr<Transaction> previousTransaction)
	{
		auto mm = MemoryManager::getInstance();
		auto prevTxHash = previousTransaction->getTxHash();

		std::string transactionIdString = std::to_string(mProtoTransaction.id());
		std::string receivedString;

		//yyyy-MM-dd HH:mm:ss
		try {
			Poco::DateTime received(Poco::Timestamp(mProtoTransaction.received().seconds() * 1000000));
			receivedString = Poco::DateTimeFormatter::format(received, "%Y-%m-%f %H:%M:%S");
		}
		catch (Poco::Exception& ex) {
			addError(new Error(__FUNCTION__, "error invalid received time"));
			addError(new ParamError(__FUNCTION__, "", ex.displayText()));
			return false;
		}

		std::string signatureMapString = mProtoTransaction.sigmap().SerializeAsString();

		auto hash = mm->getFreeMemory(crypto_generichash_BYTES);

		// Sodium use for the generichash function BLAKE2b today (11.11.2019), mabye change in the future
		crypto_generichash_state state;
		crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);
		crypto_generichash_update(&state, (const unsigned char*)prevTxHash.data(), prevTxHash.size());
		crypto_generichash_update(&state, (const unsigned char*)transactionIdString.data(), transactionIdString.size());
		crypto_generichash_update(&state, (const unsigned char*)receivedString.data(), receivedString.size());
		crypto_generichash_update(&state, (const unsigned char*)signatureMapString.data(), signatureMapString.size());
		crypto_generichash_final(&state, *hash, hash->size());

		if (memcmp(*hash, mProtoTransaction.txhash().data(), hash->size()) != 0) {
			addError(new Error(__FUNCTION__, "txhash error"));
			mm->releaseMemory(hash);
			return false;
		}
		mm->releaseMemory(hash);
		return true;
	}
}