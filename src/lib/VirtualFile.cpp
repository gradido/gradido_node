#include "VirtualFile.h"

#include "Poco/File.h"
#include "Poco/FileStream.h"

#include "../model/files/FileExceptions.h"

#include "../SingletonManager/FileLockManager.h"
#include "../SingletonManager/LoggerManager.h"

VirtualFile::VirtualFile(size_t bufferSize)
	: mBuffer((unsigned char*)malloc(bufferSize)), mBufferSize(bufferSize), mCursor(0)
{

}
VirtualFile::VirtualFile(unsigned char* buffer, size_t bufferSize)
	: mBuffer(buffer), mBufferSize(bufferSize), mCursor(0)
{
}

VirtualFile::~VirtualFile()
{
	if (mBuffer) {
		free(mBuffer);
		mBuffer = nullptr;
		mBufferSize = 0;
	}
}

bool VirtualFile::read(void* dst, size_t size)
{
	assert(mBuffer);
	if (mCursor + size > mBufferSize) {
		throw BufferOverflowException("try more read than in buffer", mBufferSize, mCursor + size);
	}
	
	memcpy(dst, &mBuffer[mCursor], size);
	mCursor += size;
	return true;
}

bool VirtualFile::write(const void* src, size_t size)
{
	assert(mBuffer);
	if (mCursor + size > mBufferSize) {
		throw BufferOverflowException("try more write than in buffer", mBufferSize, mCursor + size);
	}

	memcpy(&mBuffer[mCursor], src, size);
	mCursor += size;
	return true;
}
bool VirtualFile::setCursor(size_t dst)
{
	assert(mBuffer);
	if (dst > mBufferSize) return false;
	mCursor = dst;
	return true;
}

bool VirtualFile::writeToFile(const char* filename)
{
	auto fl = FileLockManager::getInstance();

	if (!fl->tryLockTimeout(filename, 100)) {
		return false;
	}

	Poco::FileOutputStream file(filename);
	file.seekp(0, std::ios_base::beg);

	file.write((const char*)mBuffer, mCursor);
	file.close();

	fl->unlock(filename);

	return true;
}

VirtualFile* VirtualFile::readFromFile(const char* filename)
{
	auto fl = FileLockManager::getInstance();

	if (!fl->tryLockTimeout(filename, 100)) {
		throw model::files::LockException("cannot lock file to read into virtual file", filename);
	}
	try {
		Poco::FileInputStream file(filename);
		file.seekg(0, std::ios_base::end);
		auto telled = file.tellg();
		if (!telled) {
			file.close();
			fl->unlock(filename);
			return nullptr;
		}
		file.seekg(0, std::ios_base::beg);

		auto fileBuffer = (unsigned char*)malloc(telled);
		file.read((char*)fileBuffer, telled);
		file.close();

		fl->unlock(filename);

		return new VirtualFile(fileBuffer, telled);
	} 
	catch (Poco::FileNotFoundException& ex) {
		return nullptr;
	}
	catch (Poco::Exception& ex) {
		fl->unlock(filename);

		auto lm = LoggerManager::getInstance();
		//printf("[%s] Poco Exception: %s\n", __FUNCTION__, ex.displayText().data());
		std::string functionName = __FUNCTION__;
		lm->mErrorLogging.error("[%s] Poco Exception: %s\n", functionName, ex.displayText());
		return nullptr;
	}
	
}

// ******************** Exceptions *********************************
BufferOverflowException::BufferOverflowException(const char* what, size_t bufferSize, size_t askSize) noexcept
	: GradidoBlockchainException(what), mBufferSize(bufferSize), mAskSize(askSize)
{

}

std::string BufferOverflowException::getFullString() const 
{
	return what();
}