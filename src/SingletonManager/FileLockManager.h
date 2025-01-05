#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_FILE_LOCK_MANAGER_H
#define __GRADIDO_NODE_SINGLETON_MANAGER_FILE_LOCK_MANAGER_H

#include <unordered_map>
#include <mutex>

/*!
	@author einhornimmond

	@brief For working with files, prevent access from multiple threads at the same time to a file

	TODO: Profile Memory size of mFiles and maybe using a caching to remove entries for longer not used files
*/

class FileLockManager
{
public:
	~FileLockManager();

	bool isLock(const std::string& file);
	bool tryLock(const std::string& file);
	void unlock(const std::string& file);

	bool tryLockTimeout(const std::string& file, int tryCount);

	static FileLockManager* getInstance();
protected:
	FileLockManager();

	std::mutex mWorkingMutex;
	std::unordered_map<std::string, bool*> mFiles;

};

#endif