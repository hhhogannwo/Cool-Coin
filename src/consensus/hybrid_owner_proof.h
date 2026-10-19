// Copyright (c) 2026 The CoolCoin developers
// Prototype-only ownership-binding check for *native v0 P2WPKH* outputs.
// Does not validate that the UTXO is actually bonded or that the transaction
// is included in a block. Never treat this as full PoS consensus enforcement.
#ifndef COOLCOIN_CONSENSUS_HYBRID_OWNER_PROOF_H
#define COOLCOIN_CONSENSUS_HYBRID_OWNER_PROOF_H

#include <consensus/hybrid_auth.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <script/script.h>

#include <vector>

namespace Consensus::Hybrid {

// Verifies that the key signing the domain-separated registration message
// is also the P2WPKH key explicitly committed to by the output script.
// This works ONLY for a native v0 P2WPKH output. Other script types fail
// closed until a separate reviewed authorization rule is specified.
inline bool VerifyP2WPKHRegistration(const CTxOut& deposit,
                                      const CPubKey& owner_and_validator_key,
                                      const uint256& digest,
                                      const std::vector<unsigned char>& signature)
{
    if (!owner_and_validator_key.IsFullyValid() || !owner_and_validator_key.IsCompressed()) return false;
    const uint160 keyhash = Hash160(owner_and_validator_key);
    const CScript expected = CScript() << OP_0 <<
        std::vector<unsigned char>(keyhash.begin(), keyhash.end());
    return deposit.scriptPubKey == expected &&
           VerifyRegistrationKeyProof(owner_and_validator_key, digest, signature);
}

} // namespace Consensus::Hybrid
#endif
