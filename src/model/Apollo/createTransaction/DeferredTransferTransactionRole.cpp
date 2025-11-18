#include "DeferredTransferTransactionRole.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"

namespace model {
  namespace Apollo {
    namespace createTransaction {

      Transaction DeferredTransferTransactionRole::createTransaction(
        const gradido::data::ConfirmedTransaction& confirmedTransaction,
				memory::ConstBlockPtr pubkey
      ) {
        auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
        auto transactionBody = gradidoTransaction->getTransactionBody();
        assert(transactionBody->isDeferredTransfer());

        Transaction result(confirmedTransaction, pubkey);
        auto deferredTransfer = transactionBody->getDeferredTransfer();
        auto transfer = deferredTransfer->getTransfer();
        auto amount = transfer.getSender().getAmount();

        if (transfer.getRecipient()->isTheSame(pubkey)) {
          result.setType(TransactionType::LINK_CHARGE);
          result.setPubkey(transfer.getSender().getPublicKey());
        }
				else if (transfer.getSender().getPublicKey()->isTheSame(pubkey)) {
          result.setType(TransactionType::LINK_SEND);
          result.setPubkey(transfer.getRecipient());
					amount.negate();
				} else {
          throw GradidoNodeInvalidDataException("unhandled case in model::Apollo::createTransaction::DeferredTransferTransactionRole if pubkey is neither sender or recipient");
				}

				result.setAmount(amount);
        return result;
      }
    }
  }
}