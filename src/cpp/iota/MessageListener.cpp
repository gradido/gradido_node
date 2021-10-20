#include "MessageListener.h"
#include "ServerGlobals.h"

#include "IotaWrapper.h"

namespace iota 
{
    MessageListener::MessageListener(const std::string& index, MessageType messageType, long intervalMilliseconds/* = 2000*/)
    : mIndex(index), mMessageType(messageType), 
      mListenerTimer(0, intervalMilliseconds), mErrorLog(Poco::Logger::get("errorLog"))
    {
        Poco::TimerCallback<MessageListener> callback(*this, &MessageListener::listener);
	    mListenerTimer.start(callback);

    }

    void MessageListener::listener(Poco::Timer& timer)
    {
        // main loop, called regulary in separate thread
        static const char* function_name = "MessageListener::listener";
        
        auto skipped = timer.skipped();
        if(skipped) {
            mErrorLog.error("[%s] %d calls skipped, function needs to much time", function_name, skipped);
        }
        // collect message ids for index from iota
        auto messageIds = iota::getMessageIdsForIndexiation(mIndex);
        
        // compare with existing
        for(auto it = messageIds.begin(); it != messageIds.end(); it++) {
            // add if not exist and send to message validator        
            
        }
        // add if not exist and send to message validator    
        // update status if already exist
        // update status if not exist anymore and cleanup on next run
        
}