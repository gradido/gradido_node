#include "FileExceptions.h"

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
		LockException::LockException(const char* what, const char* filename) noexcept
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

		std::string HashMismatchException::getFullString() const
		{
			return what();
		}
	}
}