#ifndef __GRADIDO_NODE_LIB_LEVEL_DB_EXCEPTIONS_H
#define __GRADIDO_NODE_LIB_LEVEL_DB_EXCEPTIONS_H

#include "gradido_blockchain/GradidoBlockchainException.h"
#include "leveldb/status.h"

class LevelDBKeyAlreadyExistException : public GradidoBlockchainException
{
public: 
	explicit LevelDBKeyAlreadyExistException(const char* what, const std::string& key, const std::string& value) noexcept;
	std::string getFullString() const;

protected:
	std::string mKey;
	std::string mValue;
};

class LevelDBStatusException : public GradidoBlockchainException
{
public:
	explicit LevelDBStatusException(const char* what, const leveldb::Status& status) noexcept;
	std::string getFullString() const;

protected:
	std::string mStatusString;
};



#endif //__GRADIDO_NODE_LIB_LEVEL_DB_EXCEPTIONS_H