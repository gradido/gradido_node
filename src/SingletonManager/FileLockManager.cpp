#include "FileLockManager.h"
#include <assert.h>

FileLockManager::FileLockManager()
{

}

FileLockManager::~FileLockManager()
{
	printf("FileLockManager::~FileLockManager\n");
	for (auto it = mFiles.begin(); it != mFiles.end(); it++) {
		printf("%s \n", it->first.data());
		delete it->second;
	}
	mFiles.clear();
}

FileLockManager* FileLockManager::getInstance()
{
	static FileLockManager theOne;
	return &theOne;
}

bool FileLockManager::isLock(const std::string& file)
{
	Poco::Mutex::ScopedLock lock(mWorkingMutex);
	auto it = mFiles.find(file);
	if (it != mFiles.end()) {
		return *it->second;
	}
	mFiles.insert(std::pair<std::string, bool*>(file, new bool(false)));
	return false;
}

bool FileLockManager::tryLock(const std::string& file)
{
	Poco::Mutex::ScopedLock lock(mWorkingMutex);
	auto it = mFiles.find(file);
	if (it != mFiles.end()) {
		if (!*it->second) {
			*it->second = true;
			return true;
		}
		return false;
	}
	mFiles.insert(std::pair<std::string, bool*>(file, new bool(true)));
	return true;
}

bool FileLockManager::tryLockTimeout(const std::string& file, int tryCount, ErrorList* errorReciver/* = nullptr*/)
{
	int timeoutRounds = tryCount;
	bool fileLocked = false;
	while (!fileLocked && timeoutRounds > 0) {
		fileLocked = tryLock(file);
		if (fileLocked) break;
		Poco::Thread::sleep(10);
		timeoutRounds--;
	}

	if (!fileLocked) {
		if (errorReciver) {
			errorReciver->addError(new ParamError(__FUNCTION__, "couldn't lock file, waiting 1 sec", file.data()));
		}
		return false;
	}
	return true;
}

void FileLockManager::unlock(const std::string& file)
{
	Poco::Mutex::ScopedLock lock(mWorkingMutex);
	auto it = mFiles.find(file);
	assert(it != mFiles.end());
	
	*it->second = false;
} 