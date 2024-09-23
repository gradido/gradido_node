#include "MessageParser.h"
#include "../SingletonManager/LoggerManager.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/interaction/deserialize/Context.h"
#include "sodium.h"

// basis message structure
using namespace gradido;
using namespace data;
using namespace interaction;

namespace iota {
    MessageParser::MessageParser(const void *data, size_t size)
        : mType(MessageType::INDEXIATION_MESSAGE), mMilestoneId(0)
    {
        const char* dataChar = (const char*)data;
        calculateMessageId(data, size);

        // raw code work only on Linux, not on windows, maybe because little endian/big endian issue
        return;
        size_t offset = 0;
        // network protocol
        //memcpy(&mMessage.protocol_version, data + offset, sizeof(uint8_t));
        offset += sizeof(uint8_t);

        // parents
        uint8_t parentCount = 0;
        memcpy(&parentCount, &dataChar[offset], sizeof(uint8_t));
        offset += sizeof(uint8_t);

        //mMessage.parents.reserve(parentCount);
        for (size_t i = 0; i < parentCount; i++)
        {
            //MessageId id;
            //id.fromByteArray((const char*)(&dataChar[offset]));
            //printf("parent message id (%d): %s\n", (int)i, id.toHex().data());
            //mMessage.parents.push_back(id);
            //*/
            offset += 32;
        }
        //printf("offset: %d\n", (int)offset);

        // payload
        uint32_t payloadSize;
        memcpy(&payloadSize, &dataChar[offset], sizeof(payloadSize));
        offset += sizeof(payloadSize);

        uint32_t payload_type = 0;
        memcpy(&payload_type, &dataChar[offset], sizeof(payload_type));
        offset += sizeof(payload_type);

        if (payload_type == 2)
        {
            mType = MessageType::INDEXIATION_MESSAGE;
            //indexation_t indexStruc;
            // indexation
            uint16_t indexSize;
            memcpy(&indexSize, &dataChar[offset], sizeof(uint16_t));
            offset += sizeof(uint16_t);

            // index
            //auto indexHex = DataTypeConverter::binToHex((const unsigned char*)data + offset, indexSize);
            //printf("hex indexation index: %s\n", indexHex.data());
            offset += indexSize;

            uint32_t dataSize;
            memcpy(&dataSize, &dataChar[offset], sizeof(uint32_t));
            offset += sizeof(uint32_t);

            auto serializedTransaction = std::make_shared<memory::Block>(dataSize, (const unsigned char*)&dataChar[offset]);
            deserialize::Context deserializer(serializedTransaction, deserialize::Type::GRADIDO_TRANSACTION);
            assert(deserializer.isGradidoTransaction());
            mTransaction = deserializer.getGradidoTransaction();

            //auto hexData = DataTypeConverter::binToHex((const unsigned char*)data + offset, dataSize);
            //printf("hex indexation data: %s\n", hexData.data());
      
            offset += dataSize;
        }
        else if (payload_type == 7)
        {
            // milestone
            LoggerManager::getInstance()->mErrorLogging.error("milestone must be implemented: %ud", payload_type);
            return;
        }
        else if (payload_type == 13)
        {
            LoggerManager::getInstance()->mErrorLogging.error("payload type 13");
            return;
        }
        else
        {
            LoggerManager::getInstance()->mErrorLogging.error("not supported iota message payload type: %ud", payload_type);
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
        memory::Block hash(crypto_generichash_BYTES);
        crypto_generichash(hash, crypto_generichash_BYTES, (const unsigned char*)data, size, nullptr, 0);
        mMessageId = MessageId(hash);
    }
}