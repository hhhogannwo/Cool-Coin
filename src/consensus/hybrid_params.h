// Copyright (c) 2026 The CoolCoin developers
// Distributed under the MIT software license.
//
// Draft hybrid consensus parameters. NOT wired into block validation.
// Never treat this header alone as activation of PoS consensus.
#ifndef COOLCOIN_CONSENSUS_HYBRID_PARAMS_H
#define COOLCOIN_CONSENSUS_HYBRID_PARAMS_H

#include <consensus/amount.h>

namespace Consensus::Hybrid {

inline constexpr int ACTIVATION_HEIGHT = 50000;
inline constexpr CAmount MINIMUM_STAKE = 1000 * COIN;
inline constexpr CAmount MAXIMUM_EFFECTIVE_STAKE = 250000 * COIN;

// Amounts before the first halving only. A separate rule must enforce
// reward allocation in the coinbase and validator payments at activation.
inline constexpr CAmount POW_SUBSIDY = 32 * COIN;
inline constexpr CAmount POS_SUBSIDY = 32 * COIN;
inline constexpr CAmount TOTAL_SUBSIDY = POW_SUBSIDY + POS_SUBSIDY;

// This must be applied to an aggregate per eligible staking identity.
// The question of preventing one person registering multiple identities
// remains open; this function does not solve that Sybil problem.
constexpr CAmount EffectiveStake(const CAmount committed_stake)
{
    if (committed_stake < MINIMUM_STAKE) return 0;
    return committed_stake > MAXIMUM_EFFECTIVE_STAKE ? MAXIMUM_EFFECTIVE_STAKE : committed_stake;
}

static_assert(EffectiveStake(999 * COIN) == 0);
static_assert(EffectiveStake(1000 * COIN) == 1000 * COIN);
static_assert(EffectiveStake(250000 * COIN) == 250000 * COIN);
static_assert(EffectiveStake(800000 * COIN) == 250000 * COIN);
static_assert(TOTAL_SUBSIDY == 64 * COIN);

} // namespace Consensus::Hybrid

#endif // COOLCOIN_CONSENSUS_HYBRID_PARAMS_H
