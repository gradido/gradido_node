#ifndef __GRADIDO_NODE_CONTROLLER_CONTROLLER_EXCEPTIONS_H
#define __GRADIDO_NODE_CONTROLLER_CONTROLLER_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"

namespace controller {

	class GroupNotFoundException : public GradidoBlockchainException
	{
	public:
		explicit GroupNotFoundException(const char* what, const std::string& groupAlias) noexcept;
		std::string getFullString() const;
	protected:
		std::string mGroupAlias;
	};

	class BlockNotLoadedException : public GradidoBlockchainException
	{
	public:
		explicit BlockNotLoadedException(const char* what, const std::string& groupAlias, int blockNr) noexcept;
		std::string getFullString() const;

	protected:
		std::string mGroupAlias;
		int			mBlockNr;
	};

	class WrongTransactionTypeException : public GradidoBlockchainException
	{
	public:
		explicit WrongTransactionTypeException(
			const char* what, 
			model::gradido::TransactionType type,
			std::string pubkeyHex
		) noexcept;
		std::string getFullString() const;

	protected:
		model::gradido::TransactionType mType;
		std::string mPubkeyHex;
	};

}

#endif //__GRADIDO_NODE_CONTROLLER_CONTROLLER_EXCEPTIONS_H