#ifndef __GRADIDO_NODE_LIB_BIN_TEXT_CONVERTER_
#define __GRADIDO_NODE_LIB_BIN_TEXT_CONVERTER_

#include "../SingletonManager/MemoryManager.h"

enum StringTransportType {
	STRING_TRANSPORT_TYPE_BASE64,
	STRING_TRANSPORT_TYPE_BASE58,
	STRING_TRANSPORT_TYPE_HEX,
	STRING_TRANSPORT_TYPE_UNKNOWN
};

MemoryBin* convertBinTransportStringToBin(const std::string& input);
MemoryBin* convertHexToBin(const std::string& hexString);
MemoryBin* convertBase64ToBin(const std::string& base64String);
MemoryBin* convertBase58ToBin(const std::string& base58String);

StringTransportType getStringTransportType(const std::string& input);


#endif //__GRADIDO_NODE_LIB_BIN_TEXT_CONVERTER_