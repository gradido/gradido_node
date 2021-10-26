#ifndef __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER
#define __GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER

/*!
 * @author: einhornimmond
 * 
 * @date: 20.10.2021
 * 
 * @brief: Get transactions unordered form iota but with information to put them into order into blockchain
 * Additional check for cross-group transactions
 */

#include "../iota/MilestoneListener.h"

class OrderingManager
{
public: 
    ~OrderingManager();
    static OrderingManager* getInstance();
    

protected:
    OrderingManager();
    iota::MilestoneListener mIotaMilestoneListener;
};

#endif //__GRADIDO_NODE_SINGLETON_MANAGER_ORDERING_MANAGER