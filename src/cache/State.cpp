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

	bool State::init()
	{
		if (mInitalized) {
			throw ClassAlreadyInitalizedException("init was already called", "cache::State");
		}
		if (!mStateFile.init()) {
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
		mIntStates.clear();
		mStates.clear();
	}

	void State::updateState(const char* key, std::string_view value)
	{
		auto it = mStates.find(key);
		if (it == mStates.end()) {
			mStates.insert({ std::string(key), std::string(value) }); 
		}
		else {
			it->second = value;
		}
		if (mInitalized) {
			mStateFile.setKeyValue(key, std::string(value));
		}			
	}


	void State::updateState(const char* key, int32_t value)
	{
		auto it = mIntStates.find(key);
		if (it == mIntStates.end()) {
			mIntStates.insert({ std::string(key), value });
		}
		else {
			it->second = value;
		}
		if (mInitalized) {
			mStateFile.setKeyValue(key, std::to_string(value));
		}			
	}

	const std::string& State::readState(const char* key, const std::string& defaultValue)
	{
		auto it = mStates.find(key);
		if (it != mStates.end()) {
			return it->second;
		}
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, leveldb file couldn't be used");
			return defaultValue;
		}
		std::string tmp;
		if (mStateFile.getValueForKey(key, &tmp)) {
			mStates.insert({ std::string(key), tmp }).first->second;
		}
		return defaultValue;
	}

	int32_t State::readInt32State(const char* key, int32_t defaultValue)
	{
		auto it = mIntStates.find(key);
		if (it != mIntStates.end()) {
			return it->second;
		}
		if (!mInitalized) {
			LOG_F(WARNING, "init wasn't called, leveldb file couldn't be used");
			return defaultValue;
		}
		std::string tmp;
		if (mStateFile.getValueForKey(key, &tmp)) {
			int32_t value = atoi(tmp.data());
			mIntStates.insert({ std::string(key), value });
			return value;
		}
		return defaultValue;
	}

	void State::readAllStates(std::function<void(const std::string&, const std::string&)> callback)
	{
		auto it = mStateFile.getIterator();
		assert(it);
		for (it->SeekToFirst(); it->Valid(); it->Next()) {
			callback(it->key().ToString(), it->value().ToString());
		}
		if (!it->status().ok()) {
			throw LevelDBStatusException("error in iterate over leveldb entries", it->status());
		}
		delete it;
	}
}