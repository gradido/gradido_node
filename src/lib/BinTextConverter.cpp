#include "BinTextConverter.h"

#include "gradido_blockchain/memory/Block.h"

#include "sodium.h"
#include "libbase58.h"

// additional header for linux
#include <cstring>
#include <cmath>
#include <regex>

static std::regex regExpIsHexString("^[a-fA-F0-9]*$");
static std::regex regExpIsBase58String("^[1-9A-HJ-NP-Za-km-z]*$");
static std::regex regExpIsBase64String("^[0-9A-Za-z+/]*=*$");


StringTransportType getStringTransportType(const std::string& input)
{
	if (std::regex_match(input, regExpIsHexString)) {
		return STRING_TRANSPORT_TYPE_HEX;
	}
	else if (std::regex_match(input, regExpIsBase58String)) {
		return STRING_TRANSPORT_TYPE_BASE58;
	}
	else if (std::regex_match(input, regExpIsBase64String)) {
		return STRING_TRANSPORT_TYPE_BASE64;
	}
	
	return STRING_TRANSPORT_TYPE_UNKNOWN;
}


std::string convertBinTransportStringToBin(const std::string& input)
{
	auto type = getStringTransportType(input);
	switch (type) {
	case STRING_TRANSPORT_TYPE_BASE58: return convertBase58ToBin(input);
	case STRING_TRANSPORT_TYPE_BASE64: return convertBase64ToBin(input);
	case STRING_TRANSPORT_TYPE_HEX: return convertHexToBin(input);
	}
	return "";
}

std::string convertHexToBin(const std::string& hexString)
{
	auto block = memory::Block::fromHex(hexString);
	return block.copyAsString();
}

std::string convertBase64ToBin(const std::string& base64String)
{
	auto block = memory::Block::fromBase64(base64String);
	return block.copyAsString();
}

std::string convertBase58ToBin(const std::string& base58String)
{
	/*
	Decoding Base58

	Simply allocate a buffer to store the binary data in, and set a variable with the buffer size, and call the b58tobin function:

	bool b58tobin(void *bin, size_t *binsz, const char *b58, size_t b58sz)

	The "canonical" base58 byte length will be assigned to binsz on success, which may be larger than the actual buffer if the input has many leading zeros. Regardless of the canonical byte length, the full binary buffer will be used. If b58sz is zero, it will be initialised with strlen(b58); note that a true zero-length base58 string is not supported here.
	*/

	size_t encodedSize = base58String.size();
	size_t binSize = (size_t)ceil(((double)encodedSize / 4.0) * 3.0);
	memory::Block bin(binSize);

	//size_t resultBinSize = 0;
	if (!b58tobin(bin, &binSize, base58String.data(), encodedSize)) {
		return "";
	}

	std::string binString((const char*)*bin, binSize);
	return binString;
}

std::string convertBinToBase58(const std::string& binString)
{
	//extern bool b58enc(char *b58, size_t *b58sz, const void *bin, size_t binsz);

	size_t binSize = binString.size();
	size_t encodedSize = binSize * 138 / 100 + 1;
	memory::Block encoded(encodedSize);
	memset(encoded, 0, encodedSize);

	if (!b58enc((char*)encoded.data(), &encodedSize, binString.data(), binSize)) {
		return "";
	}
	std::string base58String((const char*)*encoded, encodedSize-1);
	
	return base58String;
}

std::string convertBinToBase64(const std::string& binString)
{
	auto block = memory::Block(binString);
	return block.convertToBase64();
}

std::string convertBinToHex(const std::string& binString)
{
	auto block = memory::Block(binString);
	return block.convertToHex();
}
