#include "FileExceptions.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include <string>

namespace model {
	namespace files {
		IndexAddressPairAlreadyExistException::IndexAddressPairAlreadyExistException(const char* what, std::string currentPair, std::string lastPair) noexcept
			: GradidoBlockchainException(what), mCurrentPair(currentPair), mLastPair(lastPair)
		{

		}

		std::string IndexAddressPairAlreadyExistException::getFullString() const
		{
			std::string result;
			size_t resultSize = strlen(what()) + mCurrentPair.size() + mLastPair.size() + 16 + 14 + 2;
			result.reserve(resultSize);

			result = what();
			result += ", current pair: " + mCurrentPair;
			result += ", last pair: " + mLastPair;
			return result;

		}

		// ************** File Lock Exception ***************************
		LockException::LockException(const char* what, const std::string& filename) noexcept
			: GradidoBlockchainException(what), mFileName(filename)
		{

		}

		std::string LockException::getFullString() const
		{
			std::string result;
			size_t resultSize = strlen(what()) + mFileName.size() + 12 + 2;
			result.reserve(resultSize);
			result = what();
			result += ", filename: " + mFileName;
			return result;
		}

		// ******************** File Hash Exception ******************************
		HashMismatchException::HashMismatchException(const char* what) noexcept
			: GradidoBlockchainException(what)
		{
		}
		HashMismatchException::HashMismatchException(const char* what, MemoryBin* hash1, MemoryBin* hash2) noexcept
			: GradidoBlockchainException(what)
		{
			mHash1Hex = DataTypeConverter::binToHex(hash1);
			mHash2Hex = DataTypeConverter::binToHex(hash2);
		}

		std::string HashMismatchException::getFullString() const
		{
			std::string result;
			size_t resultSize = strlen(what());
			resultSize += mHash1Hex.size() + mHash2Hex.size() + 2 + 9 + 9;
			result.reserve(resultSize);
			result = what();
			result += ", hash1: " + mHash1Hex;
			result += ", hash2: " + mHash2Hex;
			return result;
		}

		// ********************** File Hash Missing **************************************
		HashMissingException::HashMissingException(const char* what, const char* filename) noexcept
			: GradidoBlockchainException(what), mFilename(filename)
		{

		}

		std::string HashMissingException::getFullString() const
		{
			std::string resultString;
			size_t resultSize = strlen(what()) + mFilename.size() + 2 + 8;
			resultString.reserve(resultSize);
			resultString = what();
			resultString += ", file: " + mFilename;
			return resultString;
		}

		// *********************** End Reached Exception ****************************
		EndReachingException::EndReachingException(const char* what, const char* filename, int readCursor, size_t blockSize) noexcept
			: GradidoBlockchainException(what), mFilename(filename), mReadCursor(readCursor), mBlockSize(blockSize)
		{

		}

		std::string EndReachingException::getFullString() const
		{
			std::string resultString;
			std::string readCursorString = std::to_string(mReadCursor);
			std::string blockSizeString = std::to_string(mBlockSize);
			size_t resultSize = strlen(what()) + mFilename.size() + readCursorString.size() + blockSizeString.size() + 2 + 8 + 27;
			resultString.reserve(resultSize);

			resultString = what();
			resultString += ", file: " + mFilename;
			resultString += ", try to read from " + readCursorString + ", " + blockSizeString + " bytes";
			return resultString;
		}
	}
}