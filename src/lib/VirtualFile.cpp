#include "VirtualFile.h"

#include "Poco/File.h"
#include "Poco/FileStream.h"

#include "../SingletonManager/FileLockManager.h"
#include "../SingletonManager/LoggerManager.h"

VirtualFile::VirtualFile(size_t size)
	: mBuffer(MemoryManager::getInstance()->getMemory(size)), mCursor(0)
{

}
VirtualFile::VirtualFile(MemoryBin* buffer)
	: mBuffer(buffer), mCursor(0)
{
}

VirtualFile::~VirtualFile()
{
	if (mBuffer) {
		MemoryManager::getInstance()->releaseMemory(mBuffer);
	}
}

bool VirtualFile::read(void* dst, size_t size)
{
	assert(mBuffer);
	if (mCursor + size > mBuffer->size()) {
		return false;
	}
	
	memcpy(dst, &mBuffer->data()[mCursor], size);
	mCursor += size;
	return true;
}

bool VirtualFile::write(const void* src, size_t size)
{
	assert(mBuffer);
	if (mCursor + size > mBuffer->size()) {
		return false;
	}

	memcpy(&mBuffer->data()[mCursor], src, size);
	mCursor += size;
	return true;
}
bool VirtualFile::setCursor(size_t dst)
{
	assert(mBuffer);
	if (dst > mBuffer->size()) return false;
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

	file.write(*mBuffer, mCursor);
	file.close();

	fl->unlock(filename);

	return true;
}

VirtualFile* VirtualFile::readFromFile(const char* filename)
{
	auto fl = FileLockManager::getInstance();
	auto mm = MemoryManager::getInstance();
	

	if (!fl->tryLockTimeout(filename, 100)) {
		return nullptr;
	}
	try {
		Poco::FileInputStream file(filename);
		file.seekg(0, std::ios_base::end);
		auto telled = file.tellg();
		file.seekg(0, std::ios_base::beg);

		auto fileBuffer = mm->getMemory(telled);
		file.read(*fileBuffer, telled);
		file.close();

		fl->unlock(filename);

		return new VirtualFile(fileBuffer);

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
