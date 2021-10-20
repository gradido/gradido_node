#ifndef __GRADIDO_NODE_IOTA_MESSAGE_LISTENER_
#define __GRADIDO_NODE_IOTA_MESSAGE_LISTENER_

#include "Poco/Timer.h"
#include "../lib/MultithreadContainer.h"
#include <vector>
#include <map>
#include "IotaWrapper.h"

namespace iota
{
    /*!
     *  @author: einhornimmond
     * 
     *  @date: 20.02.2021
     * 
     *  @brief: call iota regular and check if new message on the listening index are available
     */
    // TODO: put into GroupManager
    
    class MessageListener : public UniLib::lib::MultithreadContainer
    {
    public:
        //! \param index should be something like messages/indexation/{index}
        MessageListener(const std::string& index, MessageType messageType, long intervalMilliseconds = 500);
        
        virtual void listener(Poco::Timer& timer);
    protected:

        std::string mIndex; 
        MessageType mMessageType;
        Poco::Timer mListenerTimer;
        Poco::Logger& mErrorLog;

        enum MessageState 
        {
            MESSAGE_EMPTY,
            MESSAGE_NEW, // fresh from iota
            MESSAGE_EXIST, // already in list
            MESSAGE_REMOVED // not longer returned from iota
        };

        struct MessageEntry 
        {
            std::string messageId;
            MessageState state;
        }
        std::map<MessageId, MessageEntry> mStoredMessageIds;            
    };
}

#endif // __GRADIDO_NODE_IOTA_MESSAGE_LISTENER_