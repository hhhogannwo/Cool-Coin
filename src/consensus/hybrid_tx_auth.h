// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL signature preimage for an on-chain BOND authorization.
// This is not a replacement for script signatures or an active consensus rule.
#ifndef COOLCOIN_CONSENSUS_HYBRID_TX_AUTH_H
#define COOLCOIN_CONSENSUS_HYBRID_TX_AUTH_H

#include <consensus/hybrid_auth.h>
#include <consensus/hybrid_event_envelope.h>
#include <consensus/hybrid_proof_carrier.h>
#include <primitives/transaction.h>
#include <hash.h>

namespace Consensus::Hybrid {

// Bind an external registration proof to the actual transaction's non-proof
// input/output structure. Exclude the proof carrier to avoid self-reference.
// All ordinary inputs (including sequence), transaction version and locktime,
// plus all non-proof outputs, are committed. A malicious miner must not be able
// to transplant a proof to another registration transaction.
inline uint256 BondTransactionDigest(
    const CTransaction& tx, const StakeEnvelope& intent,
    const CPubKey& owner, const uint256& genesis, const uint256& nonce,
    CAmount amount, uint32_t height)
{
    std::vector<unsigned char> msg{'C','o','o','l','-','B','o','n','d','-','T','x','-','v','2'};
    const uint256 base = RegistrationDigest(genesis, owner, intent.outpoint.txid,
        intent.outpoint.vout, amount, height, nonce);
    msg.insert(msg.end(), base.begin(), base.end());
    AppendLE32(msg, static_cast<uint32_t>(tx.nVersion));
    AppendLE32(msg, tx.nLockTime);
    AppendLE32(msg, static_cast<uint32_t>(tx.vin.size()));
    for (const auto& input : tx.vin) {
        const uint256 id = input.prevout.hash.ToUint256();
        msg.insert(msg.end(), id.begin(), id.end());
        AppendLE32(msg, input.prevout.n);
        AppendLE32(msg, input.nSequence);
        // Scripts and witness are deliberately not committed here. A full
        // design must bind/validate the transaction's script execution.
    }
    // No output proof is accepted here: the caller supplies outputs explicitly
    // after removing one canonical CSPF carrier to break circular signing.
    AppendLE32(msg, static_cast<uint32_t>(tx.vout.size()));
    for (const auto& output : tx.vout) {
        AppendLE64(msg, static_cast<uint64_t>(output.nValue));
        AppendLE32(msg, static_cast<uint32_t>(output.scriptPubKey.size()));
        msg.insert(msg.end(), output.scriptPubKey.begin(), output.scriptPubKey.end());
    }
    return Hash(msg);
}

// Construct the transaction-bound authorization digest while excluding
// exactly one canonical CSPF proof carrier to avoid circular signing.
// The caller must independently reject duplicate or malformed carriers.
inline bool EmbeddedBondTransactionDigest(
    const CTransaction& tx,
    const StakeEnvelope& intent,
    const CPubKey& owner,
    const uint256& genesis,
    const uint256& nonce,
    CAmount amount,
    uint32_t height,
    uint256& digest)
{
    CMutableTransaction unsigned_tx(tx);
    size_t carrier_count = 0;

    for (size_t i = 0; i < unsigned_tx.vout.size();) {
        StakeProofCarrier candidate;
        if (DecodeStakeProofCarrier(
                unsigned_tx.vout[i].scriptPubKey, candidate)) {
            if (unsigned_tx.vout[i].nValue != 0)
                return false;
            ++carrier_count;
            unsigned_tx.vout.erase(unsigned_tx.vout.begin() + i);
        } else {
            ++i;
        }
    }

    if (carrier_count != 1)
        return false;

    digest = BondTransactionDigest(
        CTransaction(unsigned_tx), intent, owner,
        genesis, nonce, amount, height);

    return true;
}

} // namespace Consensus::Hybrid
#endif
