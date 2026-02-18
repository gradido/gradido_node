#include "String.h"
#include "gradido_blockchain/crypto/ByteArray.h"
#include "gradido_blockchain/memory/Block.h"

#include <memory>

using memory::ConstBlockPtr, memory::Block;
using std::string;
using std::make_shared;

namespace serialization {
    template<>
    string toString<ConstBlockPtr>(const ConstBlockPtr& ptr) 
    {
        return ptr->copyAsString();
    }

    template<>
    ConstBlockPtr fromString<ConstBlockPtr>(const char* data, size_t size)
    {
        return make_shared<const Block>(size, reinterpret_cast<const unsigned char*>(data));
    }

    template<>
    string toString<ByteArray<32>>(const ByteArray<32>& bytes)
    {
      return { (char*)bytes.data(), 32 };
    }

    template<>
    ByteArray<32> fromString<ByteArray<32>>(const char* data, size_t size)
    {
      if (size != 32) {
        throw InvalidSizeException("fromString for ByteArray<32> called with non-32 sized string", 32, size);
      }
      return ByteArray<32>(reinterpret_cast<const uint8_t*>(data));
    }
}