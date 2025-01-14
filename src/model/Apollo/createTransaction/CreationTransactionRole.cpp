#include "CreationTransactionRole.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"

namespace model {
  namespace Apollo {
    namespace createTransaction {

      Transaction CreationTransactionRole::createTransaction(
        const gradido::data::ConfirmedTransaction& confirmedTransaction, 
				memory::ConstBlockPtr pubkey
      ) {
        auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
			  auto transactionBody = gradidoTransaction->getTransactionBody();
        assert(transactionBody->isCreation());

        Transaction result(confirmedTransaction, pubkey);
        result.setType(TransactionType::CREATE);
        
				auto creation = transactionBody->getCreation();
				result.setAmount(creation->getRecipient().getAmount());
				result.setFirstName("Gradido");
				result.setLastName("Akademie");
				result.setPubkey(gradidoTransaction->getSignatureMap().getSignaturePairs().front().getPublicKey());
        return result;
      }
    }
  }
}