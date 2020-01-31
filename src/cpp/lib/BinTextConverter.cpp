#include "BinTextConverter.h"

#include "Poco/RegularExpression.h"

#include "sodium.h"
#include "libbase58.h"

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


MemoryBin* convertBinTransportStringToBin(const std::string& input)
{
	auto type = getStringTransportType(input);
	switch (type) {
	case STRING_TRANSPORT_TYPE_BASE58: return convertBase58ToBin(input);
	case STRING_TRANSPORT_TYPE_BASE64: return convertBase64ToBin(input);
	case STRING_TRANSPORT_TYPE_HEX: return convertHexToBin(input);
	}
	return nullptr;
}

MemoryBin* convertHexToBin(const std::string& hexString)
{
	/*
	int sodium_hex2bin(unsigned char * const bin, const size_t bin_maxlen,
	const char * const hex, const size_t hex_len,
	const char * const ignore, size_t * const bin_len,
	const char ** const hex_end);

	The sodium_hex2bin() function parses a hexadecimal string hex and converts it to a byte sequence.

	hex does not have to be nul terminated, as the number of characters to parse is supplied via the hex_len parameter.

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
	MemoryBin* bin = mm->getFreeMemory(binSize);
	memset(*bin, 0, binSize);

	size_t resultBinSize = 0;

	if (0 != sodium_hex2bin(*bin, binSize, hexString.data(), hexSize, nullptr, &resultBinSize, nullptr)) {
		mm->releaseMemory(bin);
		return nullptr;
	}
	if (resultBinSize != binSize) {
		MemoryBin* repackedBin = mm->getFreeMemory(resultBinSize);
		memcpy(*repackedBin, *bin, resultBinSize);
		mm->releaseMemory(bin);
		return repackedBin;
	}
	return bin;

}

MemoryBin* convertBase64ToBin(const std::string& base64String)
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
	auto bin = mm->getFreeMemory(binSize);
	memset(*bin, 0, binSize);

	size_t resultBinSize = 0;

	if (0 != sodium_base642bin(*bin, binSize, base64String.data(), encodedSize, nullptr, &resultBinSize, nullptr, sodium_base64_VARIANT_ORIGINAL)) {
		mm->releaseMemory(bin);
		return nullptr;
	}

	if (resultBinSize != binSize) {
		MemoryBin* repackedBin = mm->getFreeMemory(resultBinSize);
		memcpy(*repackedBin, *bin, resultBinSize);
		mm->releaseMemory(bin);
		return repackedBin;
	}

	return bin;
}

MemoryBin* convertBase58ToBin(const std::string& base58String)
{
	/*
	Decoding Base58

	Simply allocate a buffer to store the binary data in, and set a variable with the buffer size, and call the b58tobin function:

	bool b58tobin(void *bin, size_t *binsz, const char *b58, size_t b58sz)

	The "canonical" base58 byte length will be assigned to binsz on success, which may be larger than the actual buffer if the input has many leading zeros. Regardless of the canonical byte length, the full binary buffer will be used. If b58sz is zero, it will be initialised with strlen(b58); note that a true zero-length base58 string is not supported here.
	*/

	auto mm = MemoryManager::getInstance();
	size_t encodedSize = base58String.size();
	size_t binSize = (encodedSize / 4) * 3;
	size_t calculatedBinSize = binSize;
	auto bin = mm->getFreeMemory(binSize);
	memset(*bin, 0, binSize);

	//size_t resultBinSize = 0;
	if (!b58tobin(*bin, &binSize, base58String.data(), encodedSize)) {
		mm->releaseMemory(bin);
		return nullptr;
	}

	if (calculatedBinSize != binSize) {
		MemoryBin* repackedBin = mm->getFreeMemory(binSize);
		memcpy(*repackedBin, *bin, binSize);
		mm->releaseMemory(bin);
		return repackedBin;
	}

	return bin;
}