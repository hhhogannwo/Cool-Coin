// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL height-aware stake ledger; NOT blockchain consensus.
// Events and block hashes must originate from a consensus-validated chain.
// No funds are locked by this header, and no wallet or RPC input is trusted.
#ifndef COOLCOIN_CONSENSUS_HYBRID_HEIGHT_LEDGER_H
#define COOLCOIN_CONSENSUS_HYBRID_HEIGHT_LEDGER_H

#include <consensus/hybrid_lifecycle.h>
#include <consensus/hybrid_state_undo.h>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Consensus::Hybrid {

struct HeightBond {
    StakeOutpoint outpoint;
    BondPhase phase{BondPhase::PENDING};
    uint32_t eligible_height{0};
    uint32_t withdraw_height{0};
    CAmount committed{0};
};
struct HeightBlockUndo {
    uint256 previous_tip;
    uint256 block;
    uint32_t previous_height{0};
    bool previous_initialized{false};
    std::map<std::string, HeightBond> previous_bonds;
    std::set<StakeOutpoint> previous_claims;
};

class HeightStakeLedger {
public:
    // Height zero may be used as the first fixture block.
    bool Apply(const uint256& parent, const uint256& block, uint32_t height,
               const std::vector<StakeEvent>& events, HeightBlockUndo& undo)
    {
        if (block.IsNull() || block == parent || parent != tip_) return false;
        if (initialized_ && (height != last_height_ + 1 || last_height_ == UINT32_MAX)) return false;
        auto bonds = bonds_;
        auto claims = claims_;
        for (const auto& event : events) {
            if (event.validator.empty()) return false;
            auto it = bonds.find(event.validator);
            switch (event.type) {
            case StakeEventType::BOND:
                if (event.committed < MINIMUM_STAKE ||
                    event.committed > MAX_MONEY ||
                    it != bonds.end() || claims.count(event.outpoint) ||
                    height > UINT32_MAX - PROPOSED_MIN_CONFIRMATIONS) return false;
                bonds.emplace(event.validator, HeightBond{event.outpoint, BondPhase::PENDING,
                    height + PROPOSED_MIN_CONFIRMATIONS, 0});
                bonds.at(event.validator).committed = event.committed;
                claims.insert(event.outpoint);
                break;
            case StakeEventType::EXIT:
                if (it == bonds.end() || it->second.phase != BondPhase::BONDED ||
                    it->second.outpoint.txid != event.outpoint.txid ||
                    it->second.outpoint.vout != event.outpoint.vout ||
                    height > UINT32_MAX - PROPOSED_UNBONDING_BLOCKS) return false;
                it->second.phase = BondPhase::EXIT_PENDING;
                it->second.withdraw_height = height + PROPOSED_UNBONDING_BLOCKS;
                break;
            case StakeEventType::WITHDRAW: {
                // The exit must exist in parent state; cannot exit and
                // withdraw within the same block, even if event order changes.
                const auto prior = bonds_.find(event.validator);
                if (prior == bonds_.end() || prior->second.phase != BondPhase::EXIT_PENDING ||
                    height < prior->second.withdraw_height ||
                    it == bonds.end() ||
                    it->second.outpoint.txid != event.outpoint.txid ||
                    it->second.outpoint.vout != event.outpoint.vout) return false;
                claims.erase(it->second.outpoint);
                bonds.erase(it);
                break;
            }
            default:
                return false;
            }
        }
        // Eligibility changes by block height even when there are no events.
        for (auto& [id, bond] : bonds) {
            if (bond.phase == BondPhase::PENDING && height >= bond.eligible_height)
                bond.phase = BondPhase::BONDED;
        }
        HeightBlockUndo next;
        next.previous_tip = tip_;
        next.block = block;
        next.previous_height = last_height_;
        next.previous_initialized = initialized_;
        next.previous_bonds = bonds_;
        next.previous_claims = claims_;
        bonds_.swap(bonds);
        claims_.swap(claims);
        tip_ = block;
        last_height_ = height;
        initialized_ = true;
        undo = std::move(next);
        return true;
    }

    bool Disconnect(const HeightBlockUndo& undo)
    {
        // Prototype only: undo records must be supplied by trusted chainstate.
        // Check structural consistency before applying caller-supplied undo.
        if (tip_ != undo.block || undo.block.IsNull() ||
            undo.previous_tip == undo.block ||
            (undo.previous_initialized &&
             (last_height_ == 0 || undo.previous_height != last_height_ - 1)) ||
            (!undo.previous_initialized && !undo.previous_tip.IsNull())) return false;
        std::set<StakeOutpoint> derived;
        for (const auto& [id, record] : undo.previous_bonds) {
            if (id.empty() || !derived.insert(record.outpoint).second) return false;
        }
        if (derived != undo.previous_claims) return false;
        bonds_ = undo.previous_bonds;
        claims_ = undo.previous_claims;
        tip_ = undo.previous_tip;
        last_height_ = undo.previous_height;
        initialized_ = undo.previous_initialized;
        return true;
    }

    bool IsLocked(const StakeOutpoint& outpoint) const { return claims_.count(outpoint) != 0; }
    bool IsBonded(const std::string& id) const
    {
        const auto it = bonds_.find(id);
        return it != bonds_.end() && it->second.phase == BondPhase::BONDED;
    }
    bool CanWithdraw(const std::string& id, uint32_t height) const
    {
        const auto it = bonds_.find(id);
        return it != bonds_.end() && it->second.phase == BondPhase::EXIT_PENDING &&
               height >= it->second.withdraw_height;
    }
    const uint256& Tip() const { return tip_; }
    const std::map<std::string, HeightBond>& Records() const { return bonds_; }

private:
    uint256 tip_;
    uint32_t last_height_{0};
    bool initialized_{false};
    std::map<std::string, HeightBond> bonds_;
    std::set<StakeOutpoint> claims_;
};
} // namespace Consensus::Hybrid
#endif
