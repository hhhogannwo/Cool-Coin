// Copyright (c) 2026 The CoolCoin developers
// Distributed under the MIT software license.
//
// EXPERIMENTAL: deterministic reward math for off-chain simulation ONLY.
// Not integrated with validation, coinbase, or validator selection.
// Every input entry represents one *already validated* staking identity.
// Does not establish Sybil resistance or ensure active stake ownership.
#ifndef COOLCOIN_CONSENSUS_HYBRID_REWARD_MATH_H
#define COOLCOIN_CONSENSUS_HYBRID_REWARD_MATH_H

#include <consensus/hybrid_params.h>

#include <cstdint>
#include <limits>
#include <vector>

namespace Consensus::Hybrid {

struct RewardEntry {
    CAmount committed_stake;
    CAmount effective_stake{0};
    CAmount reward{0};
};

struct RewardAllocation {
    std::vector<RewardEntry> entries;
    CAmount distributed{0};
    CAmount undistributed{0};
    bool valid{false};
};

// Integer flooring prevents accidental overpayment. The remainder stays
// UNMINTED in this simulation; production consensus must explicitly define
// its treatment. No PoS subsidy is paid when no eligible stakers exist.
// Uses 128-bit intermediate arithmetic (GCC/Clang) for exact division.
inline RewardAllocation AllocateRewards(const std::vector<CAmount>& committed, CAmount budget = POS_SUBSIDY)
{
    RewardAllocation result;
    if (budget < 0 || budget > TOTAL_SUBSIDY) return result;

    unsigned __int128 total_weight{0};
    for (const CAmount amount : committed) {
        if (amount < 0 || amount > MAX_MONEY) return RewardAllocation{};
        const CAmount weight = EffectiveStake(amount);
        result.entries.push_back({amount, weight, 0});
        total_weight += static_cast<uint64_t>(weight);
    }

    result.undistributed = budget;
    if (total_weight == 0) {
        result.valid = true;
        return result;
    }

    for (auto& entry : result.entries) {
        const unsigned __int128 numerator =
            static_cast<unsigned __int128>(static_cast<uint64_t>(budget)) *
            static_cast<uint64_t>(entry.effective_stake);
        entry.reward = static_cast<CAmount>(numerator / total_weight);
        result.distributed += entry.reward;
    }
    result.undistributed = budget - result.distributed;
    result.valid = true;
    return result;
}

} // namespace Consensus::Hybrid

#endif // COOLCOIN_CONSENSUS_HYBRID_REWARD_MATH_H
