// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL: off-chain reward allocation only.
// NOT integrated with blockchain consensus or coinbase validation.

#ifndef COOLCOIN_CONSENSUS_HYBRID_LEDGER_REWARDS_H
#define COOLCOIN_CONSENSUS_HYBRID_LEDGER_REWARDS_H

#include <consensus/hybrid_height_ledger.h>
#include <consensus/hybrid_reward_math.h>

#include <string>
#include <utility>
#include <vector>

namespace Consensus::Hybrid {

struct ValidatorReward {
    std::string validator;
    CAmount committed{0};
    CAmount effective{0};
    CAmount reward{0};
};

struct LedgerRewardAllocation {
    std::vector<ValidatorReward> validators;
    CAmount distributed{0};
    CAmount undistributed{0};
    bool valid{false};
};

inline LedgerRewardAllocation AllocateLedgerRewards(
    const HeightStakeLedger& ledger,
    CAmount pos_budget = POS_SUBSIDY)
{
    LedgerRewardAllocation result;
    std::vector<CAmount> committed;
    std::vector<std::string> validators;

    for (const auto& [id, bond] : ledger.Records()) {
        if (bond.phase != BondPhase::BONDED) continue;

        validators.push_back(id);
        committed.push_back(bond.committed);
    }

    const auto allocation = AllocateRewards(committed, pos_budget);

    if (!allocation.valid) return result;

    for (size_t i = 0; i < validators.size(); ++i) {
        const auto& entry = allocation.entries[i];

        result.validators.push_back({
            validators[i],
            entry.committed_stake,
            entry.effective_stake,
            entry.reward
        });
    }

    result.distributed = allocation.distributed;
    result.undistributed = allocation.undistributed;
    result.valid = true;

    return result;
}

} // namespace Consensus::Hybrid

#endif
