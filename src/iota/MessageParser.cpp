#include "MessageParser.h"
#include "../SingletonManager/LoggerManager.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "sodium.h"

// basis message structur

namespace iota {
  MessageParser::MessageParser(const void *data, size_t size)
  {
    calculateMessageId(data, size);
    size_t offset = 0;
    // network protocol
    //memcpy(&mMessage.protocol_version, data + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    // parents
    uint8_t parentCount = 0;
    memcpy(&parentCount, data + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);

    //mMessage.parents.reserve(parentCount);
    for (size_t i = 0; i < parentCount; i++)
    {
      /*MessageId id;
      id.fromByteArray(data + offset);
      mMessage.parents.push_back(id);
      */
      offset += 32;
    }

    // payload
    uint32_t payloadSize;
    memcpy(&payloadSize, data + offset, sizeof(payloadSize));
    offset += sizeof(payloadSize);

    uint32_t payload_type = 0;
    memcpy(&payload_type, data + offset, sizeof(payload_type));
    offset += sizeof(payload_type);
    auto mm = MemoryManager::getInstance();

    if (payload_type == 2)
    {
      //indexation_t indexStruc;
      // indexation
      uint16_t indexSize;
      memcpy(&indexSize, data + offset, sizeof(uint16_t));
      offset += sizeof(uint16_t);

      // index
      //auto indexHex = DataTypeConverter::binToHex((const unsigned char*)data + offset, indexSize);
      //printf("hex indexation index: %s\n", indexHex.data());
      offset += indexSize;

      uint32_t dataSize;
      memcpy(&dataSize, data + offset, sizeof(uint32_t));
      offset += sizeof(uint32_t);

      mTransaction = std::make_unique<model::gradido::GradidoTransaction>(data + offset, dataSize);

      //auto hexData = DataTypeConverter::binToHex((const unsigned char*)data + offset, dataSize);
      //printf("hex indexation data: %s\n", hexData.data());
      
      offset += dataSize;
    }
    else if (payload_type == 7)
    {
      // milestone
      LoggerManager::getInstance()->mErrorLogging.error("milestone must be implemented: %d", mMessage.payload_type);
      return;
    }
    else
    {
      LoggerManager::getInstance()->mErrorLogging.error("not supported iota message payload type: %d", mMessage.payload_type);
      return;
    }

    // memcpy(&mMessage.nonce, data + offset, sizeof(uint64_t));
  }

  void MessageParser::print()
  {
    printf("[MessageParser]\n");
    printf("message id: %s\n", mMessageId.toHex().data());

  }

  void MessageParser::calculateMessageId(const void *data, size_t size)
  {
    // calculate iota message id from serialized message
		// it is simply a BLAKE2b-256 hash (https://tools.ietf.org/html/rfc7693)
    auto mm = MemoryManager::getInstance();
    auto hash = mm->getMemory(crypto_generichash_BYTES);
    crypto_generichash(hash->data(), crypto_generichash_BYTES, (const unsigned char*)data, size, nullptr, 0);
    mMessageId.fromMemoryBin(hash);
    mm->releaseMemory(hash);
  }
}