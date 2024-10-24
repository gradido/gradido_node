#include "State.h"
#include "../../lib/LevelDBExceptions.h"

#include "loguru/loguru.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <thread>

namespace model {
	namespace files {
	
		// use this global mutex to prevent an error with level db if one group is deconstructed while the same group is created new at the same time
		std::mutex g_StateMutex;

		State::State(std::string_view folderName)
			: mFolderName(folderName), mLevelDB(nullptr)
		{
		}

		State::~State()
		{
			exit();
		}

		bool State::init()
		{
			std::lock_guard _lock(g_StateMutex);
			leveldb::Options options;
			options.create_if_missing = true;
			leveldb::Status status = leveldb::DB::Open(options, mFolderName, &mLevelDB);
			// if blockchain::FileBased is removed from cache and created new at the same time, the lock file from other level db instance is maybe still there 
			// and trigger an io error, so give it same time an try it again, maximal 100 times. 
			// TODO: Maybe use the FileLockManager for this 
			int maxTry = 100;
			while (status.IsIOError() && maxTry > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				status = leveldb::DB::Open(options, mFolderName, &mLevelDB);
				maxTry--;
			}
			if (!status.ok()) {
				LOG_F(ERROR, "path: %s, state: %s, ioError: %d", mFolderName.data(), status.ToString().data(), status.IsIOError());
			}
			return status.ok();
		}
		void State::exit()
		{
			std::lock_guard _lock(g_StateMutex);
			if (mLevelDB) {
				delete mLevelDB;
				mLevelDB = nullptr;
			}
		}

		void State::reset()
		{
			exit();
			std::filesystem::remove_all(mFolderName);
			init();
		}

		bool State::getValueForKey(const char* key, std::string* value)
		{
			leveldb::Status s = mLevelDB->Get(leveldb::ReadOptions(), key, value);
			return s.ok();
		}

		bool State::setKeyValue(const char* key, const std::string& value)
		{
			leveldb::Status s = mLevelDB->Put(leveldb::WriteOptions(), key, value);
			if (!s.ok()) {
				throw LevelDBStatusException("cannot put to level db", s);
			}
			return true;
		}

		void State::removeKey(const char* key)
		{
			mLevelDB->Delete(leveldb::WriteOptions(), key);
		}
	}
}