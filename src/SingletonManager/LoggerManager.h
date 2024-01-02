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
	Poco::Logger&         mInfoLogging;
	Poco::Logger&		  mSpeedLogging;
	Poco::Logger&		  mTransactionLog;
protected:
	

	LoggerManager();
};

#define LOG_INFO(fmt, ...) LoggerManager::getInstance()->mInfoLogging.information(fmt, __VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LoggerManager::getInstance()->mInfoLogging.debug(fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...) LoggerManager::getInstance()->mInfoLogging.warning(fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) LoggerManager::getInstance()->mErrorLogging.error(fmt, __VA_ARGS__)

#endif //__GRADIDO_NODE_SINGLETONE_MANAGER_LOGGER_MANAGER_H