
#include "BlockchainOrderExceptions.h"

BlockchainOrderException::BlockchainOrderException(const char* what) noexcept
	: GradidoBlockchainException(what)
{

}

std::string BlockchainOrderException::getFullString() const 
{
	return what();
}