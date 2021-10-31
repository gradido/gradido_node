#include "GradidoBlock.h"
#include "sodium.h"

#include "../../lib/BinTextConverter.h"
#include "../../lib/DataTypeConverter.h"

#include "Poco/DateTimeFormatter.h"
//#include "Poco/Exception.h"


#include <unordered_map>

#include <google/protobuf/text_format.h>

namespace model {

	GradidoBlock::GradidoBlock(const std::string& transactionBinString, Poco::SharedPtr<controller::Group> groupRoot)
		: mGradidoTransaction(nullptr)
	{
		if (0 == transactionBinString.size() || "" == transactionBinString) {
			addError(new Error(__FUNCTION__, "parameter empty"));
			return;
		}
		mGroupRoot = groupRoot;
		Poco::Mutex::ScopedLock lock(mWorkingMutex);
		if (!mProtoGradidoBlock.ParseFromString(transactionBinString)) {
			addError(new Error(__FUNCTION__, "invalid transaction binary string"));
			return;
		}
		mGradidoTransaction = new GradidoTransaction(mProtoGradidoBlock.transaction().SerializeAsString(), groupRoot);
		if (mGradidoTransaction->errorCount() > 0) {
			mGradidoTransaction->getErrors(this);
			mGradidoTransaction->release();
			mGradidoTransaction = nullptr;
		}
		mGradidoTransaction->setGradidoBlock(this);
		//duplicate();
	}

	GradidoBlock::GradidoBlock(uint32_t receivedSeconds, std::string iotaMessageId, Poco::AutoPtr<model::GradidoTransaction> gradidoTransaction, Poco::AutoPtr<GradidoBlock> previousTransaction)
	{
		Poco::Mutex::ScopedLock lock(mWorkingMutex);
		auto mm = MemoryManager::getInstance();

		uint64_t id = 1;
		if (!previousTransaction.isNull()) {
			id = previousTransaction->getID() + 1;
		}
		mProtoGradidoBlock.set_id(id);
		auto timestampSeconds = mProtoGradidoBlock.mutable_received();
		timestampSeconds->set_seconds(receivedSeconds);
		auto mutableMessageId = mProtoGradidoBlock.mutable_message_id();
		*mutableMessageId = iotaMessageId;
		mProtoGradidoBlock.set_version_number(GRADIDO_PROTOCOL_VERSION);
		auto runningHash = mProtoGradidoBlock.mutable_running_hash();
		
		auto protoTransaction = mProtoGradidoBlock.mutable_transaction();
		protoTransaction->CopyFrom(gradidoTransaction->getProto());

		mGradidoTransaction = gradidoTransaction.get();
		mGradidoTransaction->duplicate();

		setGroupRoot(mGradidoTransaction->getGroupRoot());
		mGradidoTransaction->setGradidoBlock(this);

		auto txHash = calculateTxHash(previousTransaction);
		*runningHash = std::string((const char*)txHash->data(), txHash->size());
		mm->releaseMemory(txHash);
	}

	void GradidoBlock::setGroupRoot(Poco::SharedPtr<controller::Group> groupRoot)
	{
		mGroupRoot = groupRoot;
		if (mGradidoTransaction) {
			mGradidoTransaction->setGroupRoot(groupRoot);
		}
	}

	GradidoBlock::~GradidoBlock()
	{
		if (mGradidoTransaction) {
			mGradidoTransaction->release();
			mGradidoTransaction = nullptr;
		}
	}

	bool GradidoBlock::validate(TransactionValidationLevel level)
	{
		if (mGradidoTransaction) {
			return mGradidoTransaction->validate(level);
		}
		else {
			addError(new Error(__FUNCTION__, "invalid gradido transaction"));
			return false;
		}	

		return true;
	}

	bool GradidoBlock::validate(Poco::AutoPtr<GradidoBlock> previousTransaction)
	{
		auto mm = MemoryManager::getInstance();
		auto prevTxHash = previousTransaction->getTxHash();

		auto hash = calculateTxHash(previousTransaction);

		if (memcmp(*hash, mProtoGradidoBlock.running_hash().data(), hash->size()) != 0) {
			addError(new Error(__FUNCTION__, "txhash error"));
			mm->releaseMemory(hash);
			return false;
		}
		mm->releaseMemory(hash);
		return true;
	}

	MemoryBin* GradidoBlock::calculateTxHash(Poco::AutoPtr<GradidoBlock> previousTransaction)
	{
		auto mm = MemoryManager::getInstance();
		std::string prevTxHash;
		if (!previousTransaction.isNull()) {
			prevTxHash = previousTransaction->getTxHash();
		}

		std::string transactionIdString = std::to_string(mProtoGradidoBlock.id());
		std::string receivedString;

		//yyyy-MM-dd HH:mm:ss
		try {
			Poco::DateTime received(Poco::Timestamp(mProtoGradidoBlock.received().seconds() * 1000000));
			receivedString = Poco::DateTimeFormatter::format(received, "%Y-%m-%f %H:%M:%S");
		}
		catch (Poco::Exception& ex) {
			addError(new Error(__FUNCTION__, "error invalid received time"));
			addError(new ParamError(__FUNCTION__, "", ex.displayText()));
			return nullptr;
		}

		std::string signatureMapString = mProtoGradidoBlock.transaction().sig_map().SerializeAsString();

		auto hash = mm->getFreeMemory(crypto_generichash_BYTES);

		// Sodium use for the generichash function BLAKE2b today (11.11.2019), mabye change in the future
		crypto_generichash_state state;
		crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);
		if (prevTxHash.size()) {
			auto prexHashHex = convertBinToHex(prevTxHash);
			printf("[GradidoBlock::calculateTxHash] calculate with prev tx hash: %s", prexHashHex.data());
			crypto_generichash_update(&state, (const unsigned char*)prevTxHash.data(), prevTxHash.size());
		}
		crypto_generichash_update(&state, (const unsigned char*)transactionIdString.data(), transactionIdString.size());
		crypto_generichash_update(&state, (const unsigned char*)receivedString.data(), receivedString.size());
		crypto_generichash_update(&state, (const unsigned char*)signatureMapString.data(), signatureMapString.size());
		crypto_generichash_final(&state, *hash, hash->size());
		return hash;
	}
}