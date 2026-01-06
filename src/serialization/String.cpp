#include "String.h"
#include "gradido_blockchain/memory/Block.h"

#include <memory>

using memory::ConstBlockPtr, memory::Block;
using std::string;
using std::make_shared;

namespace serialization {
    template<>
    string toString<memory::ConstBlockPtr>(const ConstBlockPtr& ptr) 
    {
        return ptr->copyAsString();
    }

    template<>
    ConstBlockPtr fromString<ConstBlockPtr>(const char* data, size_t size)
    {
        return make_shared<const Block>(size, reinterpret_cast<const unsigned char*>(data));
    }
}