#include "TimeoutDeferredTransferTransactionRole.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"
#include "gradido_blockchain/blockchain/Abstract.h"

using namespace gradido;

namespace model {
  namespace Apollo {
    namespace createTransaction {

      Transaction TimeoutDeferredTransferTransactionRole::createTransaction(
        const data::ConfirmedTransaction& confirmedTransaction, 
				memory::ConstBlockPtr pubkey
      ) {
        auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
			  auto transactionBody = gradidoTransaction->getTransactionBody();
        assert(transactionBody->isTimeoutDeferredTransfer());

        Transaction result(confirmedTransaction, pubkey);
        auto timeoutDeferredTransfer = transactionBody->getTimeoutDeferredTransfer();
        auto changeAccountBalance = calculateDecayedDeferredTransferAmount(
          timeoutDeferredTransfer->getDeferredTransferTransactionNr(), 
          confirmedTransaction.getConfirmedAt()
        );
        const auto& deferredTransferEntry = mBlockchain->getTransactionForId(timeoutDeferredTransfer->getDeferredTransferTransactionNr());
        const auto& deferredTransferBody = deferredTransferEntry->getTransactionBody();
        assert(deferredTransferBody->isDeferredTransfer());
        const auto& deferredTransfer = deferredTransferBody->getDeferredTransfer();

        result.setType(TransactionType::LINK_TIMEOUT);
        auto balance = changeAccountBalance.getBalance();
        if (pubkey->isTheSame(deferredTransfer->getRecipientPublicKey())) {
          balance.negate();
        }
				result.setAmount(balance);
				result.setPubkey(changeAccountBalance.getPublicKey());
        return result;
      }
    }
  }
}