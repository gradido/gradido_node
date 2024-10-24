#include "MessageId.h"
#include "../ServerGlobals.h"
#include "../SystemExceptions.h"
#include "../lib/LevelDBExceptions.h"

#include "loguru/loguru.hpp"

namespace cache {
	MessageId::MessageId(std::string_view folder)
		: mInitalized(false),
		mStateFile(folder),
		mMessageIdTransactionNrs(ServerGlobals::g_CacheTimeout)
	{

	}
	MessageId::~MessageId()
	{

	}

	// try to open db 
	bool MessageId::init()
	{
		if (mInitalized) {
			throw ClassAlreadyInitalizedException("init was already called", "cache::MessageId");
		}
		if (!mStateFile.init()) {
			return false;
		}
		mInitalized = true;
		return true;
	}

	void MessageId::exit()
	{
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, cache::MessageId aren't stored in leveldb file, or exit was called more than once");
		}
		mInitalized = false;
		mStateFile.exit();
	}

	//! remove state level db folder, clear maps
	void MessageId::reset()
	{
		mStateFile.reset();
		mMessageIdTransactionNrs.clear();
	}

	void MessageId::add(memory::ConstBlockPtr messageId, uint64_t transactionNr)
	{
		iota::MessageId messageIdObj(*messageId);
		if (mMessageIdTransactionNrs.get(messageIdObj).has_value()) {
			throw GradidoAlreadyExist("cache::MessageId already has key!");
		}
		mMessageIdTransactionNrs.add(messageIdObj, transactionNr);
		if (mInitalized) {
			mStateFile.setKeyValue(messageId->convertToHex().data(), std::to_string(transactionNr).data());
		}
	}

	bool MessageId::has(memory::ConstBlockPtr messageId)
	{
		iota::MessageId messageIdObj(*messageId);
		if (mMessageIdTransactionNrs.get(messageIdObj).has_value()) {
			return true;
		}
		return readFromLevelDb(messageIdObj) != 0;
	}

	uint64_t MessageId::getTransactionNrForMessageId(memory::ConstBlockPtr messageId)
	{
		iota::MessageId messageIdObj(*messageId);
		auto result = mMessageIdTransactionNrs.get(messageIdObj);
		if (result.has_value()) {
			return result.value();
		}
		return readFromLevelDb(messageIdObj);
	}

	uint64_t MessageId::readFromLevelDb(const iota::MessageId& iotaMessageIdObj)
	{
		if (mInitalized) {
			std::string value;
			if (mStateFile.getValueForKey(iotaMessageIdObj.toHex().data(), &value)) {
				auto transactionNr = std::stoull(value);
				mMessageIdTransactionNrs.add(iotaMessageIdObj, transactionNr);
				return transactionNr;
			}
		}
		return 0;
	}

}