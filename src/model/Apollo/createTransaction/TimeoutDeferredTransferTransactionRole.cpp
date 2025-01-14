#include "TimeoutDeferredTransferTransactionRole.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"

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

        result.setType(TransactionType::LINK_TIMEOUT);
				result.setAmount(changeAccountBalance.getBalance());
				result.setPubkey(changeAccountBalance.getPublicKey());
        return result;
      }
    }
  }
}