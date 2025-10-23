#include "HieroTransactionId.h"
#include "../ServerGlobals.h"
#include "../SystemExceptions.h"
#include "../lib/LevelDBExceptions.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"

#include "loguru/loguru.hpp"

namespace cache {
	HieroTransactionId::HieroTransactionId(std::string_view folder)
		: mInitalized(false),
		mLevelDBFile(folder),
		mHieroTransactionIdTransactionNrs(ServerGlobals::g_CacheTimeout)
	{

	}
	HieroTransactionId::~HieroTransactionId()
	{

	}

	// try to open db 
	bool HieroTransactionId::init(size_t cacheInBytes)
	{
		if (mInitalized) {
			throw ClassAlreadyInitalizedException("init was already called", "cache::MessageId");
		}
		if (!mLevelDBFile.init(cacheInBytes)) {
			return false;
		}
		mInitalized = true;
		return true;
	}

	void HieroTransactionId::exit()
	{
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, cache::MessageId aren't stored in leveldb file, or exit was called more than once");
		}
		mInitalized = false;
		mLevelDBFile.exit();
	}

	//! remove state level db folder, clear maps
	void HieroTransactionId::reset()
	{
		mLevelDBFile.reset();
		mHieroTransactionIdTransactionNrs.clear();
	}

	void HieroTransactionId::add(memory::ConstBlockPtr transactionId, uint64_t transactionNr)
	{
		auto hieroTransactionId = fromProtobuf(transactionId);
		if (mHieroTransactionIdTransactionNrs.get(hieroTransactionId).has_value()) {
			throw GradidoAlreadyExist("cache::HieroTransactionId already has key!");
		}
		mHieroTransactionIdTransactionNrs.add(hieroTransactionId, transactionNr);
		if (mInitalized) {
			mLevelDBFile.setKeyValue(transactionId->convertToBase64().data(), std::to_string(transactionNr).data());
		}
	}

	bool HieroTransactionId::has(memory::ConstBlockPtr transactionId)
	{
		auto hieroTransactionId = fromProtobuf(transactionId);
		if (mHieroTransactionIdTransactionNrs.get(hieroTransactionId).has_value()) {
			return true;
		}
		return readFromLevelDb(transactionId, hieroTransactionId) != 0;
	}

	uint64_t HieroTransactionId::getTransactionNrForHieroTransactionId(memory::ConstBlockPtr transactionId)
	{
		auto hieroTransactionId = fromProtobuf(transactionId);
		auto result = mHieroTransactionIdTransactionNrs.get(hieroTransactionId);
		if (result.has_value()) {
			return result.value();
		}
		return readFromLevelDb(transactionId, hieroTransactionId);
	}

	uint64_t HieroTransactionId::readFromLevelDb(memory::ConstBlockPtr transactionId, hiero::TransactionId transactionIdObj)
	{
		if (mInitalized) {
			std::string value;
			if (mLevelDBFile.getValueForKey(transactionId->convertToBase64().data(), &value)) {
				auto transactionNr = std::stoull(value);
				mHieroTransactionIdTransactionNrs.add(transactionIdObj, transactionNr);
				return transactionNr;
			}
		}
		return 0;
	}

	hiero::TransactionId HieroTransactionId::fromProtobuf(memory::ConstBlockPtr transactionId) const
	{
		gradido::interaction::deserialize::Context deserializer(transactionId, gradido::interaction::deserialize::Type::HIERO_TRANSACTION_ID);
		deserializer.run();
		if (!deserializer.isHieroTransactionId()) {
			throw GradidoNodeInvalidDataException("[cache::HieroTransactionId::fromProtobuf] transactionId cannot be deserialized as hiero::TransactionId");
		}
		return deserializer.getHieroTransactionId();
	}

}