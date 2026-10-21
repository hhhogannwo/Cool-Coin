// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL ONLY. A withdrawal preflight is NOT consensus enforcement.
// Call only after ordinary input/script validation against the authenticated
// parent UTXO set. ConnectBlock must separately enforce the stake locks.
#ifndef COOLCOIN_CONSENSUS_HYBRID_WITHDRAW_PREFLIGHT_H
#define COOLCOIN_CONSENSUS_HYBRID_WITHDRAW_PREFLIGHT_H

#include <consensus/hybrid_height_ledger.h>
#include <consensus/hybrid_tx_intent.h>
#include <primitives/transaction.h>
#include <hash.h>

namespace Consensus::Hybrid {

// Fail-closed structural withdrawal check. The caller must ensure that the
// authenticated parent ledger corresponds to the actual parent block and
// that all input scripts are valid. This function does NOT check signatures.
inline bool PreflightWithdrawalStructure(const CTransaction& tx,
                                         const HeightStakeLedger& parent_ledger,
                                         uint32_t candidate_height)
{
    if (tx.IsCoinBase()) return false;
    StakeEnvelope envelope;
    if (ScanStakeIntent(tx, envelope) != StakeIntentScanResult::ONE ||
        envelope.intent != StakeIntent::WITHDRAW) return false;

    const std::string validator_id = envelope.validator_id.GetHex();
    const auto& records = parent_ledger.Records();
    const auto record = records.find(validator_id);
    if (record == records.end() ||
        record->second.phase != BondPhase::EXIT_PENDING ||
        candidate_height < record->second.withdraw_height ||
        record->second.outpoint.txid != envelope.outpoint.txid ||
        record->second.outpoint.vout != envelope.outpoint.vout ||
        !parent_ledger.IsLocked(envelope.outpoint)) return false;

    // Exactly one input must spend the registered outpoint. No other locked
    // stake may be spent under cover of this withdrawal marker.
    unsigned int matching_inputs = 0;
    for (const auto& input : tx.vin) {
        const StakeOutpoint spent{input.prevout.hash.ToUint256(), input.prevout.n};
        if (spent == envelope.outpoint) {
            ++matching_inputs;
        } else if (parent_ledger.IsLocked(spent)) {
            return false;
        }
    }
    return matching_inputs == 1;
}

} // namespace Consensus::Hybrid
#endif
