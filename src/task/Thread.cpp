//#include "lib/Thread.h"
//#include "UniversumLib.h"
#include "Thread.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "../SingletonManager/LoggerManager.h"

namespace task {

    Thread::Thread(const char* threadName/* = NULL*/, bool createInConstructor/* = true*/)
        : mPocoThread(nullptr), exitCalled(false)
    {
		if (createInConstructor) init(threadName);
    } 

	int Thread::init(const char* threadName)
	{
		//semaphore.wait();
		mPocoThread = new Poco::Thread(threadName);
		mPocoThread->start(*this);
		return 0;
	}

    Thread::~Thread()
    {
		//printf("[Thread::~Thread]\n");
        if(mPocoThread)
        {
            //Post Exit to Thread
            exitCalled = true;
			//printf("[Thread::~Thread] before semaphore wait\n");
			//semaphore.wait();
			//printf("[Thread::~Thread] after semaphore wait, before condSignal\n");
            condSignal();
			//printf("[Thread::~Thread] after condSignal, before thread join\n");
            //SDL_Delay(500);
			mPocoThread->join();
			//printf("[Thread::~Thread] after thread join\n");
            //SDL_WaitThread(thread, NULL);
            //LOG_WARNING_SDL();
			delete mPocoThread;

			mPocoThread = nullptr;
        }
    }

    int Thread::condSignal()
    {
		condition.signal();
        return 0;
    }

    void Thread::run()
    {
        //Thread* t = this;
		Poco::Logger& errorLog = LoggerManager::getInstance()->mErrorLogging;
		auto threadName = mPocoThread->getName();
		while (true) {
			try {
				//semaphore.tryWait(100);
				//printf("[Thread::%s] end worker thread\n", __FUNCTION__);
				//break;
				if (exitCalled) return;
				// Lock work mutex
				Poco::ScopedLock _lock(mutex);
				//int status = SDL_CondWait(t->condition, t->mutex);
				try {
					condition.wait(mutex);
					if (exitCalled) {
						return;
					}
					int ret = ThreadFunction();
					if (ret) {
						errorLog.error("[Thread::%s] error running thread functon: %d, exit thread\n", threadName, ret);
						return;
					}
				}
				catch (Poco::NullPointerException& e) {
					errorLog.error("[Thread::%s] null pointer exception", threadName);
					return;
				}
				catch (Poco::Exception& e) {
					//unlock mutex and exit
					errorLog.error("[Thread::%s] Poco exception: %s\n", threadName, e.message());
					return;
				}
				catch (GradidoBlockchainException& e) {
					errorLog.error("[Thread::%s] Gradido Blockchain exception: %s\n", threadName, e.getFullString());
				}

			} catch (Poco::TimeoutException& e) {
				errorLog.error("[Thread::%s] timeout exception\n", threadName);
			} catch (Poco::Exception& e) {
				errorLog.error("[Thread::%s] exception: %s\n", threadName, e.message().data());
				return;
			}
		}
     
    }
}