#ifndef __GRADIDO_NODE_TASK_CPU_TASK_GRPC_REACTOR_H
#define __GRADIDO_NODE_TASK_CPU_TASK_GRPC_REACTOR_H

#include "CPUTask.h"
#include "../client/hiero/MessageObserver.h"

#include <atomic>

namespace task {
    template<class Object, class ObjectMessage>
    class CPUTaskGRPCReactor : public CPUTask, public client::hiero::MessageObserver<ObjectMessage>
    {
    public:
        using CPUTask::CPUTask;

        bool isReady() override { return mMessageArrived; }

        // from MessageObserver
        virtual void onMessageArrived(const ObjectMessage& message) override {
            mObject = Object(message);
            mMessageArrived = true;
        }             

        // from MessageObserver
        // will be called from grpc client if connection was closed
        // so no more messageArrived calls
        virtual void onConnectionClosed() override {
            mIsClosed = true;
        }

        inline bool isMessageArrived() const { return mMessageArrived; }

    protected:
        Object mObject;

        std::atomic<bool> mIsClosed;
        std::atomic<bool> mMessageArrived;
    };
}
#endif //__GRADIDO_NODE_TASK_CPU_TASK_GRPC_REACTOR_H