// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL ONLY: transaction-level candidate BOND preflight.
// Ownership proof arguments are externally provided, NOT extracted from
// authenticated on-chain bytes; this is NOT consensus validation.
#ifndef COOLCOIN_CONSENSUS_HYBRID_BOND_PREFLIGHT_H
#define COOLCOIN_CONSENSUS_HYBRID_BOND_PREFLIGHT_H

#include <consensus/hybrid_bond_auth.h>
#include <consensus/hybrid_tx_intent.h>
#include <consensus/hybrid_proof_carrier.h>
#include <primitives/transaction.h>

namespace Consensus::Hybrid {

// Reject ambiguous markers, unsupported operations and a transaction that
// spends the stake it purports to register. Real consensus must specify a
// signed on-chain proof envelope and enforce locks in ConnectBlock.
inline bool PreflightBondTransaction(
    const CTransaction& tx, const CCoinsView& parent_utxos,
    const CPubKey& owner_key, const std::vector<unsigned char>& signature,
    const uint256& chain_genesis, const uint256& nonce,
    uint32_t parent_height, uint32_t candidate_height)
{
    StakeEnvelope intent;
    if (ScanStakeIntent(tx, intent) != StakeIntentScanResult::ONE ||
        intent.intent != StakeIntent::BOND) return false;

    // A BOND marker must not be attached to a transaction spending its
    // referenced outpoint. Locking it would otherwise contradict the UTXO set.
    for (const CTxIn& input : tx.vin) {
        if (input.prevout.hash.ToUint256() == intent.outpoint.txid &&
            input.prevout.n == intent.outpoint.vout) return false;
    }

    return VerifyBondIntentOwner(parent_utxos, intent, owner_key, signature,
        chain_genesis, nonce, parent_height, candidate_height);
}

// Parse public key, nonce and signature from the transaction rather than
// trusting a separate caller-supplied proof. This still must be invoked by
// consensus against an authenticated parent UTXO view before it can enforce
// anything on the network.
inline bool PreflightEmbeddedBondTransaction(
    const CTransaction& tx, const CCoinsView& parent_utxos,
    const uint256& chain_genesis, uint32_t parent_height,
    uint32_t candidate_height)
{
    StakeProofCarrier proof;
    if (!ScanStakeProofCarrier(tx, proof)) return false;
    return PreflightBondTransaction(tx, parent_utxos, proof.owner,
        proof.signature, chain_genesis, proof.nonce, parent_height,
        candidate_height);
}

} // namespace Consensus::Hybrid
#endif
