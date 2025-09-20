#include "MemoryBlock.h"

namespace client {
  namespace grpc {

	  MemoryBlock::MemoryBlock(memory::ConstBlockPtr buffer)
		  : mBuffer(std::make_shared<memory::Block>(*buffer))
	  {


	  }

	  MemoryBlock::MemoryBlock(const memory::Block& bufferRef)
		  : mBuffer(std::make_shared<memory::Block>(bufferRef))
	  {

	  }

	  MemoryBlock::MemoryBlock(const ::grpc::ByteBuffer& byteBufferRef)
		  : mBuffer(std::make_shared<memory::Block>(fromGrpc(byteBufferRef)))
	  {

	  }

	  memory::Block MemoryBlock::fromGrpc(const ::grpc::ByteBuffer& byteBufferRef)
	  {
		  ::grpc::Slice slice;
		  byteBufferRef.DumpToSingleSlice(&slice);
		  return memory::Block(slice.size(), slice.begin());
	  }

	  ::grpc::ByteBuffer MemoryBlock::createGrpcBuffer() const
	  {
		  if (!mBuffer || !mBuffer->size()) {
			  return ::grpc::ByteBuffer();
		  }
		  ::grpc::Slice slice(mBuffer->data(), mBuffer->size());
		  return ::grpc::ByteBuffer(&slice, 1);
	  }
  }
}