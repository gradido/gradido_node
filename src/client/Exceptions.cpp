#include "Exceptions.h"

namespace client
{
	MissingParameterException::MissingParameterException(const char* what, const char* parameterName) noexcept
		: GradidoBlockchainException(what), mParameterName(parameterName)
	{
	}

	std::string MissingParameterException::getFullString() const
	{
		std::string resultString;
		size_t resultSize = strlen(what()) + mParameterName.size() + 2 + 18;
		resultString.reserve(resultSize);
		resultString = what();
		resultString += ", parameter name: " + mParameterName;
		return resultString;
	}
}