#include "Exceptions.h"

namespace client {
	namespace grpc {
		std::string ChannelException::getFullString() const 
		{
			std::string result = what();
			result += ", targe: " + mTarget;
			return result;
		}
	}
}