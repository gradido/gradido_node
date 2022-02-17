#include "MessageId.h"


#include "../lib/BinTextConverter.h"
#include "../lib/DataTypeConverter.h"

namespace iota {
	MessageId::MessageId() 
	{
		memset(mMessageId, 0, 4 * sizeof(uint64_t));
	}

	void MessageId::fromByteArray(const char* byteArray)
	{
		assert(byteArray);
		memcpy(mMessageId, byteArray, 32);
	}

	void MessageId::fromHex(std::string hex)
	{
		auto binString = convertHexToBin(hex);
		fromByteArray(binString.data());
	}

	MemoryBin* MessageId::toMemoryBin()
	{
		auto result = MemoryManager::getInstance()->getFreeMemory(32);
		memcpy(result->data(), mMessageId, 32);
		return result;
	}

	std::string MessageId::toHex() const
	{
		return DataTypeConverter::binToHex((unsigned char*)mMessageId, 32).substr(0, 64);
	}

	bool MessageId::isEmpty() const
	{
		return
			mMessageId[0] == 0 &&
			mMessageId[1] == 0 &&
			mMessageId[2] == 0 &&
			mMessageId[3] == 0;
	}
}