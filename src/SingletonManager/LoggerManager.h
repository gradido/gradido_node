#ifndef __GRADIDO_NODE_SINGLETONE_MANAGER_LOGGER_MANAGER_H
#define __GRADIDO_NODE_SINGLETONE_MANAGER_LOGGER_MANAGER_H


#include "Poco/Logger.h"

/*!
 * @author Dario Rekowski
 * @date  2020-02-27
 * @brief hold logger instance in memory for program-wide accesses
 */

class LoggerManager
{
public:
	~LoggerManager();

	static LoggerManager* getInstance();

	Poco::Logger&		  mErrorLogging;
	Poco::Logger&		  mSpeedLogging;
	Poco::Logger&		  mTransactionLog;
protected:
	

	LoggerManager();
};

#endif //__GRADIDO_NODE_SINGLETONE_MANAGER_LOGGER_MANAGER_H