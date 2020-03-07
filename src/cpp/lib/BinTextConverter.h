#ifndef __GRADIDO_NODE_LIB_BIN_TEXT_CONVERTER_
#define __GRADIDO_NODE_LIB_BIN_TEXT_CONVERTER_

#include <string>

enum StringTransportType {
	STRING_TRANSPORT_TYPE_BASE64,
	STRING_TRANSPORT_TYPE_BASE58,
	STRING_TRANSPORT_TYPE_HEX,
	STRING_TRANSPORT_TYPE_UNKNOWN
};

std::string convertBinTransportStringToBin(const std::string& input);
std::string convertHexToBin(const std::string& hexString);
std::string convertBase64ToBin(const std::string& base64String);
std::string convertBase58ToBin(const std::string& base58String);

std::string convertBinToBase58(const std::string& binString);
std::string convertBinToBase64(const std::string& binString);
std::string convertBinToHex(const std::string& binString);

StringTransportType getStringTransportType(const std::string& input);


#endif //__GRADIDO_NODE_LIB_BIN_TEXT_CONVERTER_