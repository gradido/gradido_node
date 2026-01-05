#ifndef __GRADIDO_NODE_SERIALIZATION_STRING_H
#define __GRADIDO_NODE_SERIALIZATION_STRING_H

#include <string>
#include <cstdlib>
#include "gradido_blockchain/memory/Block.h"

namespace serialization {
    template<typename T>
    std::string toString(const T& value);

    template<typename T>
    T fromString(const char* data, size_t size);

    template<typename T>
    concept HasString =
        requires(const T& t, const char* c, size_t size) {
            { toString<T>(t) } -> std::same_as<std::string>;
            { fromString<T>(c, size) } -> std::same_as<T>;
        };
    
    // for string, it's only need's to copy
    template<>
    inline std::string toString<std::string>(const std::string& s) {
        return s;
    }

    template<>
    inline std::string fromString<std::string>(const char* data, size_t size) {
        return std::string(data, size);
    }

    template<>
    inline std::string toString<uint32_t>(const uint32_t& v) {
        return std::to_string(v);
    }

    template<>
    inline uint32_t fromString<uint32_t>(const char* data, size_t size) {
        return static_cast<uint32_t>(strtoul(data, nullptr, 0));
    }

    template<>
    inline std::string toString<size_t>(const size_t& v) {
        return std::to_string(v);
    }

    template<>
    inline size_t fromString<size_t>(const char* data, size_t size) {
        return strtoull(data, nullptr, 0);
    }

    template<>
    inline std::string toString<memory::ConstBlockPtr>(const memory::ConstBlockPtr& ptr) 
    {
        return ptr->copyAsString();
    }

    template<>
    inline memory::ConstBlockPtr fromString<memory::ConstBlockPtr>(const char* data, size_t size)
    {
        return std::make_shared<const memory::Block>(data, size);
    }
}

#endif //__GRADIDO_NODE_SERIALIZATION_STRING_H