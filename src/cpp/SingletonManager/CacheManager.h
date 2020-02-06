#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_CACHE_MANAGER_H
#define __GRADIDO_NODE_SINGLETON_MANAGER_CACHE_MANAGER_H

#include "../controller/Block.h"

class CacheManager
{
public:
	~CacheManager();

	static CacheManager* getInstance();
protected:
	CacheManager();
	
};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_CACHE_MANAGER_H