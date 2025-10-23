#ifndef __GRADIDO_NODE_HIERO_PROTOPUF_H
#define __GRADIDO_NODE_HIERO_PROTOPUF_H

/*
#include <string>
#include <bit>
#include "protopuf/message.h"
*/

#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/memory/Block.h"
#include "loguru/loguru.hpp"
#include <array>

namespace protopuf {
	template<class T, class Message>
	memory::Block serialize(const T& obj) 
	{
		array<std::byte, T::maxSize()> buffer;
		auto message = obj.getMessage();
		auto bufferEnd = message_coder<Message>::encode(message, buffer);
		if (!bufferEnd.has_value()) {
			throw GradidoNodeInvalidDataException("couldn't serialize");
		}
		auto bufferEndValue = bufferEnd.value();
		auto actualSize = pp::begin_diff(bufferEndValue, buffer);
		if (!actualSize) {
			throw GradidoNodeInvalidDataException("shouldn't be possible");
		}
		return memory::Block(actualSize, (const uint8_t*)buffer.data());
	}

	template<class T, class Message>
	T deserialize(const memory::Block& raw)
	{
		Profiler timeUsed;
		auto result = pp::message_coder<Message>::decode(raw.span());
		if (!result.has_value()) {
			// TODO: check if using exception is better
			LOG_F(ERROR, "couldn't be deserialized: echo \"%s\" | xxd -r -p | protoscope", raw.convertToHex().data());
			return T(); 
		}
		else {
			const auto& [message, bufferEnd2] = *result;
			LOG_F(1, "timeUsed for deserialize: %s", timeUsed.string().data());
			return T(message);
		}
	}
}

#endif