// Copyright (c) 2026 The CoolCoin developers
// Distributed under the MIT software license.
// Standalone off-chain prototype smoke tests, not consensus integration.
// c++ -std=c++17 -Isrc test/hybrid_reward_math_smoke.cpp -o /tmp/hybrid-rewards && /tmp/hybrid-rewards

#include <consensus/hybrid_reward_math.h>
#include <cassert>
#include <vector>

int main()
{
    using namespace Consensus::Hybrid;
    const auto equal = AllocateRewards({10000 * COIN, 10000 * COIN, 10000 * COIN, 10000 * COIN, 10000 * COIN});
    assert(equal.valid && equal.entries.size() == 5);
    assert(equal.distributed == POS_SUBSIDY && equal.undistributed == 0);
    for (const auto& e : equal.entries) assert(e.reward == 320000000);

    const auto mixed = AllocateRewards({100000 * COIN, 100000 * COIN, 20000 * COIN, 20000 * COIN, 20000 * COIN});
    assert(mixed.valid);
    assert(mixed.entries[0].reward == mixed.entries[1].reward);
    assert(mixed.entries[2].reward == mixed.entries[3].reward);
    assert(mixed.entries[0].reward == 5 * mixed.entries[2].reward || mixed.entries[0].reward == 5 * mixed.entries[2].reward + 1);
    assert(mixed.distributed + mixed.undistributed == POS_SUBSIDY);

    const auto capped = AllocateRewards({800000 * COIN, 250000 * COIN, 999 * COIN});
    assert(capped.valid && capped.entries[0].effective_stake == capped.entries[1].effective_stake);
    assert(capped.entries[0].reward == 8 * COIN && capped.entries[1].reward == 8 * COIN);
    assert(capped.entries[2].reward == 0);

    const auto none = AllocateRewards({0, 500 * COIN});
    assert(none.valid && none.distributed == 0 && none.undistributed == POS_SUBSIDY);

    const auto invalid = AllocateRewards({-1, 2000 * COIN});
    assert(!invalid.valid);

    const auto tiny = AllocateRewards({1000 * COIN, 2000 * COIN, 3000 * COIN}, 1);
    assert(tiny.valid && tiny.distributed + tiny.undistributed == 1);
    return 0;
}
