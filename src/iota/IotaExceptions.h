#ifndef __GRADIDO_NODE_IOTA_IOTA_EXCEPTIONS_H
#define __GRADIDO_NODE_IOTA_IOTA_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"


class MessageIdFormatException : public GradidoBlockchainException
{
public: 
	explicit MessageIdFormatException(const char* what, const std::string& messageIdHex) noexcept;
	std::string getFullString() const;

protected:
	std::string mMessageIdHex;
};


#endif //__GRADIDO_NODE_IOTA_IOTA_EXCEPTIONS_H