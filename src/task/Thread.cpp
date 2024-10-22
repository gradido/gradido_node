#include "Thread.h"
#include "gradido_blockchain/GradidoBlockchainException.h"
#include "gradido_blockchain/ServerApplication.h"

#include "loguru/loguru.hpp"

namespace task {
	Thread::Thread(const char* threadName)
		: mThread(nullptr), mExitCalled(false)
	{
		if (threadName) {
			mThreadName = threadName;
		}
	}

	void Thread::init()
	{
		mThread = new std::thread(&Thread::run, this);
	}

	void Thread::exit()
	{
		{
			std::unique_lock _lock(mMutex);
			//Post Exit to Thread
			mExitCalled = true;
			condSignal();
		}
		if (mThread)
		{
			mThread->join();
			if (mThread) {
				delete mThread;
				mThread = nullptr;
			}
		}
	}

	Thread::~Thread()
	{
		exit();
	}

	void Thread::condSignal()
	{
		mCondition.notify_one();
	}

	void Thread::run()
	{
	 	loguru::set_thread_name(mThreadName.data());
		while (true) {
			try {
				if (mExitCalled) return;
				// Lock work mutex
				std::unique_lock _lock(mMutex);
				try {
					mCondition.wait(_lock);
					if (mExitCalled) {
						return;
					}
					int ret = ThreadFunction();
					if (ret) {
						LOG_F(ERROR, "error running thread function, %s thread returned with: %d, exit thread", mThreadName.data(), ret);
						return;
					}
				}
				catch (GradidoBlockchainException& e) {
					LOG_F(FATAL, "%s thread throw an uncatched GradidoBlockchainException exception: %s, exit thread", mThreadName.data(), e.getFullString().data());
					ServerApplication::terminate();
					return;
				}
				catch (std::exception& e) {
					LOG_F(FATAL, "%s thread throw an uncatched exception: %s, exit thread", mThreadName.data(), e.what());
					ServerApplication::terminate();
					return;
				}

			}
			catch (std::system_error& e) {
				LOG_F(FATAL, "%s Thread locking throw exception: %s, exit thread", mThreadName.data(), e.what());
				ServerApplication::terminate();
				return;
			}
		}

	}
}