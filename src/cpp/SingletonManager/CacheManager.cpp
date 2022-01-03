#include "CacheManager.h"

CacheManager::CacheManager()
{

}

CacheManager::~CacheManager()
{
	int zahl = 0;
}

CacheManager* CacheManager::getInstance()
{
	static CacheManager theOne;
	return &theOne;
}