#ifndef __GRADIDO_NODE_MODEL_APOLLO_CREATE_TRANSACTION_CONTEXT_H
#define __GRADIDO_NODE_MODEL_APOLLO_CREATE_TRANSACTION_CONTEXT_H

#include "AbstractTransactionRole.h"
#include "gradido_blockchain/data/AddressType.h"

namespace model {
  namespace Apollo {
    namespace createTransaction {
      class Context 
      {
      public:
        Context(std::shared_ptr<const gradido::blockchain::Abstract> blockchain, gradido::data::AddressType addressType)
        : mBlockchain(blockchain), mAddressType(addressType) {}

      std::vector<Transaction> run(
        const gradido::data::ConfirmedTransaction& confirmedTransaction, 
				memory::ConstBlockPtr pubkey
      );

      protected:
        std::shared_ptr<const gradido::blockchain::Abstract> mBlockchain;
        gradido::data::AddressType mAddressType;
      };
    }
  }
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_CREATE_TRANSACTION_CONTEXT_H