// Copyright (c) 2026 The CoolCoin developers
// Experimental canonical data-carrier encoding for staking intent.
// NOT staking authorization, coin locking, or a consensus rule.
#ifndef COOLCOIN_CONSENSUS_HYBRID_EVENT_ENVELOPE_H
#define COOLCOIN_CONSENSUS_HYBRID_EVENT_ENVELOPE_H

#include <consensus/hybrid_admission.h>
#include <script/script.h>
#include <uint256.h>
#include <cstdint>
#include <vector>

namespace Consensus::Hybrid {

// Version 1 fixed-length payload (62 bytes), suitable for a single direct
// push following OP_RETURN. The validator field is a 20-byte identifier.
// A marker is NOT proof of ownership, registered stake or a valid withdrawal.
enum class StakeIntent : uint8_t { BOND = 1, EXIT = 2, WITHDRAW = 3 };
struct StakeEnvelope {
    StakeIntent intent{StakeIntent::BOND};
    StakeOutpoint outpoint;
    uint160 validator_id;
};
inline constexpr size_t STAKE_ENVELOPE_BYTES = 62;

inline CScript EncodeStakeEnvelope(const StakeEnvelope& event)
{
    std::vector<unsigned char> bytes{'C','S','T','K',1, static_cast<unsigned char>(event.intent)};
    bytes.insert(bytes.end(), event.outpoint.txid.begin(), event.outpoint.txid.end());
    for (unsigned int n = 0; n < 4; ++n)
        bytes.push_back(static_cast<unsigned char>(event.outpoint.vout >> (8 * n)));
    bytes.insert(bytes.end(), event.validator_id.begin(), event.validator_id.end());
    return CScript() << OP_RETURN << bytes;
}

inline bool DecodeStakeEnvelope(const CScript& script, StakeEnvelope& result)
{
    CScript::const_iterator cursor = script.begin();
    opcodetype op;
    std::vector<unsigned char> payload;
    if (!script.GetOp(cursor, op) || op != OP_RETURN ||
        !script.GetOp(cursor, op, payload) ||
        payload.size() != STAKE_ENVELOPE_BYTES ||
        cursor != script.end() ||
        payload[0] != 'C' || payload[1] != 'S' ||
        payload[2] != 'T' || payload[3] != 'K' ||
        payload[4] != 1 || payload[5] < 1 || payload[5] > 3) return false;

    StakeEnvelope parsed;
    parsed.intent = static_cast<StakeIntent>(payload[5]);
    for (unsigned int n = 0; n < 32; ++n) parsed.outpoint.txid.begin()[n] = payload[6 + n];
    uint32_t vout = 0;
    for (unsigned int n = 0; n < 4; ++n)
        vout |= static_cast<uint32_t>(payload[38 + n]) << (8 * n);
    parsed.outpoint.vout = vout;
    for (unsigned int n = 0; n < 20; ++n) parsed.validator_id.begin()[n] = payload[42 + n];

    // Reject non-minimal/non-canonical push encodings and trailing scripts.
    if (EncodeStakeEnvelope(parsed) != script) return false;
    result = parsed;
    return true;
}
} // namespace Consensus::Hybrid
#endif
