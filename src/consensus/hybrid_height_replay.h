// Copyright (c) 2026 The CoolCoin developers
// Experimental replay of validated height-indexed staking events.
// This is not persistent consensus storage or proof of block authenticity.
#ifndef COOLCOIN_CONSENSUS_HYBRID_HEIGHT_REPLAY_H
#define COOLCOIN_CONSENSUS_HYBRID_HEIGHT_REPLAY_H

#include <consensus/hybrid_height_ledger.h>
#include <utility>
#include <vector>

namespace Consensus::Hybrid {

struct HeightStakeReplayBlock {
    uint256 parent;
    uint256 hash;
    uint32_t height;
    std::vector<StakeEvent> events;
};

// Reconstruct into a temporary state; reject corrupt height, parent, event,
// or maturity sequences without modifying the caller's previously valid state.
// Callers must supply events extracted from independently validated blocks.
inline bool ReplayHeightStakeBlocks(const std::vector<HeightStakeReplayBlock>& history,
                                    HeightStakeLedger& destination)
{
    HeightStakeLedger rebuilt;
    for (const auto& block : history) {
        HeightBlockUndo discard;
        if (!rebuilt.Apply(block.parent, block.hash, block.height, block.events, discard)) {
            return false;
        }
    }
    destination = std::move(rebuilt);
    return true;
}

} // namespace Consensus::Hybrid
#endif
