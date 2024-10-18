#ifndef __GRADIDO_NODE_BLOCKCHAIN_EXCEPTIONS_H
#define __GRADIDO_NODE_BLOCKCHAIN_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"

#include <string_view>

namespace gradido {
	namespace blockchain {
		class CommunityNotFoundExceptions : GradidoBlockchainException
		{
		public:
			explicit CommunityNotFoundExceptions(const char* what, std::string_view communityId) noexcept;
			std::string getFullString() const;

		protected:
			std::string mCommunityId;

		};
	}
}


#endif //__GRADIDO_NODE_BLOCKCHAIN_EXCEPTIONS_H