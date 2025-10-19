#ifndef __GRADIDO_NODE_HIERO_KEY_H
#define __GRADIDO_NODE_HIERO_KEY_H

#include "gradido_blockchain/interaction/deserialize/Protopuf.h"
#include "gradido_blockchain/crypto/KeyPairEd25519.h"

namespace hiero {

    using KeyMessage = message<
        bytes_field<"ed25519", 2>,
        bytes_field<"ECDSA_secp256k1", 7> // will log as warning
    >;

    class Key 
    {
    public:
        Key();    
        Key(const KeyMessage& message);
        Key(std::shared_ptr<const KeyPairEd25519> ed25519);
        ~Key();

        KeyMessage getMessage() const;

        inline std::shared_ptr<const KeyPairEd25519> getEd25519() const { return mEd25519; }
        inline bool hasECDSA_secp256k1() const { return mHasECDSA_secp256k1; }
    private:
        std::shared_ptr<const KeyPairEd25519> mEd25519;
        bool mHasECDSA_secp256k1;
    };
}

#endif //__GRADIDO_NODE_HIERO_KEY_H