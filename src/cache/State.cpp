#include "State.h"
#include "../SystemExceptions.h"
#include "../lib/LevelDBExceptions.h"

#include "loguru/loguru.hpp"

namespace cache {
	State::State(std::string_view folder)
		: mInitalized(false),
		mStateFile(folder)
	{

	}

	State::~State()
	{
			
	}

	bool State::init(size_t cacheInBytes)
	{
		if (mInitalized) {
			throw ClassAlreadyInitalizedException("init was already called", "cache::State");
		}
		if (!mStateFile.init(cacheInBytes)) {
			return false;
		}
		mInitalized = true;
		return true;
	}

	void State::exit()
	{
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, states aren't stored in leveldb file, or exit was called more than once");
		}
		mInitalized = false;
		mStateFile.exit();
	}		

	void State::reset()
	{
		mStateFile.reset();
	}

	void State::updateState(const char* key, std::string_view value)
	{
		if (!mInitalized) {
			throw ClassNotInitalizedException("cannot update state, state wasn't initalized", "cache::State");
		}
		mStateFile.setKeyValue(key, std::string(value));					
	}


	void State::updateState(const char* key, int32_t value)
	{
		if (!mInitalized) {
			throw ClassNotInitalizedException("cannot update state, state wasn't initalized", "cache::State");
		}
		
		mStateFile.setKeyValue(key, std::to_string(value));			
	}
	void State::removeState(const char* key)
	{
		if (mInitalized) {
			mStateFile.removeKey(key);
		}
	}

	std::string State::readState(const char* key, const std::string& defaultValue)
	{
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, leveldb file couldn't be used");
			return defaultValue;
		}
		std::string tmp;
		if (mStateFile.getValueForKey(key, &tmp)) {
			return tmp;
		}
		return defaultValue;
	}

	int32_t State::readInt32State(const char* key, int32_t defaultValue)
	{
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, leveldb file couldn't be used");
			return defaultValue;
		}
		std::string tmp;
		if (mStateFile.getValueForKey(key, &tmp)) {
			return atoi(tmp.data());
		}
		return defaultValue;
	}
}