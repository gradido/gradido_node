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

	class DictionaryException : public GradidoBlockchainException
	{
	public: 
		explicit DictionaryException(const char* what, const char* dictionaryName) noexcept;
	protected:
		std::string mDictionaryName;
	};

	class DictionaryNotFoundException : public DictionaryException
	{
	public:
		explicit DictionaryNotFoundException(const char* what, const char* dictionaryName, const char* key) noexcept;
		std::string getFullString() const;

	protected:
		std::string mDictionaryName;
		std::string mKey;
	};

	class DictionaryInvalidNewKeyException : public DictionaryException
	{
	public:
		explicit DictionaryInvalidNewKeyException(const char* what, const char* dictionaryName, uint32_t newKey, uint32_t oldKey) noexcept;
		std::string getFullString() const;

	protected:
		uint32_t mNewKey;
		uint32_t mOldKey;
	};
}

#endif //__GRADIDO_NODE_CACHE_EXCEPTIONS_H