#include "RedeemDeferredTransferTransactionRole.h"
#include "gradido_blockchain/data/AccountBalance.h"
#include "gradido_blockchain/data/ConfirmedTransaction.h"
#include "gradido_blockchain/blockchain/Abstract.h"

using namespace gradido;

namespace model {
  namespace Apollo {
    namespace createTransaction {

      Transaction RedeemDeferredTransferTransactionRole::createTransaction(
        const gradido::data::ConfirmedTransaction& confirmedTransaction, 
				memory::ConstBlockPtr pubkey
      ) {
        auto gradidoTransaction = confirmedTransaction.getGradidoTransaction();
			  auto transactionBody = gradidoTransaction->getTransactionBody();
        assert(transactionBody->isRedeemDeferredTransfer());

        Transaction result(confirmedTransaction, pubkey);
        auto redeemDeferredTransfer = transactionBody->getRedeemDeferredTransfer();
        auto transfer = redeemDeferredTransfer->getTransfer();
        auto amount = transfer.getSender().getAmount();
				
        if (transfer.getRecipient()->isTheSame(pubkey)) {
          result.setType(TransactionType::LINK_RECEIVE);
          result.setPubkey(transfer.getSender().getPublicKey());
        }
				else if (transfer.getSender().getPublicKey()->isTheSame(pubkey)) {
          result.setType(TransactionType::SEND);
          result.setPubkey(transfer.getRecipient());
					amount.negate();
				} else { 
          auto changeAccountBalance = calculateDecayedDeferredTransferAmount(
            redeemDeferredTransfer->getDeferredTransferTransactionNr(), 
            confirmedTransaction.getConfirmedAt()
          );
          result.setType(TransactionType::LINK_CHANGE);
          result.setPubkey(changeAccountBalance.getPublicKey());  
          amount = changeAccountBalance.getBalance() - transfer.getSender().getAmount();          
				}
				result.setAmount(amount);		
        return result;		
      }

       data::AccountBalance RedeemDeferredTransferTransactionRole::calculateDecayedDeferredTransferAmount(
          uint64_t transactionNr,
          Timepoint targetDate
      ) {
			if(!transactionNr) {
				throw GradidoNullPointerException("transactionNr is zero", "transactionNr", __FUNCTION__);
			}
			auto deferredTransferEntry = mBlockchain->getTransactionForId(transactionNr);
			auto deferredTransferBody = deferredTransferEntry->getTransactionBody();
			assert(deferredTransferBody->isDeferredTransfer());
			auto deferredTransfer = deferredTransferBody->getDeferredTransfer();
            auto& transferAmount = deferredTransferBody->getTransferAmount();
			GradidoUnit decayed = transferAmount.getAmount().calculateDecay(
				deferredTransferEntry->getConfirmedTransaction()->getConfirmedAt(),
				targetDate
			);

			return data::AccountBalance(transferAmount.getPublicKey(), decayed, transferAmount.getCommunityId());
      }
    }
  }
}