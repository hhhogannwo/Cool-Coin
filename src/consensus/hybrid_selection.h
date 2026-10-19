// Copyright (c) 2026 The CoolCoin developers
// Experimental deterministic weighted selection for TESTING ONLY.
// Not secure for on-chain validator election: caller-provided tickets can be manipulated.
#ifndef COOLCOIN_CONSENSUS_HYBRID_SELECTION_H
#define COOLCOIN_CONSENSUS_HYBRID_SELECTION_H
#include <consensus/hybrid_registry.h>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
namespace Consensus::Hybrid {
struct SelectionResult {
    bool valid{false};
    std::string validator_id;
    uint64_t total_weight{0};
};

// A ticket in [0,total_weight) selects a validator. The ticket source
// MUST NOT be controlled by validators for any future consensus usage.
inline SelectionResult SelectByTicket(const std::vector<ValidatorRecord>& validators,
                                     uint64_t ticket)
{
    uint64_t total = 0;
    for (const auto& v : validators) {
        if (v.validator_id.empty() || v.committed < MINIMUM_STAKE ||
            v.effective != EffectiveStake(v.committed)) return {};
        const uint64_t weight = static_cast<uint64_t>(v.effective);
        if (weight > std::numeric_limits<uint64_t>::max() - total) return {};
        total += weight;
    }
    if (total == 0 || ticket >= total) return {};
    uint64_t prefix = 0;
    for (const auto& v : validators) {
        prefix += static_cast<uint64_t>(v.effective);
        if (ticket < prefix) return {true, v.validator_id, total};
    }
    return {};
}
} // namespace Consensus::Hybrid
#endif
