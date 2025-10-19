#ifndef __GRADIDO_NODE_CLIENT_HIERO_EXCEPTIONS_H
#define __GRADIDO_NODE_CLIENT_HIERO_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"

namespace client {
	namespace hiero {
		class Exception : public GradidoBlockchainException 
		{
		public:
			using GradidoBlockchainException::GradidoBlockchainException;
		};

		class SSLException : public Exception 
		{
		public: 
			using Exception::Exception;
			std::string getFullString() const { return what();}
		};

		class ChannelException : public Exception
		{
		public:
			explicit ChannelException(const char* what, const char* target) noexcept
				: Exception(what), mTarget(target) {}

			std::string getFullString() const;

		protected: 
			std::string mTarget;
		};
	}
}

#endif // __GRADIDO_NODE_CLIENT_HIERO_EXCEPTIONS_H