#ifndef __GRADIDO_NODE_MODEL_APOLLO_CREATE_TRANSACTION_CONTEXT_H
#define __GRADIDO_NODE_MODEL_APOLLO_CREATE_TRANSACTION_CONTEXT_H

#include "AbstractTransactionRole.h"
#include "gradido_blockchain_core/types/address.h"

namespace model {
  namespace Apollo {
    namespace createTransaction {
      class Context
      {
      public:
        Context(std::shared_ptr<const gradido::blockchain::Abstract> blockchain, grdt_address addressType)
        : mBlockchain(blockchain), mAddressType(addressType) {}

      std::vector<Transaction> run(
        const gradido::data::ConfirmedTransaction& confirmedTransaction,
				memory::ConstBlockPtr pubkey
      );

      protected:
        std::shared_ptr<const gradido::blockchain::Abstract> mBlockchain;
        grdt_address mAddressType;
      };
    }
  }
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_CREATE_TRANSACTION_CONTEXT_H