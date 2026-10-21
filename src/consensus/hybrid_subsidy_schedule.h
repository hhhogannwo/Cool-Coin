// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL deterministic subsidy schedule, not active consensus rules.
// Activation requires a coordinated, reviewed network upgrade.
#ifndef COOLCOIN_CONSENSUS_HYBRID_SUBSIDY_SCHEDULE_H
#define COOLCOIN_CONSENSUS_HYBRID_SUBSIDY_SCHEDULE_H

#include <consensus/hybrid_params.h>
#include <cstdint>

namespace Consensus::Hybrid {

struct SubsidySplit {
    CAmount pow{0};
    CAmount pos{0};
    CAmount total{0};
};

// The historical subsidy must remain unchanged. Post-activation halvings
// follow the chain's configured halving interval, not a hardcoded 210000.
// At most 64 right shifts are permitted for 64-bit monetary arithmetic.
inline SubsidySplit ProposedSubsidySplit(int height, int halving_interval)
{
    if (height < 0 || halving_interval <= 0) return {};
    const int halvings = height / halving_interval;
    if (halvings >= 64) return {};
    const CAmount total = (64 * COIN) >> halvings;
    if (height < ACTIVATION_HEIGHT) return {total, 0, total};
    // Exact conservation for odd satoshi values: the remainder is assigned
    // to PoW. No rounding may inflate the combined subsidy.
    const CAmount pos = total / 2;
    return {total - pos, pos, total};
}

} // namespace Consensus::Hybrid
#endif
