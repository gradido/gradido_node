#include "LoggerManager.h"


LoggerManager::LoggerManager()
	: mErrorLogging(Poco::Logger::get("errorLog")), mSpeedLogging(Poco::Logger::get("SpeedLog"))
{

}

LoggerManager::~LoggerManager()
{

}

LoggerManager* LoggerManager::getInstance()
{
	static LoggerManager one;
	return &one;
}

