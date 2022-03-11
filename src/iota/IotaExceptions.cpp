#include "IotaExceptions.h"


MessageIdFormatException::MessageIdFormatException(const char* what, const std::string& messageIdHex) noexcept
	: GradidoBlockchainException(what), mMessageIdHex(messageIdHex)
{

}

std::string MessageIdFormatException::getFullString() const
{
	std::string resultString;
	size_t resultSize = strlen(what()) + 22 + 2 + mMessageIdHex.size();
	resultString.reserve(resultSize);
	resultString = what();
	resultString += " with message id hex: " + mMessageIdHex;
	return resultString;
}
