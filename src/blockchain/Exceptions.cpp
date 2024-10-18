#include "Exceptions.h"

namespace gradido {
	namespace blockchain {
		CommunityNotFoundExceptions::CommunityNotFoundExceptions(const char* what, std::string_view communityId) noexcept
			: GradidoBlockchainException(what), mCommunityId(communityId)
		{

		}

		std::string CommunityNotFoundExceptions::getFullString() const
		{
			std::string result = what();
			result += ", community: " + mCommunityId;
			return result;
		}

	}
}