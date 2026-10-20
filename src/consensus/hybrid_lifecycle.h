// Copyright (c) 2026 The CoolCoin developers
// Experimental in-memory staking lifecycle prototype; NOT consensus enforcement.
#ifndef COOLCOIN_CONSENSUS_HYBRID_LIFECYCLE_H
#define COOLCOIN_CONSENSUS_HYBRID_LIFECYCLE_H

#include <consensus/hybrid_admission.h>
#include <cstdint>
#include <map>
#include <string>

namespace Consensus::Hybrid {

// Heights are consensus candidate parameters, NOT settled network rules.
inline constexpr uint32_t PROPOSED_UNBONDING_BLOCKS = 144;
enum class BondPhase { PENDING, BONDED, EXIT_PENDING };
struct BondRecord {
    StakeOutpoint outpoint;
    BondPhase phase{BondPhase::PENDING};
    uint32_t eligible_height{0};
    uint32_t withdraw_height{0};
};

class BondLifecycle {
public:
    // Only call from verified on-chain events in a future implementation.
    bool Bond(const std::string& id, const StakeOutpoint& outpoint,
              uint32_t inclusion_height, uint32_t activation_delay)
    {
        if (id.empty() || bonds_.count(id) || claimed_.count(outpoint)) return false;
        if (inclusion_height > UINT32_MAX - activation_delay) return false;
        bonds_.emplace(id, BondRecord{outpoint, BondPhase::PENDING,
                                     inclusion_height + activation_delay, 0});
        claimed_.emplace(outpoint, id);
        return true;
    }

    bool Activate(const std::string& id, uint32_t height)
    {
        auto it = bonds_.find(id);
        if (it == bonds_.end() || it->second.phase != BondPhase::PENDING ||
            height < it->second.eligible_height) return false;
        it->second.phase = BondPhase::BONDED;
        return true;
    }

    bool RequestExit(const std::string& id, uint32_t height, uint32_t unbond_delay)
    {
        auto it = bonds_.find(id);
        if (it == bonds_.end() || it->second.phase != BondPhase::BONDED ||
            unbond_delay < PROPOSED_UNBONDING_BLOCKS ||
            height > UINT32_MAX - unbond_delay) return false;
        it->second.phase = BondPhase::EXIT_PENDING;
        it->second.withdraw_height = height + unbond_delay;
        return true;
    }

    bool Withdraw(const std::string& id, uint32_t height)
    {
        auto it = bonds_.find(id);
        if (it == bonds_.end() || it->second.phase != BondPhase::EXIT_PENDING ||
            height < it->second.withdraw_height) return false;
        claimed_.erase(it->second.outpoint);
        bonds_.erase(it);
        return true;
    }

    bool IsLocked(const StakeOutpoint& outpoint) const { return claimed_.count(outpoint) != 0; }
    bool IsEligible(const std::string& id) const
    {
        auto it = bonds_.find(id);
        return it != bonds_.end() && it->second.phase == BondPhase::BONDED;
    }

private:
    std::map<std::string, BondRecord> bonds_;
    std::map<StakeOutpoint, std::string> claimed_;
};
} // namespace Consensus::Hybrid
#endif
