#include "Dictionary.h"
#include "Exceptions.h"
#include "../ServerGlobals.h"
#include "../SystemExceptions.h"
#include "../model/files/FileExceptions.h"

#include "loguru/loguru.hpp"

namespace cache {
	
	Dictionary::Dictionary(std::string_view fileName)
		: mLastIndex(0),
		mDictionaryFile(fileName)
	{
	}

	Dictionary::~Dictionary()
	{
	}

	bool Dictionary::init(size_t cacheInBytes)
	{
		std::lock_guard _lock(mFastMutex);
		if (!mDictionaryFile.init(cacheInBytes)) {
			return false;
		}
		mDictionaryFile.iterate([&](leveldb::Slice key, leveldb::Slice value) -> void {
			char* end = nullptr;
			mValueKeyReverseLookup.insert({ SignatureOctet((const uint8_t*)value.data(), value.size()), strtoul(key.data(), &end, 10) });
		});
		mLastIndex = mValueKeyReverseLookup.size();
		return true;
	}

	void Dictionary::exit()
	{
		mDictionaryFile.exit();
	}

	void Dictionary::reset()
	{
		mDictionaryFile.reset();
		mLastIndex = 0;
	}

	bool Dictionary::addStringIndex(const std::string& string, uint32_t index)
	{
		if (hasStringIndexPair(string, index)) {
			return false;
		}
		std::lock_guard _lock(mFastMutex);
		if (mLastIndex + 1 != index) {
			throw DictionaryInvalidNewKeyException(
				"addValueKeyPair called with invalid new key",
				mDictionaryFile.getFolderName().data(),
				index,
				mLastIndex
			);
		}
		mLastIndex = index;
		mDictionaryFile.setKeyValue(std::to_string(index).c_str(), string);
		mValueKeyReverseLookup.insert({ string, index });

		return true;
	}

	uint32_t Dictionary::getIndexForString(const std::string& string)  const
	{
		auto result = findIndexForString(string);
		if (!result) {
			auto hex = DataTypeConverter::binToHex(string);
			throw DictionaryNotFoundException("string not found in dictionary", mDictionaryFile.getFolderName().data(), hex.data());
		}
		return result;
	}

	std::string Dictionary::getStringForIndex(uint32_t index) const
	{
		std::string value;
		if(!mDictionaryFile.getValueForKey(std::to_string(index).c_str(), &value)) {
			throw DictionaryNotFoundException(
				"index not found in dictionary", 
				mDictionaryFile.getFolderName().data(),
				std::to_string(index).c_str()
			);
		}
		return value;
	}
	bool Dictionary::hasIndex(uint32_t index) const
	{
		std::string value;
		return mDictionaryFile.getValueForKey(std::to_string(index).c_str(), &value);
	}

	uint32_t Dictionary::getOrAddIndexForString(const std::string& string)
	{
		auto index = findIndexForString(string);
		if (!index) {
			std::lock_guard _lock(mFastMutex);
			index = ++mLastIndex;
			mDictionaryFile.setKeyValue(std::to_string(index).c_str(), string);
			mValueKeyReverseLookup.insert({ string, index });
		}
		return index;
	}

	bool Dictionary::hasStringIndexPair(const std::string& string, uint32_t index) const
	{
		std::lock_guard _lock(mFastMutex);
		auto itRange = mValueKeyReverseLookup.equal_range(string);
		if (itRange.first == mValueKeyReverseLookup.end() && itRange.second == mValueKeyReverseLookup.end()) {
			return false;
		}
		for (auto& it = itRange.first; it != itRange.second; ++it) {
			return it->second == index;
		}
		return false;
	}
	 
	uint32_t Dictionary::findIndexForString(const std::string& string) const
	{
		if (string.empty()) {
			throw GradidoNodeInvalidDataException("string is empty");
		}
		std::lock_guard _lock(mFastMutex);
		auto itRange = mValueKeyReverseLookup.equal_range(string);
		for (auto& it = itRange.first; it != itRange.second; ++it) {
			std::string key = std::to_string(it->second);
			std::string value;
			if (mDictionaryFile.getValueForKey(key.c_str(), &value)) {
				if (string == value) {
					return it->second;
				}
			}
		}
		return 0;
	}
}
