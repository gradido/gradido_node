#include "Exceptions.h"

namespace cache {
	// ************************** Block Index Invalid File Cursor Exception ***********************
	BlockIndexInvalidFileCursorException::BlockIndexInvalidFileCursorException(
		const char* what, int32_t fileCursor, uint64_t transactionNr, uint64_t minTransactionNr
	) noexcept
		: BlockIndexException(what), mFileCursor(fileCursor), mTransactionNr(transactionNr), mMinTransactionNr(minTransactionNr)
	{

	}

	std::string BlockIndexInvalidFileCursorException::getFullString() const
	{
		std::string result;
		std::string fileCursorString = std::to_string(mFileCursor);
		std::string transactionNrString = std::to_string(mTransactionNr);
		std::string minTransactionString = std::to_string(mMinTransactionNr);
		size_t resultSize = strlen(what()) + 2 + 15 + fileCursorString.size() + 18 + transactionNrString.size() + 23 + minTransactionString.size();
		result.reserve(resultSize);
		result = what();
		result += ", file cursor: " + fileCursorString;
		result += ", transaction nr: " + transactionNrString;
		result += ", min transaction nr: " + minTransactionString;
		return std::move(result);
	}

	DictionaryException::DictionaryException(const char* what, const char* dictionaryName) noexcept
		: GradidoBlockchainException(what), mDictionaryName(dictionaryName)
	{

	}



	DictionaryNotFoundException::DictionaryNotFoundException(const char* what, const char* dictionaryName, const char* key) noexcept
		: DictionaryException(what, dictionaryName), mKey(key)
	{

	}

	std::string DictionaryNotFoundException::getFullString() const
	{
		std::string result = what();
		result += ", dictionary name: " + mDictionaryName;
		result += ", key: " + mKey;
		return result;
	}

	DictionaryInvalidNewKeyException::DictionaryInvalidNewKeyException(const char* what, const char* dictionaryName, uint32_t newKey, uint32_t oldKey) noexcept
		: DictionaryException(what, dictionaryName), mNewKey(newKey), mOldKey(oldKey)
	{

	}

	std::string DictionaryInvalidNewKeyException::getFullString() const
	{
		std::string result = what();
		result += ", dictionary name: " + mDictionaryName;
		result += ", current key: " + std::to_string(mOldKey);
		result += ", new key: " + std::to_string(mNewKey);
		return result;
	}
}