#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_CACHE_MANAGER_H
#define __GRADIDO_NODE_SINGLETON_MANAGER_CACHE_MANAGER_H

//#include "../controller/Block.h"
#include "../lib/FuzzyTimer.h"

class CacheManager
{
public:
	~CacheManager();

	static CacheManager* getInstance();

	inline FuzzyTimer* getFuzzyTimer() { return &mFuzzyTimer; }

protected:
	CacheManager();

	FuzzyTimer mFuzzyTimer;
	
};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_CACHE_MANAGER_H