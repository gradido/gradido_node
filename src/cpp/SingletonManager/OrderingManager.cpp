#include "OrderingManager.h"

OrderingManager::OrderingManager()
: mIotaMilestoneListener()
{
}

OrderingManager::~OrderingManager()
{

}

OrderingManager* OrderingManager::getInstance()
{
    static OrderingManager theOne;
    return &theOne;
}
