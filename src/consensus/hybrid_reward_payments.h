// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL: off-chain validator reward payment construction.
// NOT integrated with blockchain consensus or coinbase validation.

#ifndef COOLCOIN_CONSENSUS_HYBRID_REWARD_PAYMENTS_H
#define COOLCOIN_CONSENSUS_HYBRID_REWARD_PAYMENTS_H

#include <consensus/hybrid_ledger_rewards.h>
#include <consensus/hybrid_validator_payout.h>

#include <script/script.h>

#include <set>
#include <vector>

namespace Consensus::Hybrid {

struct ValidatorPayment {
    CScript script;
    CAmount amount{0};
};

struct ValidatorPaymentPlan {
    std::vector<ValidatorPayment> payments;
    CAmount distributed{0};
    CAmount undistributed{0};
    bool valid{false};
};

inline ValidatorPaymentPlan BuildValidatorPaymentPlan(
    const LedgerRewardAllocation& allocation,
    CAmount pos_budget = POS_SUBSIDY)
{
    ValidatorPaymentPlan result;

    if (!allocation.valid ||
        pos_budget < 0 ||
        pos_budget > POS_SUBSIDY ||
        allocation.distributed < 0 ||
        allocation.undistributed < 0 ||
        allocation.distributed > pos_budget ||
        allocation.undistributed != pos_budget - allocation.distributed) {
        return result;
    }

    CAmount total{0};
    std::set<std::string> seen_validators;

    for (const auto& validator : allocation.validators) {
        if (validator.reward < 0 ||
            validator.reward > MAX_MONEY ||
            total > MAX_MONEY - validator.reward) {
            return ValidatorPaymentPlan{};
        }

        CScript script;

        if (!ValidatorPayoutScript(validator.validator, script) ||
            !seen_validators.insert(validator.validator).second) {
            return ValidatorPaymentPlan{};
        }

        if (validator.reward > 0) {
            result.payments.push_back({
                script,
                validator.reward
            });
        }

        total += validator.reward;
    }

    if (total != allocation.distributed) {
        return ValidatorPaymentPlan{};
    }

    result.distributed = total;
    result.undistributed = allocation.undistributed;
    result.valid = true;

    return result;
}

} // namespace Consensus::Hybrid

#endif // COOLCOIN_CONSENSUS_HYBRID_REWARD_PAYMENTS_H
