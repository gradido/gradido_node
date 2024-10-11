#ifndef __GRADIDO_NODE_CACHE_STATE_H
#define __GRADIDO_NODE_CACHE_STATE_H

#include "DefaultStateKeys.h"
#include "../model/files/State.h"

#include "magic_enum/magic_enum.hpp"

#include <string_view>

namespace cache {

	class State 
	{
	public:
		State(std::string_view baseFolder);
		virtual ~State();

		// try to open db 
		bool init();
		void exit();
		//! remove state level db folder, clear maps
		void reset();

		inline void updateState(DefaultStateKeys key, std::string_view value);
		inline void updateState(DefaultStateKeys key, int32_t value);
		void updateState(const char* key, std::string_view value);
		void updateState(const char* key, int32_t value);

		//! first check if value for key is already in memory else read from file via leveldb
		inline const std::string& readState(DefaultStateKeys key, const std::string& defaultValue);
		const std::string& readState(const char* key, const std::string& defaultValue);
		inline int32_t readInt32State(DefaultStateKeys key, int32_t defaultValue);
		int32_t readInt32State(const char* key, int32_t defaultValue);

		//! go through all states in call callback for each with key, value
		//! don't put key value pairs into map
		void readAllStates(std::function<void(const std::string&, const std::string&)> callback);

	protected:
		bool mInitalized;
		model::files::State mStateFile;

		std::unordered_map<std::string, int32_t> mIntStates;
		std::unordered_map<std::string, std::string> mStates;

	};

	// simple functions for inline declarations
	void State::updateState(DefaultStateKeys key, std::string_view value)
	{
		return updateState(magic_enum::enum_name(key).data(), value);
	}

	void State::updateState(DefaultStateKeys key, int32_t value)
	{
		return updateState(magic_enum::enum_name(key).data(), value);
	}

	const std::string& State::readState(DefaultStateKeys key, const std::string& defaultValue)
	{
		return readState(magic_enum::enum_name(key).data(), defaultValue);
	}

	int32_t State::readInt32State(DefaultStateKeys key, int32_t defaultValue)
	{
		return readInt32State(magic_enum::enum_name(key).data(), defaultValue);
	}
}

#endif //__GRADIDO_NODE_CACHE_STATE_H