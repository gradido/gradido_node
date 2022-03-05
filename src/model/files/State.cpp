#include "State.h"

#include <cassert>
#include "Poco/Mutex.h"
#include "Poco/Thread.h"
#include "../../lib/LevelDBExceptions.h"
#include "../../SingletonManager/LoggerManager.h"

namespace model {
	namespace files {
	
		// use this global mutex to prevent an error with level db if one group is deconstructed while the same group is created new at the same time
		Poco::FastMutex g_StateMutex;

		State::State(Poco::Path path)
			: mLevelDB(nullptr)
		{
			Poco::ScopedLock<Poco::FastMutex> _lock(g_StateMutex);
			path.pushDirectory(".state");
			leveldb::Options options;
			options.create_if_missing = true;
			leveldb::Status status = leveldb::DB::Open(options, path.toString(), &mLevelDB);
			// if group is removed from cache and created new at the same time, the lock file from other level db instance is maybe still there 
			// and trigger an io error, so give it same time an try it again, maximal 100 times. 
			// TODO: Maybe use the FileLockManager for this 
			int maxTry = 100;
			while (status.IsIOError() && maxTry > 0) {
				Poco::Thread::sleep(100);
				status = leveldb::DB::Open(options, path.toString(), &mLevelDB);
				maxTry--;
			}
			if (!status.ok()) {
				 //status.ToString()
				LoggerManager::getInstance()->mErrorLogging.error("[State::State] path: %s, state: %s, ioError: %d", path.toString(), status.ToString(), status.IsIOError());
			}
			assert(status.ok());
		}

		State::~State()
		{
			Poco::ScopedLock<Poco::FastMutex> _lock(g_StateMutex);
			if (mLevelDB) {
				delete mLevelDB;
				mLevelDB = nullptr;
			}
			//mDBEntrys.clear();
		}

		std::string State::getValueForKey(const std::string& key)
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);

			std::string value;
			leveldb::Status s = mLevelDB->Get(leveldb::ReadOptions(), key, &value);
			if (s.ok()) {
				return value;
			}

			return "<not found>";
		}

		int32_t State::getInt32ValueForKey(const std::string& key, int32_t defaultValue /* = 0 */)
		{
			auto value = getValueForKey(key);
			if (value == "<not found>") {
				return defaultValue;
			}
			
			return std::atoi(value.data());
		}

		bool State::addKeyValue(const std::string& key, const std::string& value)
		{
			// check if already exist
			auto result = getValueForKey(key);

			Poco::FastMutex::ScopedLock lock(mFastMutex);

			if (result != "<not found>") {
				throw LevelDBKeyAlreadyExistException("key already exist", key, value);
			}

			leveldb::Status s = mLevelDB->Put(leveldb::WriteOptions(), key, value);
			if (!s.ok()) {
				throw LevelDBStatusException("cannot put to level db", s);
			}
			return true;
		}
		bool State::setKeyValue(const std::string& key, const std::string& value)
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);
			leveldb::Status s = mLevelDB->Put(leveldb::WriteOptions(), key, value);
			if (!s.ok()) {
				throw LevelDBStatusException("cannot put to level db", s);
			}
			return true;
		}

	}
}