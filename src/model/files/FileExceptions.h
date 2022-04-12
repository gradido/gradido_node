#ifndef _GRADIDO_NODE_MODEL_FILES_FILE_EXCEPTIONS_H
#define _GRADIDO_NODE_MODEL_FILES_FILE_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/MemoryManager.h"

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

		class FileStreamException : public GradidoBlockchainException
		{
		public:
			explicit FileStreamException(const char* what, const std::string& filename) noexcept;
		};

		class HashMismatchException : public GradidoBlockchainException
		{
		public:
			explicit HashMismatchException(const char* what) noexcept;
			explicit HashMismatchException(const char* what, MemoryBin* hash1, MemoryBin* hash2) noexcept;

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
	}
}

#endif //_GRADIDO_NODE_MODEL_FILES_FILE_EXCEPTIONS_H