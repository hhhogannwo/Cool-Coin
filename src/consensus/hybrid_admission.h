// Copyright (c) 2026 The CoolCoin developers
// Distributed under the MIT software license.
//
// EXPERIMENTAL: off-chain bookkeeping prototype only; NOT blockchain validation.
// The caller is responsible for consensus verification of existence, value,
// unspent state, staking lock and ownership of the referenced UTXO.
// Never use caller-supplied booleans as a mainnet ownership proof.
#ifndef COOLCOIN_CONSENSUS_HYBRID_ADMISSION_H
#define COOLCOIN_CONSENSUS_HYBRID_ADMISSION_H

#include <consensus/hybrid_params.h>
#include <uint256.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace Consensus::Hybrid {

struct StakeOutpoint {
    uint256 txid;
    uint32_t vout{0};
    bool operator<(const StakeOutpoint& other) const
    {
        return txid < other.txid || (txid == other.txid && vout < other.vout);
    }
};

// This evidence MUST eventually be created inside chainstate validation, not
// accepted from an RPC user, wallet or network message. These fields only
// make preconditions testable before chainstate integration.
struct ValidatedStakeEvidence {
    StakeOutpoint outpoint;
    CAmount amount{0};
    uint32_t confirmations{0};
    bool unspent{false};
    bool owner_authorized{false};
    bool lock_enforced{false};
};

// Prototype choice, not yet adopted as a consensus rule.
inline constexpr uint32_t PROPOSED_MIN_CONFIRMATIONS = 100;

class StakeAdmissionBook {
public:
    bool Register(const std::string& validator_id, const ValidatedStakeEvidence& proof,
                  const uint256& registration_nonce)
    {
        if (validator_id.empty() || records_.count(validator_id)) return false;
        if (proof.amount < MINIMUM_STAKE || proof.amount > MAX_MONEY) return false;
        if (proof.confirmations < PROPOSED_MIN_CONFIRMATIONS) return false;
        if (!proof.unspent || !proof.owner_authorized || !proof.lock_enforced) return false;
        if (committed_outpoints_.count(proof.outpoint) || used_nonces_.count(registration_nonce)) return false;

        records_.emplace(validator_id, proof.outpoint);
        committed_outpoints_.insert(proof.outpoint);
        used_nonces_.insert(registration_nonce);
        return true;
    }

    // Unregister releases the bookkeeping claim but NEVER makes a nonce reusable.
    // Actual withdrawal must be subject to on-chain unbonding and slashing rules.
    bool Unregister(const std::string& validator_id)
    {
        const auto it = records_.find(validator_id);
        if (it == records_.end()) return false;
        committed_outpoints_.erase(it->second);
        records_.erase(it);
        return true;
    }

    bool IsCommitted(const StakeOutpoint& outpoint) const
    {
        return committed_outpoints_.count(outpoint) != 0;
    }

private:
    std::map<std::string, StakeOutpoint> records_;
    std::set<StakeOutpoint> committed_outpoints_;
    std::set<uint256> used_nonces_;
};

} // namespace Consensus::Hybrid
#endif
