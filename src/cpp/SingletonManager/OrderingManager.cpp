#include "OrderingManager.h"

OrderingManager::OrderingManager()
// maybe milestones/confirmed must be tried
: mIotaMilestoneListener("milestones/latest", iota::MESSAGE_TYPE_MILESTONE)
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
