#ifndef __GRADIDO_NODE_MODEL_APOLLO_TIMEOUT_DEFERRED_TRANSFER_TRANSACTION_ROLE_H
#define __GRADIDO_NODE_MODEL_APOLLO_TIMEOUT_DEFERRED_TRANSFER_TRANSACTION_ROLE_H

#include "RedeemDeferredTransferTransactionRole.h"

namespace model {
  namespace Apollo {
    namespace createTransaction {
      class TimeoutDeferredTransferTransactionRole : public RedeemDeferredTransferTransactionRole 
      {
      public:
        using RedeemDeferredTransferTransactionRole::RedeemDeferredTransferTransactionRole;
        
        virtual Transaction createTransaction(
          const gradido::data::ConfirmedTransaction& confirmedTransaction, 
          memory::ConstBlockPtr pubkey
        );
      };
    }
  }
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_TIMEOUT_DEFERRED_TRANSFER_TRANSACTION_ROLE_H