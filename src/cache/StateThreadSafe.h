#ifndef __GRADIDO_NODE_CACHE_STATE_THREAD_SAFE_H
#define __GRADIDO_NODE_CACHE_STATE_THREAD_SAFE_H

#include "State.h"

#include <mutex>

namespace cache {
	/*!
	* @brief simply overload all functions from State and lock a mutex beforehand
	*/
	class StateThreadSafe : public State
	{
	public:
		using State::State;

		// try to open db 
		bool init(size_t cacheInBytes) 
		{ 
			std::lock_guard _lock(mFastMutex);
			return State::init(cacheInBytes);
		}
		void exit() 
		{ 
			std::lock_guard _lock(mFastMutex);
			State::exit();
		}

		void updateState(const char* key, std::string_view value) 
		{
			std::lock_guard _lock(mFastMutex);
			State::updateState(key, value);
		}
		void updateState(DefaultStateKeys key, std::string_view value)
		{
			return updateState(magic_enum::enum_name(key).data(), value);
		}

		void updateState(const char* key, int32_t value) 
		{
			std::lock_guard _lock(mFastMutex);
			State::updateState(key, value);
		}

		void updateState(DefaultStateKeys key, int32_t value)
		{
			return updateState(magic_enum::enum_name(key).data(), value);
		}

		const std::string readState(const char* key, const std::string& defaultValue) 
		{
			std::lock_guard _lock(mFastMutex);
			return State::readState(key, defaultValue);
		}
		const std::string readState(DefaultStateKeys key, const std::string& defaultValue)
		{
			return readState(magic_enum::enum_name(key).data(), defaultValue);
		}
		int32_t readInt32State(const char* key, int32_t defaultValue)
		{
			std::lock_guard _lock(mFastMutex);
			return State::readInt32State(key, defaultValue);
		}
		int32_t readInt32State(DefaultStateKeys key, int32_t defaultValue)
		{
			return readInt32State(magic_enum::enum_name(key).data(), defaultValue);
		}

	protected:
		std::mutex mFastMutex;
	};

	

	
}

#endif //__GRADIDO_NODE_CACHE_STATE_THREAD_SAFE_H