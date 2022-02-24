#include "BinTextConverter.h"

#include "Poco/RegularExpression.h"

#include "sodium.h"
#include "libbase58.h"

#include "gradido_blockchain/MemoryManager.h"

// additional header for linux
#include <cstring>
#include <cmath>

static Poco::RegularExpression regExpIsHexString("^[a-fA-F0-9]*$");
static Poco::RegularExpression regExpIsBase58String("^[1-9A-HJ-NP-Za-km-z]*$");
static Poco::RegularExpression regExpIsBase64String("^[0-9A-Za-z+/]*=*$");


StringTransportType getStringTransportType(const std::string& input)
{
	if (regExpIsHexString.match(input)) {
		return STRING_TRANSPORT_TYPE_HEX;
	}
	else if (regExpIsBase58String.match(input)) {
		return STRING_TRANSPORT_TYPE_BASE58;
	}
	else if (regExpIsBase64String.match(input)) {
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
	/*
	int sodium_hex2bin(unsigned char * const bin, const size_t bin_maxlen,
	const char * const hex, const size_t hex_len,
	const char * const ignore, size_t * const bin_len,
	const char ** const hex_end);

	The sodium_hex2bin() function parses a hexadecimal string hex and converts it to a byte sequence.

	hex does not have to be null terminated, as the number of characters to parse is supplied via the hex_len parameter.

	ignore is a string of characters to skip. For example, the string ": " allows columns and spaces to be present at any locations in the hexadecimal string. These characters will just be ignored. As a result, "69:FC", "69 FC", "69 : FC" and "69FC" will be valid inputs, and will produce the same output.

	ignore can be set to NULL in order to disallow any non-hexadecimal character.

	bin_maxlen is the maximum number of bytes to put into bin.

	The parser stops when a non-hexadecimal, non-ignored character is found or when bin_maxlen bytes have been written.

	If hex_end is not NULL, it will be set to the address of the first byte after the last valid parsed character.

	The function returns 0 on success.

	It returns -1 if more than bin_maxlen bytes would be required to store the parsed string, or if the string couldn't be fully parsed, but a valid pointer for hex_end was not provided.

	It evaluates in constant time for a given length and format.
	*/

	auto mm = MemoryManager::getInstance();
	size_t hexSize = hexString.size();
	size_t binSize = (hexSize) / 2;
	MemoryBin* bin = mm->getMemory(binSize);
	memset(*bin, 0, binSize);

	size_t resultBinSize = 0;

	if (0 != sodium_hex2bin(*bin, binSize, hexString.data(), hexSize, nullptr, &resultBinSize, nullptr)) {
		mm->releaseMemory(bin);
		return "";
	}
	std::string binString((const char*)*bin, resultBinSize);
	mm->releaseMemory(bin);
	return binString;

}

std::string convertBase64ToBin(const std::string& base64String)
{
	/*
	int sodium_base642bin(unsigned char * const bin, const size_t bin_maxlen,
	const char * const b64, const size_t b64_len,
	const char * const ignore, size_t * const bin_len,
	const char ** const b64_end, const int variant);

	sodium_base64_VARIANT_ORIGINAL
	*/
	auto mm = MemoryManager::getInstance();
	size_t encodedSize = base64String.size();
	size_t binSize = (encodedSize / 4) * 3;
	auto bin = mm->getMemory(binSize);
	memset(*bin, 0, binSize);

	size_t resultBinSize = 0;

	if (0 != sodium_base642bin(*bin, binSize, base64String.data(), encodedSize, nullptr, &resultBinSize, nullptr, sodium_base64_VARIANT_ORIGINAL)) {
		mm->releaseMemory(bin);
		return "";
	}

	std::string binString((const char*)*bin, resultBinSize);
	mm->releaseMemory(bin);
	return binString;
}

std::string convertBase58ToBin(const std::string& base58String)
{
	/*
	Decoding Base58

	Simply allocate a buffer to store the binary data in, and set a variable with the buffer size, and call the b58tobin function:

	bool b58tobin(void *bin, size_t *binsz, const char *b58, size_t b58sz)

	The "canonical" base58 byte length will be assigned to binsz on success, which may be larger than the actual buffer if the input has many leading zeros. Regardless of the canonical byte length, the full binary buffer will be used. If b58sz is zero, it will be initialised with strlen(b58); note that a true zero-length base58 string is not supported here.
	*/

	auto mm = MemoryManager::getInstance();
	size_t encodedSize = base58String.size();
	size_t binSize = (size_t)ceil(((double)encodedSize / 4.0) * 3.0);
	auto bin = mm->getMemory(binSize);
	memset(*bin, 0, binSize);

	//size_t resultBinSize = 0;
	if (!b58tobin(*bin, &binSize, base58String.data(), encodedSize)) {
		mm->releaseMemory(bin);
		return "";
	}

	std::string binString((const char*)*bin, binSize);
	mm->releaseMemory(bin);
	return binString;

}

std::string convertBinToBase58(const std::string& binString)
{
	//extern bool b58enc(char *b58, size_t *b58sz, const void *bin, size_t binsz);

	auto mm = MemoryManager::getInstance();
	size_t binSize = binString.size();
	size_t encodedSize = binSize * 138 / 100 + 1;
	auto encoded = mm->getMemory(encodedSize);
	memset(*encoded, 0, encodedSize);

	if (!b58enc(*encoded, &encodedSize, binString.data(), binSize)) {
		mm->releaseMemory(encoded);
		return "";
	}
	std::string base58String((const char*)*encoded, encodedSize-1);
	
	mm->releaseMemory(encoded);
	return base58String;
}

std::string convertBinToBase64(const std::string& binString)
{
	auto mm = MemoryManager::getInstance();
	size_t binSize = binString.size();
	size_t encodedSize = sodium_base64_encoded_len(binSize, sodium_base64_VARIANT_ORIGINAL);
	
	auto base64 = mm->getMemory(encodedSize);
	memset(*base64, 0, encodedSize);

	size_t resultBinSize = 0;

	if (0 != sodium_bin2base64(*base64, encodedSize, (const unsigned char*)binString.data(), binSize, sodium_base64_VARIANT_ORIGINAL)) {
		mm->releaseMemory(base64);
		return "";
	}

	std::string base64String((const char*)*base64, encodedSize);
	mm->releaseMemory(base64);
	return base64String;
}

std::string convertBinToHex(const std::string& binString)
{
	auto mm = MemoryManager::getInstance();
	size_t hexSize = binString.size() * 2 +1;
	size_t binSize = binString.size();
	MemoryBin* hex = mm->getMemory(hexSize);
	memset(*hex, 0, hexSize);

	size_t resultBinSize = 0;

	sodium_bin2hex(*hex, hexSize, (const unsigned char*)binString.data(), binSize);
		
	std::string hexString((const char*)*hex, hexSize);
	mm->releaseMemory(hex);
	return hexString;
}
