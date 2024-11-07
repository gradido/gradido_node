#include "LevelDBWrapper.h"
#include "../../lib/LevelDBExceptions.h"

#include "loguru/loguru.hpp"
#include "leveldb/cache.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <thread>

namespace model {
	namespace files {
	
		// use this global mutex to prevent an error with level db if one group is deconstructed while the same group is created new at the same time
		std::mutex g_StateMutex;

		LevelDBWrapper::LevelDBWrapper(std::string_view folderName)
			: mFolderName(folderName), mLevelDB(nullptr)
		{
		}

		LevelDBWrapper::~LevelDBWrapper()
		{
			exit();
		}

		bool LevelDBWrapper::init(size_t cacheInByte/* = 0*/)
		{
			std::lock_guard _lock(g_StateMutex);
			mOptions.create_if_missing = true;
			mOptions.paranoid_checks = true;
			if (cacheInByte) {
				mOptions.block_cache = leveldb::NewLRUCache(cacheInByte);
			}
			leveldb::Status status = leveldb::DB::Open(mOptions, mFolderName, &mLevelDB);
			// if blockchain::FileBased is removed from cache and created new at the same time, the lock file from other level db instance is maybe still there 
			// and trigger an io error, so give it same time an try it again, maximal 100 times. 
			// TODO: Maybe use the FileLockManager for this 
			int maxTry = 100;
			while (status.IsIOError() && maxTry > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				status = leveldb::DB::Open(mOptions, mFolderName, &mLevelDB);
				maxTry--;
			}
			if (!status.ok()) {
				LOG_F(ERROR, "path: %s, state: %s, ioError: %d", mFolderName.data(), status.ToString().data(), status.IsIOError());
			}
			return status.ok();
		}
		void LevelDBWrapper::exit()
		{
			std::lock_guard _lock(g_StateMutex);
			if (mLevelDB) {
				delete mLevelDB;
				mLevelDB = nullptr;
				if (mOptions.block_cache) {
					delete mOptions.block_cache;
					mOptions.block_cache = nullptr;
				}
			}
		}

		void LevelDBWrapper::reset()
		{
			exit();
			std::filesystem::remove_all(mFolderName);
			init();
		}

		bool LevelDBWrapper::getValueForKey(const char* key, std::string* value)
		{
			leveldb::Status s = mLevelDB->Get(leveldb::ReadOptions(), key, value);
			return s.ok();
		}

		void LevelDBWrapper::setKeyValue(const char* key, const std::string& value)
		{
			leveldb::Status s = mLevelDB->Put(leveldb::WriteOptions(), key, value);
			if (!s.ok()) {
				throw LevelDBStatusException("cannot put to level db", s);
			}
		}

		void LevelDBWrapper::removeKey(const char* key)
		{
			mLevelDB->Delete(leveldb::WriteOptions(), key);
		}

		void LevelDBWrapper::iterate(std::function<void(leveldb::Slice key, leveldb::Slice value)> callback)
		{
			//! \brief get iterator for looping over every entry
			leveldb::ReadOptions options;
			options.fill_cache = false;
			options.verify_checksums = true;
			auto it = mLevelDB->NewIterator(options);
			assert(it);
			for (it->SeekToFirst(); it->Valid(); it->Next()) {
				callback(it->key(), it->value());
			}
			if (!it->status().ok()) {
				throw LevelDBStatusException("error in iterate over leveldb entries", it->status());
			}
			delete it;
		}
	}
}