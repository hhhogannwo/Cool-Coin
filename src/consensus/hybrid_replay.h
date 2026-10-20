// Copyright (c) 2026 The CoolCoin developers
// Experimental deterministic replay from caller-validated block events.
// No disk storage, chain authentication, or production consensus integration.
#ifndef COOLCOIN_CONSENSUS_HYBRID_REPLAY_H
#define COOLCOIN_CONSENSUS_HYBRID_REPLAY_H

#include <consensus/hybrid_state_undo.h>
#include <vector>

namespace Consensus::Hybrid {

struct StakeReplayBlock {
    uint256 parent;
    uint256 hash;
    std::vector<StakeEvent> events;
};

// Atomic reconstruction: invalid histories do not mutate the caller's state.
// Input blocks/events MUST come from validated chain history, not wallet/RPC data.
inline bool ReplayStakeBlocks(const std::vector<StakeReplayBlock>& history,
                              StakeStateUndoPrototype& state)
{
    StakeStateUndoPrototype rebuilt;
    for (const auto& block : history) {
        StakeBlockUndo unused_undo;
        if (!rebuilt.ApplyBlock(block.parent, block.hash, block.events, unused_undo)) {
            return false;
        }
    }
    state = std::move(rebuilt);
    return true;
}
} // namespace Consensus::Hybrid
#endif
