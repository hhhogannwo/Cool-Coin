// Copyright (c) 2026 The CoolCoin developers
// Distributed under the MIT software license.
//
// EXPERIMENTAL ONLY. Proves key control, not stake ownership or a locked UTXO.
// No production consensus path calls this header.
#ifndef COOLCOIN_CONSENSUS_HYBRID_AUTH_H
#define COOLCOIN_CONSENSUS_HYBRID_AUTH_H

#include <consensus/amount.h>
#include <hash.h>
#include <pubkey.h>
#include <uint256.h>

#include <cstdint>
#include <vector>

namespace Consensus::Hybrid {

// Canonical little-endian integer encoding avoids platform-dependent hashing.
inline void AppendLE32(std::vector<unsigned char>& out, uint32_t value)
{
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<unsigned char>(value >> (8 * i)));
}
inline void AppendLE64(std::vector<unsigned char>& out, uint64_t value)
{
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<unsigned char>(value >> (8 * i)));
}

// Construct the message that a registering validator must sign.
// chain_genesis distinguishes mainnet and other CoolCoin chains;
// stake_txid, vout, amount, and nonce bind a signature to one application.
// Validation MUST separately check UTXO ownership, funds, lockup, maturity,
// uniqueness, nonce freshness and validator participation at the given height.
inline uint256 RegistrationDigest(const uint256& chain_genesis,
                                  const CPubKey& validator_key,
                                  const uint256& stake_txid,
                                  uint32_t stake_vout,
                                  CAmount committed_amount,
                                  uint32_t registration_height,
                                  const uint256& nonce)
{
    const unsigned char domain[] = {'C','o','o','l','C','o','i','n','-','P','o','S','-','R','e','g','-','v','1'};
    std::vector<unsigned char> payload(domain, domain + sizeof(domain));
    payload.insert(payload.end(), chain_genesis.begin(), chain_genesis.end());
    payload.insert(payload.end(), validator_key.begin(), validator_key.end());
    payload.insert(payload.end(), stake_txid.begin(), stake_txid.end());
    AppendLE32(payload, stake_vout);
    AppendLE64(payload, static_cast<uint64_t>(committed_amount));
    AppendLE32(payload, registration_height);
    payload.insert(payload.end(), nonce.begin(), nonce.end());
    return Hash(payload);
}

// Verification ONLY; no private keys are imported or exported.
inline bool VerifyRegistrationKeyProof(const CPubKey& validator_key,
                                       const uint256& digest,
                                       const std::vector<unsigned char>& signature)
{
    return validator_key.IsFullyValid() && validator_key.Verify(digest, signature);
}
} // namespace Consensus::Hybrid

#endif
