#ifndef __GRADIDO_NODE_SYSTEM_EXCEPTIONS_H
#define __GRADIDO_NODE_SYSTEM_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"

class NoSpaceLeftOnDevice : public GradidoBlockchainException
{
public:
	explicit NoSpaceLeftOnDevice(const char* what, const std::string& filename) noexcept;
	std::string getFullString() const;
protected:
	std::string mFileName;
};

class CannotLockMutexAfterTimeout : public GradidoBlockchainException
{
public:
	explicit CannotLockMutexAfterTimeout(const char* what, int timeoutMs) noexcept;
	std::string getFullString() const;
protected:
	int mTimeoutMs;
};

class ClassNotInitalizedException : public GradidoBlockchainException
{
public:
	explicit ClassNotInitalizedException(const char* what, const char* classname) noexcept;
	std::string getFullString() const;
protected:
	std::string mClassName;
};

class FileNotFoundException : public GradidoBlockchainException
{
public:
	explicit FileNotFoundException(const char* what, const char* fileName) noexcept;
	std::string getFullString() const;
protected:
	std::string mFileName;
};


#endif //__GRADIDO_NODE_SYSTEM_EXCEPTIONS_H