#include "SystemExceptions.h"

NoSpaceLeftOnDevice::NoSpaceLeftOnDevice(const char* what, const std::string& filename) noexcept
	: GradidoBlockchainException(what), mFileName(filename)
{

}

std::string NoSpaceLeftOnDevice::getFullString() const
{
	std::string resultString;
	size_t resultSize = strlen(what()) + mFileName.size() + 2 + 14;
	resultString.reserve(resultSize);
	resultString = what();
	resultString += ", tried file: " + mFileName;

	return resultString;
}
