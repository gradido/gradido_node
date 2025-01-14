#ifndef __GRADIDO_NODE_MODEL_APOLLO_REDEEM_DEFERRED_TRANSFER_TRANSACTION_ROLE_H
#define __GRADIDO_NODE_MODEL_APOLLO_REDEEM_DEFERRED_TRANSFER_TRANSACTION_ROLE_H

#include "AbstractTransactionRole.h"

namespace gradido {
  namespace data {
    class AccountBalance;
  }
}

namespace model {
  namespace Apollo {
    namespace createTransaction {
      class RedeemDeferredTransferTransactionRole : public AbstractTransactionRole 
      {
      public:
        using AbstractTransactionRole::AbstractTransactionRole;
        
        virtual Transaction createTransaction(
          const gradido::data::ConfirmedTransaction& confirmedTransaction, 
          memory::ConstBlockPtr pubkey
        );      
      protected:
        gradido::data::AccountBalance calculateDecayedDeferredTransferAmount(uint64_t transactionNr, Timepoint targetDate);
      };
    }
  }
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_REDEEM_DEFERRED_TRANSFER_TRANSACTION_ROLE_H