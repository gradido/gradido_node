#ifndef _GRADIDO_NODE_MODEL_FILES_FILE_EXCEPTIONS_H
#define _GRADIDO_NODE_MODEL_FILES_FILE_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"
#include <fstream>

namespace model {
	namespace files {
		class IndexStringPairAlreadyExistException : public GradidoBlockchainException
		{
		public:
			explicit IndexStringPairAlreadyExistException(const char* what, std::string currentPair, std::string lastPair) noexcept;

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
			explicit HashMismatchException(const char* what, const memory::Block& hash1, const memory::Block& hash2) noexcept;

			std::string getFullString() const;
		protected:
			std::string mHash1Hex;
			std::string mHash2Hex;
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

		class InvalidReadBlockSize : public EndReachingException
		{
		public: 
			explicit InvalidReadBlockSize(const char* what, const char* filename, int readCursor, size_t blockSize) noexcept
				: EndReachingException(what, filename, readCursor, blockSize) {};

		};

		class OpenFileException : public GradidoBlockchainException
		{
		public: 
			explicit OpenFileException(const char* what, const char* fileName, std::ios::iostate errorState) noexcept
				: GradidoBlockchainException(what), mFileName(fileName), mErrorState(errorState) {}

			std::string getFullString() const;

		protected:
			std::string mFileName;
			std::ios::iostate mErrorState;
		};
	}
}

#endif //_GRADIDO_NODE_MODEL_FILES_FILE_EXCEPTIONS_H