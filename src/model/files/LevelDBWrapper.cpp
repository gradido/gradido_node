#include "LevelDBWrapper.h"
#include "../../lib/LevelDBExceptions.h"
#include "../../SingletonManager/FileLockManager.h"

#include "loguru/loguru.hpp"
#include "leveldb/cache.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <thread>

using std::chrono::milliseconds, std::this_thread::sleep_for;
using std::filesystem::remove_all;
using std::optional, std::nullopt, std::string, std::string_view, std::function;
using leveldb::Status, leveldb::DB, leveldb::Slice, leveldb::ReadOptions, leveldb::WriteOptions, leveldb::NewLRUCache;

namespace model {
	namespace files {

		LevelDBWrapper::LevelDBWrapper(string_view folderName)
			: mFolderName(folderName), mLevelDB(nullptr)
		{
		}

		LevelDBWrapper::~LevelDBWrapper()
		{
			exit();
		}

		bool LevelDBWrapper::init(size_t cacheInByte/* = 0*/)
		{
			auto fm = FileLockManager::getInstance();
			if (!fm->tryLockTimeout(mFolderName, 100)) {
				LOG_F(ERROR, "path: %s couldn't locked, another process still use this folder?", mFolderName.c_str());
				return false;
			}
			mOptions.create_if_missing = true;
			mOptions.paranoid_checks = true;
			if (cacheInByte) {
				mOptions.block_cache = NewLRUCache(cacheInByte);
			}
			Status status = DB::Open(mOptions, mFolderName, &mLevelDB);
			// if blockchain::FileBased is removed from cache and created new at the same time, the lock file from other level db instance is maybe still there 
			// and trigger an io error, so give it same time an try it again, maximal 100 times. 
			// TODO: Maybe use the FileLockManager for this 
			int maxTry = 100;
			while (status.IsIOError() && maxTry > 0) {
				sleep_for(milliseconds(100));
				status = DB::Open(mOptions, mFolderName, &mLevelDB);
				maxTry--;
			}
			fm->unlock(mFolderName);
			if (!status.ok()) {
				LOG_F(ERROR, "path: %s, state: %s, ioError: %d", mFolderName.data(), status.ToString().data(), status.IsIOError());
			}
			return status.ok();
		}
		void LevelDBWrapper::exit()
		{
			auto fm = FileLockManager::getInstance();
			if (!fm->tryLockTimeout(mFolderName, 1000)) {
				LOG_F(FATAL, "on exit: path: %s couldn't locked, another process still use this folder, data maybe corrupted, please refresh!", mFolderName.c_str());
				return;
			}
			if (mLevelDB) {
				delete mLevelDB;
				mLevelDB = nullptr;
				if (mOptions.block_cache) {
					delete mOptions.block_cache;
					mOptions.block_cache = nullptr;
				}
			}
			fm->unlock(mFolderName);
		}

		void LevelDBWrapper::reset()
		{
			exit();
			remove_all(mFolderName);
			init();
		}

		optional<string> LevelDBWrapper::getValueForKey(const std::string& key)
		{
			string value;
			Status s = mLevelDB->Get(ReadOptions(), key, &value);
			if (!s.ok()) { return nullopt; }
			return value;
		}

		void LevelDBWrapper::setKeyValue(const std::string& key, const std::string& value)
		{
			WriteOptions writeOptions;
			writeOptions.sync = false;
			Status s = mLevelDB->Put(writeOptions, key, value);
			if (!s.ok()) {
				throw LevelDBStatusException("cannot put to level db", s);
			}
		}

		void LevelDBWrapper::removeKey(const std::string& key)
		{
			mLevelDB->Delete(WriteOptions(), key);
		}

		void LevelDBWrapper::iterate(function<void(Slice key, Slice value)> callback)
		{
			//! \brief get iterator for looping over every entry
			ReadOptions options;
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