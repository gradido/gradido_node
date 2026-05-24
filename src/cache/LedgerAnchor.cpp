#include "LedgerAnchor.h"
#include "../ServerGlobals.h"
#include "../SystemExceptions.h"
#include "../lib/LevelDBExceptions.h"
#include "gradido_blockchain/data/LedgerAnchor.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "gradido_blockchain/interaction/serialize/Context.h"
#include "gradido_blockchain/memory/Block.h"

#include "loguru/loguru.hpp"
using memory::Block;
using std::shared_ptr, std::make_shared;

namespace cache {
	LedgerAnchor::LedgerAnchor(std::string_view folder)
		: mInitalized(false),
		mLevelDBFile(folder),
		mLedgerAnchorTransactionNrs(ServerGlobals::g_CacheTimeout)
	{

	}
	LedgerAnchor::~LedgerAnchor()
	{

	}

	// try to open db 
	bool LedgerAnchor::init(size_t cacheInBytes)
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

	void LedgerAnchor::exit()
	{
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, cache::MessageId aren't stored in leveldb file, or exit was called more than once");
		}
		mInitalized = false;
		mLevelDBFile.exit();
	}

	//! remove state level db folder, clear maps
	void LedgerAnchor::reset()
	{
		mLevelDBFile.reset();
		mLedgerAnchorTransactionNrs.clear();
	}

	void LedgerAnchor::add(const gradido::data::LedgerAnchor& transactionId, uint64_t transactionNr)
	{
		auto ledgerAnchorSerialized = toProtobuf(transactionId);

		if (mLedgerAnchorTransactionNrs.get(ledgerAnchorSerialized).has_value()) {
			throw GradidoAlreadyExist("cache::LedgerAnchor already has key!");
		}
		mLedgerAnchorTransactionNrs.add(ledgerAnchorSerialized, transactionNr);
		if (mInitalized) {
			mLevelDBFile.setKeyValue(ledgerAnchorSerialized, std::to_string(transactionNr).data());
		}
	}

	bool LedgerAnchor::has(const gradido::data::LedgerAnchor& transactionId)
	{
		auto ledgerAnchorSerialized = toProtobuf(transactionId);
		if (mLedgerAnchorTransactionNrs.get(ledgerAnchorSerialized).has_value()) {
			return true;
		}
		return readFromLevelDb(ledgerAnchorSerialized) != 0;
	}

	uint64_t LedgerAnchor::getTransactionNrForLedgerAnchor(const gradido::data::LedgerAnchor& transactionId)
	{
		auto ledgerAnchorSerialized = toProtobuf(transactionId);
		auto result = mLedgerAnchorTransactionNrs.get(ledgerAnchorSerialized);
		if (result.has_value()) {
			return result.value();
		}
		return readFromLevelDb(ledgerAnchorSerialized);
	}

	uint64_t LedgerAnchor::readFromLevelDb(const std::string& ledgerAnchorSerialized)
	{
		if (mInitalized) {
			auto result = mLevelDBFile.getValueForKey(ledgerAnchorSerialized);
			if (result.has_value()) 
			{
				auto transactionNr = std::stoull(result.value());
				mLedgerAnchorTransactionNrs.add(ledgerAnchorSerialized, transactionNr);
				return transactionNr;
			}
		}
		return 0;
	}

	gradido::data::LedgerAnchor LedgerAnchor::fromProtobuf(const std::string transactionIdString) const
	{
		auto binTransactionId = make_shared<Block>(transactionIdString);
		gradido::interaction::deserialize::Context deserializer(binTransactionId, gradido::interaction::deserialize::Type::LEDGER_ANCHOR);
		deserializer.run();
		if (!deserializer.isLedgerAnchor()) {
			throw GradidoNodeInvalidDataException("[cache::LedgerAnchor::fromProtobuf] transactionId cannot be deserialized as gradido::data::LedgerAnchor");
		}
		return deserializer.getLedgerAnchor();
	}

	std::string LedgerAnchor::toProtobuf(const gradido::data::LedgerAnchor& transactionId) const
	{
		gradido::interaction::serialize::Context serializer(transactionId);
		return serializer.run()->copyAsString();
	}

}