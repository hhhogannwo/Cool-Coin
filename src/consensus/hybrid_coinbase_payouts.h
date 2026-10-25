// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL: standalone coinbase payout verification.
// NOT connected to live consensus validation.

#ifndef COOLCOIN_CONSENSUS_HYBRID_COINBASE_PAYOUTS_H
#define COOLCOIN_CONSENSUS_HYBRID_COINBASE_PAYOUTS_H

#include <consensus/hybrid_reward_payments.h>
#include <primitives/transaction.h>

#include <set>
#include <vector>

namespace Consensus::Hybrid {

// EXPERIMENTAL: caller must independently validate the subsidy split,
// transaction fees, and validator payment plan.
inline bool VerifyValidatorCoinbasePayments(
    const CTransaction& coinbase,
    const ValidatorPaymentPlan& plan,
    CAmount pow_budget,
    CAmount pos_budget,
    CAmount fees)
{
    if (!coinbase.IsCoinBase() || !plan.valid ||
        pow_budget < 0 || pos_budget < 0 || fees < 0 ||
        pow_budget > TOTAL_SUBSIDY ||
        pos_budget > POS_SUBSIDY ||
        pow_budget > TOTAL_SUBSIDY - pos_budget ||
        fees > MAX_MONEY - pow_budget - pos_budget ||
        plan.distributed < 0 ||
        plan.distributed > pos_budget ||
        plan.undistributed != pos_budget - plan.distributed) {
        return false;
    }

    std::set<CScript> expected_scripts;
    CAmount expected_total{0};

    for (const auto& payment : plan.payments) {
        if (payment.amount <= 0 ||
            payment.amount > MAX_MONEY ||
            expected_total > MAX_MONEY - payment.amount ||
            !expected_scripts.insert(payment.script).second) {
            return false;
        }
        expected_total += payment.amount;
    }

    if (expected_total != plan.distributed) {
        return false;
    }

    std::set<CScript> paid_scripts;
    CAmount miner_total{0};
    CAmount output_total{0};

    for (const auto& output : coinbase.vout) {
        if (output.nValue < 0 ||
            output.nValue > MAX_MONEY ||
            output_total > MAX_MONEY - output.nValue) {
            return false;
        }

        output_total += output.nValue;

        if (expected_scripts.count(output.scriptPubKey)) {
            bool found = false;

            for (const auto& payment : plan.payments) {
                if (payment.script == output.scriptPubKey) {
                    if (output.nValue != payment.amount ||
                        !paid_scripts.insert(output.scriptPubKey).second) {
                        return false;
                    }
                    found = true;
                    break;
                }
            }

            if (!found) {
                return false;
            }
        } else {
            if (miner_total > MAX_MONEY - output.nValue) {
                return false;
            }
            miner_total += output.nValue;
        }
    }

    return paid_scripts.size() == expected_scripts.size() &&
           miner_total <= pow_budget + fees &&
           output_total <= pow_budget + fees + plan.distributed;
}

} // namespace Consensus::Hybrid

#endif // COOLCOIN_CONSENSUS_HYBRID_COINBASE_PAYOUTS_H
