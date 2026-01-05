#ifndef __GRADIDO_NODE_PERSISTENT_DICTIONARY_H
#define __GRADIDO_NODE_PERSISTENT_DICTIONARY_H

#include "../model/files/LevelDBWrapper.h"
#include "../serialization/String.h"
#include "gradido_blockchain/lib/DictionaryInterface.h"
#include "gradido_blockchain/lib/DictionaryExceptions.h"

#include "loguru/loguru.hpp"

#include <shared_mutex>
#include <unordered_map>
#include <optional>
#include <cstdlib>


// TODO: remove bi-directionality, update whole code for not using getDataForIndex at all!
template<typename DataType>
requires serialization::HasString<DataType>
class PersistentDictionary: public IMutableDictionary<DataType>
{
public:
    explicit PersistentDictionary(const std::string& directory) : mDictionaryFile(directory) {}
    ~PersistentDictionary() {}

    bool init(size_t cacheInBytes);
    void exit();
    void reset();
    uint32_t getLastIndex();

    virtual std::optional<uint32_t> getIndexForData(const DataType& data) const override;
    virtual std::optional<DataType> getDataForIndex(uint32_t index) const override;
    virtual uint32_t getOrAddIndexForData(const DataType& data) override;

private:
    // LevelDB reads are logically const but mutate internal state
    mutable model::files::LevelDBWrapper mDictionaryFile;
    mutable std::shared_mutex mWorkingMutex;
    std::unordered_map<uint32_t, DataType> mIndexDataReverseLookup;
};


template<typename DataType>
requires serialization::HasString<DataType>
bool PersistentDictionary<DataType>::init(size_t cacheInBytes)
{
    std::unique_lock _lock(mWorkingMutex);
    if (!mDictionaryFile.init(cacheInBytes)) {
        return false;
    }

    // key is DataType, value is uint32
    mDictionaryFile.iterate([&](leveldb::Slice key, leveldb::Slice value) -> void {
        mIndexDataReverseLookup.insert({
            serialization::fromString<uint32_t>(value.data(), value.size()),
            serialization::fromString<DataType>(key.data(), key.size())
        });
    });
    return true;
}

template<typename DataType>
requires serialization::HasString<DataType>
void PersistentDictionary<DataType>::exit()
{
    std::unique_lock _lock(mWorkingMutex);
    mDictionaryFile.exit();
}

template<typename DataType>
requires serialization::HasString<DataType>
void PersistentDictionary<DataType>::reset()
{
    std::unique_lock _lock(mWorkingMutex);
    mDictionaryFile.reset();
}

template<typename DataType>
requires serialization::HasString<DataType>
uint32_t PersistentDictionary<DataType>::getLastIndex()
{
    std::unique_lock _lock(mWorkingMutex);
    mIndexDataReverseLookup.size() - 1;
}

template<typename DataType>
requires serialization::HasString<DataType>
std::optional<uint32_t> PersistentDictionary<DataType>::getIndexForData(const DataType& data) const
{
    std::shared_lock _lock(mWorkingMutex);
    auto result = mDictionaryFile.getValueForKey(serialization::toString(data).c_str());
    if (result.has_value()) {
        const auto& value = result.value();
        return serialization::fromString<uint32_t>(value.data(), value.size());
    }
    return std::nullopt;
}

template<typename DataType>
requires serialization::HasString<DataType>
std::optional<DataType> PersistentDictionary<DataType>::getDataForIndex(uint32_t index) const
{
    std::shared_lock _lock(mWorkingMutex);
    auto it = mIndexDataReverseLookup.find(index);
    if (it == mIndexDataReverseLookup.end()) {
        return std::nullopt;
    }
    return it->second;
}

template<typename DataType>
requires serialization::HasString<DataType>
uint32_t PersistentDictionary<DataType>::getOrAddIndexForData(const DataType& data)
{
    auto dataString = serialization::toString<DataType>(data);
    std::unique_lock _lock(mWorkingMutex);
    auto result = mDictionaryFile.getValueForKey(dataString);
    if (result.has_value()) {
        const auto& value = result.value();
        return serialization::fromString<uint32_t>(value.data(), value.size());
    }
    if (mIndexDataReverseLookup.size() >= static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
        throw DictionaryOverflowException("try to add more index data set's as uint32_t as index can handle", mDictionaryFile.getFolderName());
    }
    uint32_t index = static_cast<uint32_t>(mIndexDataReverseLookup.size());
    mDictionaryFile.setKeyValue(dataString, serialization::toString<uint32_t>(index));
    mIndexDataReverseLookup.insert({ index, data });
    return index;
}

#endif //__GRADIDO_NODE_PERSISTENT_DICTIONARY_H