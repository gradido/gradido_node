#ifndef __GRADIDO_NODE_BLOCKCHAIN_ORDER_EXCEPTIONS_H
#define __GRADIDO_NODE_BLOCKCHAIN_ORDER_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"

class BlockchainOrderException : public GradidoBlockchainException
{
public:
	explicit BlockchainOrderException(const char* what) noexcept;
	std::string getFullString() const;
};


#endif //__GRADIDO_NODE_BLOCKCHAIN_ORDER_EXCEPTIONS_H