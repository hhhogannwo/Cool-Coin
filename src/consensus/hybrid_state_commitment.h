// Copyright (c) 2026 The CoolCoin developers
// Experimental: deterministic post-transaction stake state commitment.
// NOT a consensus commitment until validated events and state root are
// specified and enforced as part of blocks.
#ifndef COOLCOIN_CONSENSUS_HYBRID_STATE_COMMITMENT_H
#define COOLCOIN_CONSENSUS_HYBRID_STATE_COMMITMENT_H

#include <consensus/hybrid_state_undo.h>
#include <hash.h>

#include <vector>

namespace Consensus::Hybrid {

// Canonical serialization: fixed domain, count, sorted validator IDs,
// length-prefixed IDs, outpoints and exit states. For testing only.
inline uint256 StakeStateCommitment(const std::map<std::string, StakeStateRecord>& records)
{
    std::vector<unsigned char> bytes{
        'C','o','o','l','C','o','i','n','-','S','t','a','k','e','-','V','1'};
    auto write32 = [&bytes](uint32_t value) {
        for (unsigned int i = 0; i < 4; ++i)
            bytes.push_back(static_cast<unsigned char>(value >> (8 * i)));
    };
    if (records.size() > UINT32_MAX) return uint256{};
    write32(static_cast<uint32_t>(records.size()));
    for (const auto& [id, record] : records) {
        if (id.size() > UINT32_MAX) return uint256{};
        write32(static_cast<uint32_t>(id.size()));
        bytes.insert(bytes.end(), id.begin(), id.end());
        bytes.insert(bytes.end(), record.outpoint.txid.begin(), record.outpoint.txid.end());
        write32(record.outpoint.vout);
        bytes.push_back(record.exiting ? 1 : 0);
    }
    return Hash(bytes);
}
} // namespace Consensus::Hybrid
#endif
