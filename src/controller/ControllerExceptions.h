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
}

#endif //__GRADIDO_NODE_CONTROLLER_CONTROLLER_EXCEPTIONS_H