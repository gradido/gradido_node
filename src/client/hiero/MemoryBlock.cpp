#include "MemoryBlock.h"

using std::make_shared;

namespace client {
  namespace hiero {

	  MemoryBlock::MemoryBlock(memory::ConstBlockPtr buffer)
		  : mBuffer(make_shared<memory::Block>(*buffer))
	  {


	  }

	  MemoryBlock::MemoryBlock(const memory::Block& bufferRef)
		  : mBuffer(make_shared<memory::Block>(bufferRef))
	  {

	  }

	  MemoryBlock::MemoryBlock(const grpc::ByteBuffer& byteBufferRef)
		  : mBuffer(make_shared<memory::Block>(fromGrpc(byteBufferRef)))
	  {

	  }

	  memory::Block MemoryBlock::fromGrpc(const grpc::ByteBuffer& byteBufferRef)
	  {
		  // TODO: split into multiple slice when it is to big, check default slicing size for grpc
		  grpc::Slice slice;
		  byteBufferRef.DumpToSingleSlice(&slice);
		  return memory::Block(slice.size(), slice.begin());
	  }

	  grpc::ByteBuffer MemoryBlock::createGrpcBuffer() const
	  {
		  if (!mBuffer || !mBuffer->size()) {
			  return grpc::ByteBuffer();
		  }
		  // TODO: split into multiple slice when it is to big, check default slicing size for grpc
		  grpc::Slice slice(mBuffer->data(), mBuffer->size());
		  return grpc::ByteBuffer(&slice, 1);
	  }
  }
}