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
}