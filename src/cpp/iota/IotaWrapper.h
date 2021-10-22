#ifndef __GRADIDO_NODE_IOTA_IOTA_WRAPPER
#define __GRADIDO_NODE_IOTA_IOTA_WRAPPER

/*!
 * @author: einhornimmond
 *
 * @date: 20.10.2021
 *
 * @brief: wrap all iota call for easy replacement for windows with rust calls
 */

#include <vector>
#include <string.h>
#include "../SingletonManager/MemoryManager.h"

namespace iota
{
    struct MessageId
    {
        uint64_t messageId[4];
        const char* groupAlias;

        MessageId() : groupAlias(nullptr) {
            memset(messageId, 0, 4 * sizeof(uint64_t));
        }

        //! operator needed for MessageId as key in unordered map
        bool operator==(const MessageId &other) const {
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
        bool operator<(const MessageId &ob) const {
            return
                messageId[0] < ob.messageId[0] ||
                (messageId[0] == ob.messageId[0] && messageId[1] < ob.messageId[1]) || (
                    messageId[0] == ob.messageId[0] &&
                    messageId[1] == ob.messageId[1] &&
                    messageId[2] < ob.messageId[2]
                ) ||  (
                    messageId[0] == ob.messageId[0] &&
                    messageId[1] == ob.messageId[1] &&
                    messageId[2] == ob.messageId[2] &&
                    messageId[3] < ob.messageId[3]
                );
        }

        void fromByteArray(const char* byteArray);
        MemoryBin* toMemoryBin();

    };

    struct Milestone
    {
        uint32_t id;
        uint64_t timestamp;
        std::vector<MessageId> messageIds;

        Milestone() : timestamp(0) {}
    };

    enum MessageType
    {
        MESSAGE_TYPE_TRANSACTION,
        MESSAGE_TYPE_MILESTONE,
        MESSAGE_TYPE_MAX
    };

    struct NodeInfo
    {
        int32_t lastMilestoneIndex;
        int64_t lastMilestoneTimestamp;
        int32_t confirmedMilestoneIndex;
    };


    //! \brief collect message ids for indexiation from iota
    //! \param indexiation something like for example messages/indexation/{index} or milestones/confirmed
    // full list here: https://client-lib.docs.iota.org/docs/libraries/nodejs/examples#listening-to-mqtt
    //! \return empty vector on error or if nothing couldn't be found
    std::vector<MessageId> getMessageIdsForIndexiation(const std::string& indexiation);

    Milestone getMilestone(MessageId milestoneMessageId);
    std::string getIndexiationMessage(MessageId indexiationMessageId);

    NodeInfo getNodeInfo();

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

#endif //__GRADIDO_NODE_IOTA_IOTA_WRAPPER
