// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL: standalone coinbase payout verification.
// NOT connected to live consensus validation.

#ifndef COOLCOIN_CONSENSUS_HYBRID_COINBASE_PAYOUTS_H
#define COOLCOIN_CONSENSUS_HYBRID_COINBASE_PAYOUTS_H

#include <consensus/hybrid_reward_payments.h>
#include <primitives/transaction.h>

#include <vector>

namespace Consensus::Hybrid {

inline bool VerifyValidatorCoinbasePayments(
    const CTransaction& coinbase,
    const ValidatorPaymentPlan& plan)
{
    if (!coinbase.IsCoinBase() || !plan.valid) {
        return false;
    }

    // Require every expected validator payment to appear as
    // a distinct coinbase output with the correct script and amount.
    std::vector<bool> matched(coinbase.vout.size(), false);

    CAmount verified_total{0};

    for (const auto& payment : plan.payments) {
        if (payment.amount <= 0 ||
            payment.amount > MAX_MONEY ||
            verified_total > MAX_MONEY - payment.amount) {
            return false;
        }

        bool found = false;

        for (size_t i = 0; i < coinbase.vout.size(); ++i) {
            if (matched[i]) {
                continue;
            }

            const auto& output = coinbase.vout[i];

            if (output.nValue == payment.amount &&
                output.scriptPubKey == payment.script) {
                matched[i] = true;
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }

        verified_total += payment.amount;
    }

    return verified_total == plan.distributed;
}

} // namespace Consensus::Hybrid

#endif // COOLCOIN_CONSENSUS_HYBRID_COINBASE_PAYOUTS_H
