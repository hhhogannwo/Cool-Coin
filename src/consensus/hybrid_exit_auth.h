#ifndef COOLCOIN_CONSENSUS_HYBRID_EXIT_AUTH_H
#define COOLCOIN_CONSENSUS_HYBRID_EXIT_AUTH_H

// EXPERIMENTAL ONLY: EXIT authorization primitives.
// Not connected to live consensus validation.

#include <consensus/hybrid_auth.h>
#include <consensus/hybrid_utxo.h>
#include <primitives/transaction.h>

#include <string>
#include <vector>

namespace Consensus::Hybrid {

// Domain-separated authorization for one validator's EXIT request.
// The transaction hash binds the proof to a specific transaction.
// The caller must independently verify the parent stake record,
// stake ownership, proof encoding, and ordinary transaction scripts.
inline uint256 ExitAuthorizationDigest(
    const uint256& chain_genesis,
    const uint256& transaction_hash,
    const std::string& validator,
    const StakeOutpoint& stake,
    uint32_t exit_height,
    const uint256& nonce)
{
    const unsigned char domain[] = {
        'C','o','o','l','C','o','i','n','-',
        'P','o','S','-','E','X','I','T','-','v','1'
    };

    std::vector<unsigned char> payload(
        domain, domain + sizeof(domain));

    payload.insert(
        payload.end(), chain_genesis.begin(), chain_genesis.end());

    payload.insert(
        payload.end(), transaction_hash.begin(), transaction_hash.end());

    AppendLE32(payload, static_cast<uint32_t>(validator.size()));

    payload.insert(
        payload.end(), validator.begin(), validator.end());

    payload.insert(
        payload.end(), stake.txid.begin(), stake.txid.end());

    AppendLE32(payload, stake.vout);
    AppendLE32(payload, exit_height);

    payload.insert(
        payload.end(), nonce.begin(), nonce.end());

    return Hash(payload);
}

// Verifies cryptographic key control only.
// It does NOT establish that the key owns the bonded UTXO.
inline bool VerifyExitAuthorizationSignature(
    const CPubKey& owner,
    const uint256& digest,
    const std::vector<unsigned char>& signature)
{
    return owner.IsFullyValid() &&
           owner.Verify(digest, signature);
}

} // namespace Consensus::Hybrid

#endif
