#ifndef __GRADIDO_NODE_CACHE_EXCEPTIONS_H
#define __GRADIDO_NODE_CACHE_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"

namespace cache {
	class BlockException : public GradidoBlockchainException
	{
	public:
		explicit BlockException(const char* what) noexcept : GradidoBlockchainException(what) {};
		std::string getFullString() const { return what(); }
	};

	class BlockIndexException : public GradidoBlockchainException
	{
	public:
		explicit BlockIndexException(const char* what) noexcept : GradidoBlockchainException(what) {};
		std::string getFullString() const { return what(); }
	};

	class BlockIndexInvalidFileCursorException : public BlockIndexException
	{
	public:
		explicit BlockIndexInvalidFileCursorException(const char* what, int32_t fileCursor, uint64_t transactionNr, uint64_t minTransactionNr) noexcept;
		std::string getFullString() const;
	protected:
		int32_t mFileCursor;
		uint64_t mTransactionNr;
		uint64_t mMinTransactionNr;
	};
}

#endif //__GRADIDO_NODE_CACHE_EXCEPTIONS_H