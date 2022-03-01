#ifndef _GRADIDO_NODE_MODEL_FILES_FILE_EXCEPTIONS_H
#define _GRADIDO_NODE_MODEL_FILES_FILE_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"

namespace model {
	namespace files {
		class IndexAddressPairAlreadyExistException : public GradidoBlockchainException
		{
		public:
			explicit IndexAddressPairAlreadyExistException(const char* what, std::string currentPair, std::string lastPair) noexcept;

			std::string getFullString() const;

		protected:
			std::string mCurrentPair;
			std::string mLastPair;
		};

		class LockException : public GradidoBlockchainException
		{
		public:
			explicit LockException(const char* what, const std::string& filename) noexcept;
			std::string getFullString() const;
		protected:
			std::string mFileName;
		};

		class HashMismatchException : public GradidoBlockchainException
		{
		public:
			explicit HashMismatchException(const char* what) noexcept;

			std::string getFullString() const;
		};

		class HashMissingException : public GradidoBlockchainException
		{
		public: 
			explicit HashMissingException(const char* what, const char* filename) noexcept;

			std::string getFullString() const;
		protected:
			std::string mFilename;
		};

		class EndReachingException : public GradidoBlockchainException
		{
		public:
			explicit EndReachingException(const char* what, const char* filename, int readCursor, size_t blockSize) noexcept;
			
			std::string getFullString() const;

		protected:
			std::string mFilename;
			int mReadCursor;
			size_t mBlockSize;
		};
	}
}

#endif //_GRADIDO_NODE_MODEL_FILES_FILE_EXCEPTIONS_H