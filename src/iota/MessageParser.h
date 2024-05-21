#ifndef __GRADIDO_NODE_IOTA_MESSAGE_PARSER_H
#define __GRADIDO_NODE_IOTA_MESSAGE_PARSER_H

#include "gradido_blockchain/MemoryManager.h"
#include "gradido_blockchain/model/TransactionEntry.h"
#include "MessageId.h"

#include <memory>
#include <vector>

namespace iota {

  // from https://github.com/iotaledger/iota.c
  struct milestone_t
  {
    uint32_t type;  // Must be set to 1
    uint64_t index; // The index number of the milestone.
    uint64_t timestamp;                     // The Unix timestamp at which the milestone was issued. The unix timestamp is specified in seconds.
    uint8_t inclusion_merkle_proof[64]; // Specifies the merkle proof which is computed out of all the tail transaction
                                       // hashes of all the newly confirmed state-mutating bundles.
    uint8_t signature[64]; // The signature signing the entire message excluding the nonce and the signature itself.
  };

  struct core_message_t
  {
    core_message_t() : payload(nullptr) {}
    uint8_t protocol_version; ///< Network identifier. It is first 8 bytes of the `BLAKE2b-256` hash of the concatenation of
                                                         ///< the network type and the protocol version string.
    std::vector<MessageId> parents; ///< parents of this message
    uint32_t payload_type; ///< payload type
    void *payload;          ///< payload object, NULL is no payload
    uint64_t nonce;         ///< The nonce which lets this message fulfill the Proof-of-Work requirement.
  };

  enum class MessageType {
      INDEXIATION_MESSAGE,
      MESSAGE_METADATA
  };

  class MessageParser
  {
  public:
    MessageParser(const void* data, size_t size);

    void print();

    inline std::unique_ptr<model::gradido::GradidoTransaction> getTransaction() {return std::move(mTransaction);}
    inline MessageId getMessageId() const { return mMessageId;}
    inline MessageType getMessageType() const { return mType; }
    inline int32_t getMilestoneId() const { return mMilestoneId; }

  protected:
    // core_message_t mMessage;
    std::unique_ptr<model::gradido::GradidoTransaction> mTransaction;
    MessageId mMessageId;
    MessageType mType;
    int32_t mMilestoneId;

  private: 
    void calculateMessageId(const void *data, size_t size);
  };
}

#endif