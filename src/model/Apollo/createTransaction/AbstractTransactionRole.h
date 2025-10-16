#ifndef __GRADIDO_NODE_MODEL_APOLLO_ABSTRACT_TRANSACTION_ROLE_H
#define __GRADIDO_NODE_MODEL_APOLLO_ABSTRACT_TRANSACTION_ROLE_H

#include "../Transaction.h"
#include "gradido_blockchain/memory/Block.h"

namespace gradido {
  namespace data {
    class ConfirmedTransaction;
  }
  namespace blockchain {
    class Abstract;
  }
}

namespace model {
  namespace Apollo {
    namespace createTransaction {
      class AbstractTransactionRole 
      {
      public:
        AbstractTransactionRole(std::shared_ptr<const gradido::blockchain::Abstract> blockchain)
        : mBlockchain(blockchain) {}
        virtual ~AbstractTransactionRole() = default;

        virtual Transaction createTransaction(
          const gradido::data::ConfirmedTransaction& confirmedTransaction, 
          memory::ConstBlockPtr pubkey
        ) = 0;      

      protected:
        std::shared_ptr<const gradido::blockchain::Abstract> mBlockchain;
      };
    }
  }
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_ABSTRACT_TRANSACTION_ROLE_H