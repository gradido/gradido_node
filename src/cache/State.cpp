#include "State.h"
#include "../SystemExceptions.h"
#include "../lib/LevelDBExceptions.h"

#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"

#include <string_view>

using namespace magic_enum;
using std::string_view;

namespace cache {
	State::State(std::string_view folder)
		: mInitalized(false),
		mStateFile(folder)
	{
		mFastAccessDefaultStates.resize(static_cast<size_t>(DefaultStateKeys::MAX), 0);
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
		// fill fast access default states vector
		mStateFile.iterate(
			[&](leveldb::Slice key, leveldb::Slice value)
			{
				auto state = enum_cast<DefaultStateKeys>(string_view(key.data(), key.size()));
				if (state.has_value()) {
					assert(state.value() < DefaultStateKeys::MAX);
					mFastAccessDefaultStates[static_cast<size_t>(state.value())] = strtoll(value.data(), nullptr, 10);
				}
			}
		);
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

	void State::updateState(const char* key, uint32_t value)
	{
		if (!mInitalized) {
			throw ClassNotInitalizedException("cannot update state, state wasn't initalized", "cache::State");
		}

		mStateFile.setKeyValue(key, std::to_string(value));
	}

	void State::updateState(const char* key, int64_t value)
	{
		if (!mInitalized) {
			throw ClassNotInitalizedException("cannot update state, state wasn't initalized", "cache::State");
		}

		mStateFile.setKeyValue(key, std::to_string(value));
	}

	void State::updateState(const char* key, uint64_t value)
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
		auto result = mStateFile.getValueForKey(key);
		if (result.has_value()) {
			return result.value();
		}
		return defaultValue;
	}

	int32_t State::readInt32State(const char* key, int32_t defaultValue)
	{
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, leveldb file couldn't be used");
			return defaultValue;
		}
		auto result = mStateFile.getValueForKey(key);
		if (result.has_value()) {
			return atoi(result.value().data());
		}
		return defaultValue;
	}

	int64_t State::readInt64State(const char* key, int64_t defaultValue)
	{
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, leveldb file couldn't be used");
			return defaultValue;
		}
		auto result = mStateFile.getValueForKey(key);
		if (result.has_value()) {
			return strtoll(result.value().data(), nullptr, 10);
		}
		return defaultValue;
	}
}