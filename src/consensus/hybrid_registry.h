// Copyright (c) 2026 The CoolCoin developers
// Distributed under the MIT software license.
// EXPERIMENTAL: off-chain validator registry prototype, not consensus validation.
#ifndef COOLCOIN_CONSENSUS_HYBRID_REGISTRY_H
#define COOLCOIN_CONSENSUS_HYBRID_REGISTRY_H

#include <consensus/hybrid_params.h>
#include <map>
#include <string>
#include <vector>

namespace Consensus::Hybrid {

struct ValidatorRecord {
    std::string validator_id;
    CAmount committed{0};
    CAmount effective{0};
};

// Prototype in-memory registry only. No stake proof, key signature, lockup,
// identity attestation, chain persistence, or Sybil resistance is provided.
class ValidatorRegistry {
public:
    bool Register(const std::string& id, CAmount amount)
    {
        if (id.empty() || amount < MINIMUM_STAKE || records_.count(id)) return false;
        records_.emplace(id, ValidatorRecord{id, amount, EffectiveStake(amount)});
        return true;
    }

    bool UpdateCommitted(const std::string& id, CAmount amount)
    {
        auto it = records_.find(id);
        if (it == records_.end() || amount < MINIMUM_STAKE) return false;
        it->second.committed = amount;
        it->second.effective = EffectiveStake(amount);
        return true;
    }

    bool Unregister(const std::string& id) { return records_.erase(id) == 1; }

    std::vector<ValidatorRecord> Snapshot() const
    {
        std::vector<ValidatorRecord> result;
        for (const auto& pair : records_) result.push_back(pair.second);
        return result; // lexicographic ID ordering for repeatable simulations
    }

private:
    std::map<std::string, ValidatorRecord> records_;
};

} // namespace Consensus::Hybrid
#endif // COOLCOIN_CONSENSUS_HYBRID_REGISTRY_H
