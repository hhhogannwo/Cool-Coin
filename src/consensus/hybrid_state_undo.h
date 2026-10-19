// Copyright (c) 2026 The CoolCoin developers
// Experimental deterministic staking state/undo prototype.
// NOT consensus, durable storage, or a replacement for chainstate undo.
#ifndef COOLCOIN_CONSENSUS_HYBRID_STATE_UNDO_H
#define COOLCOIN_CONSENSUS_HYBRID_STATE_UNDO_H

#include <consensus/hybrid_admission.h>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Consensus::Hybrid {

enum class StakeEventType : uint8_t { BOND, EXIT, WITHDRAW };

struct StakeEvent {
    StakeEventType type;
    std::string validator;
    StakeOutpoint outpoint;
};

struct StakeStateRecord {
    StakeOutpoint outpoint;
    bool exiting{false};
};

struct StakeBlockUndo {
    uint256 parent_block;
    uint256 applied_block;
    std::map<std::string, StakeStateRecord> previous_records;
    std::set<StakeOutpoint> previous_outpoints;
};

// Events MUST be validated before calling ApplyBlock in a future consensus
// integration. A failed batch must leave the entire state unchanged.
class StakeStateUndoPrototype {
public:
    bool ApplyBlock(const uint256& parent, const uint256& block,
                    const std::vector<StakeEvent>& events, StakeBlockUndo& undo)
    {
        if (parent != tip_ || block.IsNull() || block == parent) return false;
        auto updated = records_;
        auto outpoints = claimed_;
        // A bond cannot exit and withdraw within one block. A withdrawal
        // must reference an exit already present in the parent state.
        for (const auto& event : events) {
            if (event.validator.empty()) return false;
            auto it = updated.find(event.validator);
            switch (event.type) {
            case StakeEventType::BOND:
                if (it != updated.end() || outpoints.count(event.outpoint)) return false;
                updated.emplace(event.validator, StakeStateRecord{event.outpoint, false});
                outpoints.insert(event.outpoint);
                break;
            case StakeEventType::EXIT:
                if (it == updated.end() || it->second.exiting ||
                    !(it->second.outpoint.txid == event.outpoint.txid &&
                      it->second.outpoint.vout == event.outpoint.vout)) return false;
                it->second.exiting = true;
                break;
            case StakeEventType::WITHDRAW:
                const auto parent_record = records_.find(event.validator);
                if (parent_record == records_.end() || !parent_record->second.exiting ||
                    it == updated.end() || !it->second.exiting ||
                    !(it->second.outpoint.txid == event.outpoint.txid &&
                      it->second.outpoint.vout == event.outpoint.vout)) return false;
                outpoints.erase(it->second.outpoint);
                updated.erase(it);
                break;
            default:
                return false;
            }
        }
        StakeBlockUndo next_undo;
        next_undo.parent_block = tip_;
        next_undo.applied_block = block;
        next_undo.previous_records = records_;
        next_undo.previous_outpoints = claimed_;
        records_.swap(updated);
        claimed_.swap(outpoints);
        tip_ = block;
        undo = std::move(next_undo);
        return true;
    }

    bool DisconnectBlock(const StakeBlockUndo& undo)
    {
        // Caller must supply the genuine undo record for the connected block.
        // In-memory metadata alone cannot authenticate a malicious undo.
        if (tip_ != undo.applied_block) return false;
        records_ = undo.previous_records;
        claimed_ = undo.previous_outpoints;
        tip_ = undo.parent_block;
        return true;
    }

    bool IsBonded(const std::string& id) const
    {
        const auto it = records_.find(id);
        return it != records_.end() && !it->second.exiting;
    }
    bool IsClaimed(const StakeOutpoint& outpoint) const
    {
        return claimed_.count(outpoint) != 0;
    }
    const std::map<std::string, StakeStateRecord>& Records() const { return records_; }
    const uint256& Tip() const { return tip_; }

private:
    uint256 tip_;
    std::map<std::string, StakeStateRecord> records_;
    std::set<StakeOutpoint> claimed_;
};
} // namespace Consensus::Hybrid
#endif
