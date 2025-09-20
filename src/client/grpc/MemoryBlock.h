#ifndef __GRADIDO_NODE_CLIENT_GRPC_MEMORY_BLOCK_H_
#define __GRADIDO_NODE_CLIENT_GRPC_MEMORY_BLOCK_H_

#include "gradido_blockchain/memory/Block.h"
#include <grpcpp/support/byte_buffer.h>

namespace client {
    namespace grpc {
        class MemoryBlock
        {
        public:
            //! copy pointer
            MemoryBlock(memory::BlockPtr buffer) : mBuffer(buffer) {};
            //! copy content
            MemoryBlock(memory::ConstBlockPtr buffer);
            //! copy content
            MemoryBlock(const memory::Block& bufferRef);
            //! copy content
            MemoryBlock(const ::grpc::ByteBuffer& byteBufferRef);

            inline operator memory::BlockPtr() { return mBuffer; }
            inline operator memory::ConstBlockPtr() const { return mBuffer; }
            static memory::Block fromGrpc(const ::grpc::ByteBuffer& byteBufferRef);

            //! copy data into grpc byte buffer
            //! return empty ByteBuffer if mBuffer is empty
            ::grpc::ByteBuffer createGrpcBuffer() const;
        protected:
            
            memory::BlockPtr mBuffer;
        };
    }
}

#endif //__GRADIDO_NODE_CLIENT_GRPC_MEMORY_BLOCK_H_