#include "LevelDBExceptions.h"

LevelDBKeyAlreadyExistException::LevelDBKeyAlreadyExistException(const char* what, const std::string& key, const std::string& value) noexcept
	: GradidoBlockchainException(what), mKey(key), mValue(value)
{

}

std::string LevelDBKeyAlreadyExistException::getFullString() const
{
	std::string result;
	size_t resultSize = strlen(what()) + mKey.size() + 7 + 2; 
	result.reserve(resultSize);
	result = what();
	result += ", key: " + mKey;
	return result;
}

// #########################  Level DB Put Exception ###############################
LevelDBStatusException::LevelDBStatusException(const char* what, const leveldb::Status& status) noexcept
	: GradidoBlockchainException(what), mStatusString(status.ToString())
{

}

std::string LevelDBStatusException::getFullString() const
{
	std::string result = what();
	result += ", level db status: " + mStatusString;
	return result;
}