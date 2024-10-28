#include "FileExceptions.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include <string>

namespace model {
	namespace files {
		IndexStringPairAlreadyExistException::IndexStringPairAlreadyExistException(const char* what, std::string currentPair, std::string lastPair) noexcept
			: GradidoBlockchainException(what), mCurrentPair(currentPair), mLastPair(lastPair)
		{

		}

		std::string IndexStringPairAlreadyExistException::getFullString() const
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
		HashMismatchException::HashMismatchException(const char* what, const memory::Block& hash1, const memory::Block& hash2) noexcept
			: GradidoBlockchainException(what), mHash1Hex(hash1.convertToHex()), mHash2Hex(hash2.convertToHex())
		{
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

		std::string OpenFileException::getFullString() const
		{
			std::string resultString = what();
			resultString += ", file name: " + mFileName;
			if ((std::ios::eofbit & mErrorState) == std::ios::eofbit) {
				resultString += ", End-of-File reached on input operation";
			}

			if ((std::ios::failbit & mErrorState) == std::ios::failbit) {
				resultString += ", Logical error on i/o operation";
			}

			if ((std::ios::badbit & mErrorState) == std::ios::badbit) {
				resultString += ", Read/writing error on i/o operation";
			}
			return resultString;
		}
	}
}