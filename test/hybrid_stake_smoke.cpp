// Copyright (c) 2026 The CoolCoin developers
// Distributed under the MIT software license.
//
// Standalone smoke test for DRAFT parameters only; does NOT test block validation.
// Run from repository root:
//   c++ -std=c++17 -Isrc test/hybrid_stake_smoke.cpp -o /tmp/coolcoin-hybrid-smoke
//   /tmp/coolcoin-hybrid-smoke

#include <consensus/hybrid_params.h>

#include <cassert>
#include <cstdint>

int main()
{
    using namespace Consensus::Hybrid;

    assert(ACTIVATION_HEIGHT == 50000);
    assert(EffectiveStake(0) == 0);
    assert(EffectiveStake(-1) == 0);
    assert(EffectiveStake(999 * COIN) == 0);
    assert(EffectiveStake(1000 * COIN) == 1000 * COIN);
    assert(EffectiveStake(10000 * COIN) == 10000 * COIN);
    assert(EffectiveStake(50000 * COIN) == 50000 * COIN);
    assert(EffectiveStake(250000 * COIN) == 250000 * COIN);
    assert(EffectiveStake(250001 * COIN) == 250000 * COIN);
    assert(EffectiveStake(500000 * COIN) == 250000 * COIN);
    assert(EffectiveStake(800000 * COIN) == 250000 * COIN);

    assert(POW_SUBSIDY == 48 * COIN);
    assert(POS_SUBSIDY == 16 * COIN);
    assert(TOTAL_SUBSIDY == 64 * COIN);

    // One staking identity with 800K and one with 250K have equal capped weight.
    // This does NOT stop a single human from creating multiple identities.
    assert(EffectiveStake(800000 * COIN) == EffectiveStake(250000 * COIN));
    return 0;
}
