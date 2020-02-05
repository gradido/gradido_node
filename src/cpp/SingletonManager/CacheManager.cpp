#include "CacheManager.h"

CacheManager::CacheManager()
{

}

CacheManager::~CacheManager()
{

}

CacheManager* CacheManager::getInstance()
{
	static CacheManager theOne;
	return &theOne;
}