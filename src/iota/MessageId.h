#ifndef __GRADIDO_NODE_IOTA_MESSAGE_ID
#define __GRADIDO_NODE_IOTA_MESSAGE_ID

#include <cassert>
#include "../SingletonManager/MemoryManager.h"

namespace iota
{
	class MessageId
	{
	public:
		MessageId();

		//! operator needed for MessageId as key in unordered map
		bool operator==(const MessageId& other) const {
			// I don't use if or for to hopefully speed up the comparisation
			// maybe it can be speed up further with SSE, SSE2 or MMX (only x86 or amd64 not arm or arm64)
			// http://ithare.com/infographics-operation-costs-in-cpu-clock-cycles/
			// https://streamhpc.com/blog/2012-07-16/how-expensive-is-an-operation-on-a-cpu/
			return
				mMessageId[0] == other.mMessageId[0] &&
				mMessageId[1] == other.mMessageId[1] &&
				mMessageId[2] == other.mMessageId[2] &&
				mMessageId[3] == other.mMessageId[3];
		}

		// overload `<` operator to use a `MessageId` object as a key in a `std::map`
		// It returns true if the current object appears before the specified object
		bool operator<(const MessageId& ob) const {
			return
				mMessageId[0] < ob.mMessageId[0] ||
				(mMessageId[0] == ob.mMessageId[0] && mMessageId[1] < ob.mMessageId[1]) || (
					mMessageId[0] == ob.mMessageId[0] &&
					mMessageId[1] == ob.mMessageId[1] &&
					mMessageId[2] < ob.mMessageId[2]
					) || (
						mMessageId[0] == ob.mMessageId[0] &&
						mMessageId[1] == ob.mMessageId[1] &&
						mMessageId[2] == ob.mMessageId[2] &&
						mMessageId[3] < ob.mMessageId[3]
						);
		}

		void fromByteArray(const char* byteArray);
		void fromHex(std::string hex);
		MemoryBin* toMemoryBin();
		std::string toHex() const;
		bool isEmpty() const;

		inline uint64_t getMessageIdByte(uint8_t index) const { assert(index < 4); return mMessageId[index]; };

	protected:
		uint64_t mMessageId[4];
	};
}

// hashing function for message id
// simply use from first 8 Bytes up to size_t size
// should be super fast and enough different for unordered map hashlist because message ids are already hashes
namespace std {
	template <>
	struct hash<iota::MessageId> {
		size_t operator()(const iota::MessageId& k) const {
			return static_cast<size_t>(k.getMessageIdByte(0));
		}
	};
}

#endif //__GRADIDO_NODE_IOTA_MESSAGE_ID