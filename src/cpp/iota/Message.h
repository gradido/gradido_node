#ifndef __GRADIDO_NODE_IOTA_MESSAGE 
#define __GRADIDO_NODE_IOTA_MESSAGE 

#include "../SingletonManager/MemoryManager.h"
#include "../task/CPUTask.h"

#include "Poco/SharedPtr.h"

#include "rapidjson/document.h"

namespace iota {

	struct MessageId
	{
		uint64_t messageId[4];
		const char* groupAlias;

		MessageId() : groupAlias(nullptr) {
			memset(messageId, 0, 4 * sizeof(uint64_t));
		}

		//! operator needed for MessageId as key in unordered map
		bool operator==(const MessageId& other) const {
			// I don't use if or for to hopefully speed up the comparisation
			// maybe it can be speed up further with SSE, SSE2 or MMX (only x86 or amd64 not arm or arm64)
			// http://ithare.com/infographics-operation-costs-in-cpu-clock-cycles/
			// https://streamhpc.com/blog/2012-07-16/how-expensive-is-an-operation-on-a-cpu/
			return
				messageId[0] == other.messageId[0] &&
				messageId[1] == other.messageId[1] &&
				messageId[2] == other.messageId[2] &&
				messageId[3] == other.messageId[3];
		}

		// overload `<` operator to use a `MessageId` object as a key in a `std::map`
		// It returns true if the current object appears before the specified object
		bool operator<(const MessageId& ob) const {
			return
				messageId[0] < ob.messageId[0] ||
				(messageId[0] == ob.messageId[0] && messageId[1] < ob.messageId[1]) || (
					messageId[0] == ob.messageId[0] &&
					messageId[1] == ob.messageId[1] &&
					messageId[2] < ob.messageId[2]
					) || (
						messageId[0] == ob.messageId[0] &&
						messageId[1] == ob.messageId[1] &&
						messageId[2] == ob.messageId[2] &&
						messageId[3] < ob.messageId[3]
						);
		}

		void fromByteArray(const char* byteArray);
		void fromHex(std::string hex);
		MemoryBin* toMemoryBin();
		std::string toHex() const;
		bool isEmpty() const;

	};

	enum MessageType
	{
		MESSAGE_TYPE_TRANSACTION,
		MESSAGE_TYPE_MILESTONE,
		MESSAGE_TYPE_MAX
	};


	class Message
	{
	public:
		Message(MessageId id);
		~Message();

		// use message id operators for using in maps
		bool operator==(const Message& other) const {
			return mId == other.mId;
		}
		bool operator<(const Message& ob) const {
			return mId < ob.mId;
		}

		inline MessageType getType() const { return mType; }
		inline const MessageId& getId() const { return mId; }
		inline int32_t getMilestoneId() const { return mMilestoneId; }
		inline int64_t getMilestoneTimestamp() const { return mMilestoneTimestamp; }
		inline std::string getDataString() const { return std::string((const char*)mData->data(), mData->size()); }
		inline bool isFetched() const { return mFetched; }
		bool isGradidoTransaction() const;
		std::string getGradidoGroupAlias() const;

		const std::vector<MessageId>& getParentMessageIds() const { return mParentMessageIds; }

		static MessageType fromIotaTransactionType(uint32_t iotaTransactionType);

		bool loadFromJson(rapidjson::Document& json);
		bool requestFromIota();

	protected:
		MessageId mId;
		MessageType mType;
		std::vector<MessageId> mParentMessageIds;
		MemoryBin* mData;
		std::string mIndex;
		int32_t mMilestoneId;
		int64_t mMilestoneTimestamp;
		bool mFetched;
	};

	/*
	* start from milestone, load recursive the parents and hand over gradido transactions to ordering Manager
	*/
	class ConfirmedMessageLoader : public UniLib::controller::CPUTask
	{
	public:
		// every iota message has at least two parent messages
		// with recursion messages will be loaded recursively
		// command will be needed for retrieving results
		ConfirmedMessageLoader(Poco::SharedPtr<Message> rootMilestone, uint8_t targetRecursionDeep = 0, uint8_t recursionDeep = 0);
		ConfirmedMessageLoader(MessageId id, Poco::SharedPtr<Message> rootMilestone, uint8_t targetRecursionDeep = 0, uint8_t recursionDeep = 0);
		~ConfirmedMessageLoader();

		const char* getResourceType() const { return "iota::ConfirmedMessageLoader"; }
		int run();

		inline const Poco::SharedPtr<Message> getMessage() const { return mMessage; }
		inline uint8_t getTargetRecursionDeep() const { return mTargetRecursionDeep; }
		inline uint8_t getRecursionDeep() const { return mRecursionDeep; }
		inline const Poco::SharedPtr<Message> getRootMilestone() const { return mRootMilestone; }

	protected:
		 Poco::SharedPtr<Message> mMessage;		 
		 Poco::SharedPtr<Message> mRootMilestone;
		 uint8_t mTargetRecursionDeep;
		 uint8_t mRecursionDeep;
	};
}

// hashing function for message id
// simply use from first 8 Bytes up to size_t size
// should be super fast and enough different for unordered map hashlist because message ids are already hashes
namespace std {
	template <>
	struct hash<iota::MessageId> {
		size_t operator()(const iota::MessageId& k) const {
			return static_cast<size_t>(k.messageId[0]);
		}
	};
}

#endif //__GRADIDO_NODE_IOTA_MESSAGE 