#include "gradido_blockchain/data/ConfirmedTransaction.h"
#include "gradido_blockchain_core/types/address.h"
#include "gradido_blockchain_core/types/transaction.h"

#include "Context.h"

#include "CreationTransactionRole.h"
#include "DeferredTransferTransactionRole.h"
#include "RedeemDeferredTransferTransactionRole.h"
#include "TimeoutDeferredTransferTransactionRole.h"
#include "TransferTransactionRole.h"

#include "magic_enum/magic_enum.hpp"

using namespace magic_enum;

using namespace gradido;

namespace model {
  namespace Apollo {
    namespace createTransaction {
      std::vector<Transaction> Context::run(
        const gradido::data::ConfirmedTransaction& confirmedTransaction, 
				memory::ConstBlockPtr pubkey
      ) {
        auto body = confirmedTransaction.getGradidoTransaction()->getTransactionBody();
        std::vector<std::unique_ptr<AbstractTransactionRole>> roles;
        std::unique_ptr<RedeemDeferredTransferTransactionRole> redeemRole;
        roles.reserve(2);
        switch(body->getTransactionType()) {
          case GRDT_TRANSACTION_CREATION:
            roles.push_back(std::make_unique<CreationTransactionRole>(mBlockchain));
            break;
          case GRDT_TRANSACTION_TRANSFER:
            roles.push_back(std::make_unique<TransferTransactionRole>(mBlockchain));
            break;
          case GRDT_TRANSACTION_DEFERRED_TRANSFER:
            roles.push_back(std::make_unique<DeferredTransferTransactionRole>(mBlockchain));
            break;
          case GRDT_TRANSACTION_REDEEM_DEFERRED_TRANSFER:
            redeemRole = std::make_unique<RedeemDeferredTransferTransactionRole>(mBlockchain);
            redeemRole->setAddressType(mAddressType);
            roles.push_back(std::move(redeemRole));
            break;
          case GRDT_TRANSACTION_TIMEOUT_DEFERRED_TRANSFER:
            roles.push_back(std::make_unique<TimeoutDeferredTransferTransactionRole>(mBlockchain));
            break;
          case GRDT_TRANSACTION_COMMUNITY_ROOT:
          case GRDT_TRANSACTION_REGISTER_ADDRESS:
            return {};
          default:
            auto type = body->getTransactionType();
            throw GradidoUnhandledEnum(
              "unhandles transaction type in model::Apollo::createTransaction::Context",
              enum_type_name<decltype(type)>().data(),
              enum_name(type).data()
            );

        }
        std::vector<Transaction> transactions;
        for(auto& role: roles) {
          transactions.push_back(role->createTransaction(confirmedTransaction, pubkey));
        }
        return transactions;
      }
    }
  }
}