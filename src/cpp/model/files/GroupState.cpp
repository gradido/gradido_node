#include "GroupState.h"

#include <cassert>

namespace model {
	namespace files {
	
		GroupState::GroupState(Poco::Path path)
			: mLevelDB(nullptr)
		{
			path.pushDirectory(".state");
			leveldb::Options options;
			options.create_if_missing = true;
			leveldb::Status status = leveldb::DB::Open(options, path.toString(), &mLevelDB);
			assert(status.ok());
		}

		GroupState::~GroupState()
		{
			if (mLevelDB) {
				delete mLevelDB;
				mLevelDB = nullptr;
			}
			//mDBEntrys.clear();
		}

		std::string GroupState::getValueForKey(const std::string& key)
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);

			std::string value;
			leveldb::Status s = mLevelDB->Get(leveldb::ReadOptions(), key, &value);
			if (s.ok()) {
				return value;
			}

			return "<not found>";
		}

		int32_t GroupState::getInt32ValueForKey(const std::string& key, int32_t defaultValue /* = 0 */)
		{
			auto value = getValueForKey(key);
			if (value == "<not found>") {
				return defaultValue;
			}
			try {
				return std::atoi(value.data());
			}
			catch (const std::invalid_argument& ia) {
				addError(new ParamError(__FUNCTION__, "Invalid Argument", ia.what()));
			}
			catch (const std::out_of_range& oor) {
				addError(new ParamError(__FUNCTION__, "Out of Range Error", oor.what()));
			}
			catch (const std::logic_error & ler) {
				addError(new ParamError(__FUNCTION__, "Logical Error", ler.what()));
			}
			catch (...) {
				addError(new Error(__FUNCTION__, "Unknown error"));
			}
			return defaultValue;
		}

		bool GroupState::addKeyValue(const std::string& key, const std::string& value)
		{
			// check if already exist
			auto result = getValueForKey(key);

			Poco::FastMutex::ScopedLock lock(mFastMutex);

			if (result != "<not found>") {
				addError(new Error(__FUNCTION__, "key already exist"));
				return false;
			}

			leveldb::Status s = mLevelDB->Put(leveldb::WriteOptions(), key, value);
			if (s.ok()) {
				return true;
			} 
			addError(new Error(__FUNCTION__, "cannot put to level db"));

			return false;
		}
		bool GroupState::setKeyValue(const std::string& key, const std::string& value)
		{
			Poco::FastMutex::ScopedLock lock(mFastMutex);
			leveldb::Status s = mLevelDB->Put(leveldb::WriteOptions(), key, value);
			if (s.ok()) {
				return true;
			}
			addError(new Error(__FUNCTION__, "cannot put to level db"));

			return false;
		}

	}
}