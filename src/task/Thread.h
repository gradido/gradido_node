/**
 * @Author Dario Rekowski
 * 
 * @Date 13.08.12
 * 
 * @Desc Class for easy handling threading
 */
 
#ifndef __GRADIDO_NODE_TASK_THREAD_H
#define __GRADIDO_NODE_TASK_THREAD_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <string>

namespace task { 
    class Thread
    {
    public:
        Thread(const char* threadName);
        virtual ~Thread();

        // signal data chance, will continue thread, if he is paused
        void condSignal();

        void init();
        void exit();

        void run();

        inline void setName(const char* threadName) { mThreadName = threadName; }
        inline const std::string& getName() const { return mThreadName; }
        inline bool isExitCalled() const { std::unique_lock lock(mMutex); return mExitCalled; }
    protected:
        //! \brief will be called every time from thread, when condSignal was called
        //! will be called from thread with locked working mutex,<br>
        //! mutex will be unlock after calling this function
        //! \return if return isn't 0, thread will exit
        virtual int ThreadFunction() = 0;

    private:
        mutable std::mutex      mMutex;
        std::thread*            mThread;
        std::condition_variable mCondition;
        bool                    mExitCalled;
        std::string             mThreadName;
    };
}


#endif //__GRADIDO_NODE_TASK_THREAD_H
