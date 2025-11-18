#ifndef __GRADIDO_NODE_MODEL_APOLLO_REDEEM_DEFERRED_TRANSFER_TRANSACTION_ROLE_H
#define __GRADIDO_NODE_MODEL_APOLLO_REDEEM_DEFERRED_TRANSFER_TRANSACTION_ROLE_H

#include "AbstractTransactionRole.h"
#include "gradido_blockchain/data/AddressType.h"
#include "gradido_blockchain/data/TransferAmount.h"

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
        inline void setAddressType(gradido::data::AddressType addressType) { mAddressType = addressType; }
        virtual Transaction createTransaction(
          const gradido::data::ConfirmedTransaction& confirmedTransaction, 
          memory::ConstBlockPtr pubkey
        );
      protected:
        gradido::data::AccountBalance calculateChange(
          uint64_t deferredTransferTransactionNr,
          Timepoint targetDate,
          const gradido::data::TransferAmount& transferAmount
        ) const;
        gradido::data::AccountBalance calculateDecayedDeferredTransferAmount(uint64_t transactionNr, Timepoint targetDate) const;
        gradido::data::AddressType mAddressType;
      };
    }
  }
}

#endif //__GRADIDO_NODE_MODEL_APOLLO_REDEEM_DEFERRED_TRANSFER_TRANSACTION_ROLE_H