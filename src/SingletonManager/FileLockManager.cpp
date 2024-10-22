#include "FileLockManager.h"
#include "../model/files/FileExceptions.h"

#include <assert.h>
#include <thread>



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
	std::scoped_lock lock(mWorkingMutex);
	auto it = mFiles.find(file);
	if (it != mFiles.end()) {
		return *it->second;
	}
	mFiles.insert(std::pair<std::string, bool*>(file, new bool(false)));
	return false;
}

bool FileLockManager::tryLock(const std::string& file)
{
	std::scoped_lock lock(mWorkingMutex);
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

bool FileLockManager::tryLockTimeout(const std::string& file, int tryCount)
{
	int timeoutRounds = tryCount;
	bool fileLocked = false;
	while (!fileLocked && timeoutRounds > 0) {
		fileLocked = tryLock(file);
		if (fileLocked) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		timeoutRounds--;
	}

	if (!fileLocked) {
		throw model::files::LockException("couldn't lock file", file.data());
		return false;
	}
	return true;
}

void FileLockManager::unlock(const std::string& file)
{
	std::scoped_lock lock(mWorkingMutex);
	auto it = mFiles.find(file);
	assert(it != mFiles.end());
	
	*it->second = false;
} 