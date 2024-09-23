#include "ControllerExceptions.h"

#include "magic_enum/magic_enum.hpp"

#include <string>

using namespace gradido::data;

namespace controller {
	GroupNotFoundException::GroupNotFoundException(const char* what, const std::string& groupAlias) noexcept
		: GradidoBlockchainException(what), mGroupAlias(groupAlias)
	{

	}
	std::string GroupNotFoundException::getFullString() const
	{
		std::string resultString;
		size_t resultSize = strlen(what()) + mGroupAlias.size() + 2 + 15;
		resultString.reserve(resultSize);
		resultString = what();
		resultString += ", group alias: " + mGroupAlias;
		return resultString;
	}

	// *******************  BlockNotLoadedException *****************************
	BlockNotLoadedException::BlockNotLoadedException(const char* what, const std::string& groupAlias, int blockNr) noexcept
		: GradidoBlockchainException(what), mGroupAlias(groupAlias), mBlockNr(blockNr)
	{

	}

	std::string BlockNotLoadedException::getFullString() const
	{
		std::string resultString;
		std::string blockNrString = std::to_string(mBlockNr);
		size_t resultSize = strlen(what()) + 2 + 14 + 13 + mGroupAlias.size() + blockNrString.size();
		resultString = what();
		resultString += ", with group: " + mGroupAlias;
		resultString += ", block nr: " + blockNrString;
		return resultString;
	}


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

	// ************************ Wrong Transaction Type Exception **********************************
	WrongTransactionTypeException::WrongTransactionTypeException(
		const char* what, 
		TransactionType type,
		std::string pubkeyHex
	) noexcept
		: GradidoBlockchainException(what), mType(type), mPubkeyHex(pubkeyHex)
	{

	}

	std::string WrongTransactionTypeException::getFullString() const
	{
		std::string resultString;
		std::string transactionTypeString(magic_enum::enum_name(mType));
		size_t resultSize = strlen(what()) + 2 + 20 + transactionTypeString.size() + mPubkeyHex.size() + 10;
		resultString.reserve(resultSize);
		resultString = what();
		resultString += ", transaction type: " + transactionTypeString;
		resultString += ", pubkey: " + mPubkeyHex;
		return resultString;
	}
}