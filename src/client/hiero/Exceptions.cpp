#include "Exceptions.h"

namespace client {
	namespace hiero {
		std::string ChannelException::getFullString() const 
		{
			std::string result = what();
			result += ", targe: " + mTarget;
			return result;
		}
	}
}