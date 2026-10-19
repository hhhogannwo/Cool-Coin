// Copyright (c) 2026 The CoolCoin developers
// Experimental transaction-level staking-intent inspection.
// A data carrier alone is NEVER an authorized stake operation.
#ifndef COOLCOIN_CONSENSUS_HYBRID_TX_INTENT_H
#define COOLCOIN_CONSENSUS_HYBRID_TX_INTENT_H

#include <consensus/hybrid_event_envelope.h>
#include <primitives/transaction.h>

namespace Consensus::Hybrid {

enum class StakeIntentScanResult {
    NONE,
    ONE,
    INVALID,
};

// Reject ambiguity: at most one recognized version-1 stake marker per
// transaction; malformed CSTK markers must not be silently ignored.
// The caller MUST separately authenticate ownership, lock the UTXO,
// validate lifecycle and check transaction script semantics.
inline StakeIntentScanResult ScanStakeIntent(const CTransaction& tx,
                                            StakeEnvelope& out)
{
    bool found = false;
    StakeEnvelope candidate;
    for (const CTxOut& output : tx.vout) {
        const CScript& script = output.scriptPubKey;
        CScript::const_iterator it = script.begin();
        opcodetype opcode;
        std::vector<unsigned char> bytes;
        if (!script.GetOp(it, opcode) || opcode != OP_RETURN) continue;
        if (!script.GetOp(it, opcode, bytes)) continue;
        // Strictly recognize the dedicated CSTK domain when it is carried
        // as a direct push. Nonmatching OP_RETURN remains ordinary data.
        if (bytes.size() < 4 || bytes[0] != 'C' || bytes[1] != 'S' ||
            bytes[2] != 'T' || bytes[3] != 'K') continue;
        if (output.nValue != 0 || found || !DecodeStakeEnvelope(script, candidate))
            return StakeIntentScanResult::INVALID;
        found = true;
    }
    if (!found) return StakeIntentScanResult::NONE;
    out = candidate;
    return StakeIntentScanResult::ONE;
}
} // namespace Consensus::Hybrid
#endif
