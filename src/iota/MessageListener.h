#ifndef __GRADIDO_NODE_IOTA_MESSAGE_LISTENER_
#define __GRADIDO_NODE_IOTA_MESSAGE_LISTENER_

//#include "Poco/Timer.h"
#include "Poco/Logger.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"
#include "../lib/FuzzyTimer.h"

#include "MessageId.h"

#include <vector>
#include <map>

namespace iota
{
    /*!
       @author einhornimmond
      
       @date 20.02.2021
      
       @brief call iota regular and check if new message on the listening index are available

       Iota store the last 1000 Messages so we must filter ourselves which are new
       All new MessageIDs will be sended to MessageValidator
      
      \startuml
      (*) --> "Call from Timer"
      --> "Iota Api call /api/v1/messages?index=4752414449444f2e7079746861676f726173"
      note right
       index consist of "GRADIDO." + groupAlias
       in hex format, for example:
       4752414449444f2e7079746861676f726173 => GRADIDO.pythagoras
      end note
      --> "Add new MessageIDs to intern List"
      note right
        Iota store the last 1000 Messages for a index,
        so on every call we get all stored MessageIDs not only the new ones,
        so we must filter ourselves
      end note
      --> "Send new MessageIDs to MessageValidator"
      --> "Remove old MessageIDs from intern List"
      note right
        remove all MessageIDs from intern List,
        which are not longer returned from Iota
      end note
      --> (*)
      \enduml
     */    
    
    class MessageListener : public MultithreadContainer, public TimerCallback
    {
    public:
        //! \param index should be something like GRADIDO.gdd1
        MessageListener(const std::string& index, long intervalMilliseconds = 1000);
        ~MessageListener();
        
        //virtual void listener(Poco::Timer& timer);

		TimerReturn callFromTimer();
		const char* getResourceType() const { return "iota::MessageListener"; };

    protected:

        void updateStoredMessages(std::vector<MemoryBin*>& currentMessageIds);

        std::string mIndex; 
        //Poco::Timer mListenerTimer;
        Poco::Logger& mErrorLog;

        enum MessageState 
        {
            MESSAGE_EMPTY,
            MESSAGE_NEW, // fresh from iota
            MESSAGE_EXIST, // already in list
            MESSAGE_REMOVED // not longer returned from iota
        };
        bool mFirstRun;
        std::map<MessageId, MessageState> mStoredMessageIds;            
    };
}

#endif // __GRADIDO_NODE_IOTA_MESSAGE_LISTENER_