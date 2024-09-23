#include "SystemExceptions.h"
#include <string>

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

CannotLockMutexAfterTimeout::CannotLockMutexAfterTimeout(const char* what, int timeoutMs) noexcept
	: GradidoBlockchainException(what), mTimeoutMs(timeoutMs)
{

}

std::string CannotLockMutexAfterTimeout::getFullString() const
{
	std::string resultString;
	std::string timeoutStr = std::to_string(mTimeoutMs) + " ms";
	size_t resultSize = strlen(what()) + timeoutStr.size() + 2 + 11;
	resultString.reserve(resultSize);
	resultString = what();
	resultString += ", timeout: " + timeoutStr;

	return resultString;
}

ClassNotInitalizedException::ClassNotInitalizedException(const char* what, const char* classname)
	: GradidoBlockchainException(what), mClassName(classname)
{

}

std::string ClassNotInitalizedException::getFullString() const 
{
	std::string result;
	result = what();
	result += ", class: " + mClassName;
	return result;
}

FileNotFoundException::FileNotFoundException(const char* what, const char* fileName) noexcept
	: GradidoBlockchainException(what), mFileName(fileName)
{

}

std::string FileNotFoundException::getFullString() const
{
	std::string result;
	result = what();
	result += ", file: " + mFileName;
	return result;
}