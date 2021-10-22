#include "OrderingManager.h"

OrderingManager::OrderingManager()
// maybe milestones/confirmed must be tried
// milestones/latest"
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
