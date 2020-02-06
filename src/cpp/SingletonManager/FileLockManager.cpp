#include "FileLockManager.h"
#include <assert.h>

FileLockManager::FileLockManager()
{

}

FileLockManager::~FileLockManager()
{
	for (auto it = mFiles.begin(); it != mFiles.end(); it++) {
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

void FileLockManager::unlock(const std::string& file)
{
	Poco::Mutex::ScopedLock lock(mWorkingMutex);
	auto it = mFiles.find(file);
	assert(it != mFiles.end());
	
	*it->second = false;
} 