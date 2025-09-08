#include "Key.h"
#include "loguru/loguru.hpp"
#include "gradido_blockchain/GradidoBlockchainException.h"

namespace hiero {

	Key::Key() : mEd25519(nullptr), mHasECDSA_secp256k1(false)
	{

	}
	Key::~Key()
	{

	}

	Key::Key(const KeyMessage& message)
		: Key()
	{
		if (message["ed25519"_f].has_value()) {
			auto publicKey = std::make_shared<const memory::Block>(message["ed25519"_f].value());
			mEd25519 = std::make_shared<KeyPairEd25519>(publicKey);
		}
		else if (message["ECDSA_secp256k1"_f].has_value()) {
			LOG_F(WARNING, "interpreting ECDSA_secp256k1 keys currently not implemented");
			mHasECDSA_secp256k1 = true;
		}
	}

	Key::Key(std::shared_ptr<const KeyPairEd25519> ed25519)
		: mEd25519(ed25519), mHasECDSA_secp256k1(false)
	{

	}

	KeyMessage Key::getMessage() const
	{
		KeyMessage message;
		if (mEd25519) {
			message["ed25519"_f] = mEd25519->getPublicKey()->copyAsVector();
		}
		else if (mHasECDSA_secp256k1) {
			throw GradidoNotImplementedException("hiero::Key::getMessage returning ECDSA_secp256k1 keys currently not implemented");
		}
		return message;
	}
}