#include "ChangeRedeemDeferredTransferTransactionRole.h"
#include "gradido_blockchain/data/AccountBalance.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"
#include "gradido_blockchain/blockchain/Abstract.h"

using namespace gradido;

namespace model {
  namespace Apollo {
    namespace createTransaction {

      Transaction ChangeRedeemDeferredTransferTransactionRole::createTransaction(
        const data::ConfirmedTransaction& confirmedTransaction, 
				memory::ConstBlockPtr pubkey
      ) {
        auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
			  auto transactionBody = gradidoTransaction->getTransactionBody();
        assert(transactionBody->isRedeemDeferredTransfer());

        Transaction result(confirmedTransaction, pubkey);
        auto redeemDeferredTransfer = transactionBody->getRedeemDeferredTransfer();
        auto transfer = redeemDeferredTransfer->getTransfer();
        auto changeAccountBalance = calculateDecayedDeferredTransferAmount(
          redeemDeferredTransfer->getDeferredTransferTransactionNr(), 
          confirmedTransaction.getConfirmedAt()
        );

        auto change = changeAccountBalance.getBalance() - transfer.getSender().getAmount();

        result.setType(TransactionType::LINK_CHANGE);
				result.setAmount(change.negated());
				result.setPubkey(changeAccountBalance.getPublicKey());
        return result;
      }
    }
  }
}