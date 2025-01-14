#include "TransferTransactionRole.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"

namespace model {
  namespace Apollo {
    namespace createTransaction {

      Transaction TransferTransactionRole::createTransaction(
        const gradido::data::ConfirmedTransaction& confirmedTransaction, 
				memory::ConstBlockPtr pubkey
      ) {
        auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
			  auto transactionBody = gradidoTransaction->getTransactionBody();
        assert(transactionBody->isTransfer());

        Transaction result(confirmedTransaction, pubkey);
        auto transfer = transactionBody->getTransfer();
        auto amount = transfer->getSender().getAmount();
				
        if (transfer->getRecipient()->isTheSame(pubkey)) {
          result.setType(TransactionType::RECEIVE);
          result.setPubkey(transfer->getSender().getPublicKey());
        }
				else if (transfer->getSender().getPublicKey()->isTheSame(pubkey)) {
          result.setType(TransactionType::SEND);
          result.setPubkey(transfer->getRecipient());
					amount.negate();
				} else { 
          throw GradidoNodeInvalidDataException("unhandled case in model::Apollo::createTransaction::TransferTransactionRole if pubkey is neither sender or recipient");
				}

				result.setAmount(amount);
				result.setPubkey(gradidoTransaction->getSignatureMap().getSignaturePairs().front().getPublicKey());
        return result;
      }
    }
  }
}