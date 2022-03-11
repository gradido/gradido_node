#ifndef __GRADIDO_NODE_CLIENT_EXCEPTIONS_H
#define __GRADIDO_NODE_CLIENT_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"

namespace client
{
	class MissingParameterException : public GradidoBlockchainException
	{
	public: 
		explicit MissingParameterException(const char* what, const char* parameterName) noexcept;
		std::string getFullString() const;

	protected:
		std::string mParameterName;
	};
}

#endif //__GRADIDO_NODE_CLIENT_EXCEPTIONS_H