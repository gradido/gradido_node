#ifndef __GRADIDO_NODE_SINGLETONE_MANAGER_LOGGER_MANAGER_H
#define __GRADIDO_NODE_SINGLETONE_MANAGER_LOGGER_MANAGER_H


#include "Poco/Logger.h"

class LoggerManager
{
public:
	~LoggerManager();

	static LoggerManager* getInstance();

	Poco::Logger&		  mErrorLogging;
	Poco::Logger&		  mSpeedLogging;
protected:
	

	LoggerManager();
};

#endif //__GRADIDO_NODE_SINGLETONE_MANAGER_LOGGER_MANAGER_H