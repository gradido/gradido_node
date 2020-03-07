#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_FILE_LOCK_MANAGER_H
#define __GRADIDO_NODE_SINGLETON_MANAGER_FILE_LOCK_MANAGER_H

#include <unordered_map>
#include "Poco/Mutex.h"

#include "../lib/ErrorList.h"

class FileLockManager
{
public:
	~FileLockManager();

	bool isLock(const std::string& file);
	bool tryLock(const std::string& file);
	void unlock(const std::string& file);

	bool tryLockTimeout(const std::string& file, int tryCount, ErrorList* errorReciver = nullptr);

	static FileLockManager* getInstance();
protected:
	FileLockManager();

	Poco::Mutex mWorkingMutex;
	std::unordered_map<std::string, bool*> mFiles;

};

#endif