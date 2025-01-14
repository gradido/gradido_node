#ifndef __GRADIDO_NODE_MODEL_APOLLO_CREATION_TRANSACTION_ROLE_H
#define __GRADIDO_NODE_MODEL_APOLLO_CREATION_TRANSACTION_ROLE_H

#include "AbstractTransactionRole.h"

namespace model {
  namespace Apollo {
    namespace createTransaction {
      class CreationTransactionRole : public AbstractTransactionRole 
      {
      public:
        using AbstractTransactionRole::AbstractTransactionRole;
        
        virtual Transaction createTransaction(
          const gradido::data::ConfirmedTransaction& confirmedTransaction, 
          memory::ConstBlockPtr pubkey
        );      
      protected:
      };
    }
  }
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_CREATION_TRANSACTION_ROLE_H