// Copyright (c) 2026 The CoolCoin developers
// Distributed under the MIT software license.
//
// EXPERIMENTAL read-only spend-conflict guard prototype. NOT integrated with
// ConnectBlock, mempool policy, persistent state or reorg undo processing.
// A caller must construct the admission book from consensus-verified history;
// otherwise this guard provides NO consensus security.
#ifndef COOLCOIN_CONSENSUS_HYBRID_SPEND_GUARD_H
#define COOLCOIN_CONSENSUS_HYBRID_SPEND_GUARD_H

#include <consensus/hybrid_admission.h>
#include <consensus/hybrid_height_ledger.h>
#include <primitives/transaction.h>
#include <set>
#include <vector>

namespace Consensus::Hybrid {

// Indicates a conflicting spend of a currently booked stake. This function
// intentionally does NOT authorize registration, withdrawal or unlocking.
inline bool SpendsCommittedStake(const CTransaction& transaction,
                                 const StakeAdmissionBook& book)
{
    for (const CTxIn& input : transaction.vin) {
        StakeOutpoint outpoint{input.prevout.hash.ToUint256(), input.prevout.n};
        if (book.IsCommitted(outpoint)) return true;
    }
    return false;
}

// Check transactions against the height-aware ledger, not caller-supplied
// admission flags. The claim remains locked through EXIT_PENDING. This is
// a read-only integration seam: NOT called by ConnectBlock, and does not
// authorize an on-chain withdrawal transaction.
inline bool SpendsHeightLockedStake(const CTransaction& transaction,
                                    const HeightStakeLedger& ledger)
{
    for (const CTxIn& input : transaction.vin) {
        const StakeOutpoint outpoint{input.prevout.hash.ToUint256(), input.prevout.n};
        if (ledger.IsLocked(outpoint)) return true;
    }
    return false;
}

// Experimental block-level conflict scan. Catch a spend against a bond
// established earlier in the chain OR registered within this same block.
// Do not permit WITHDRAW events to bypass this check: unlocking requires
// a separately specified consensus transaction format and spending rules.
// Events are still caller supplied; this is NOT consensus enforcement.
inline bool BlockSpendsBondedStake(
    const std::vector<CTransactionRef>& transactions,
    const std::vector<StakeEvent>& events,
    const HeightStakeLedger& parent_ledger)
{
    std::set<StakeOutpoint> new_bonds;
    for (const auto& event : events) {
        if (event.type == StakeEventType::BOND) {
            new_bonds.insert(event.outpoint);
        }
    }
    for (const auto& tx : transactions) {
        if (!tx) return true; // Invalid transaction reference: fail closed.
        for (const auto& input : tx->vin) {
            const StakeOutpoint spent{input.prevout.hash.ToUint256(), input.prevout.n};
            if (parent_ledger.IsLocked(spent) || new_bonds.count(spent)) return true;
        }
    }
    return false;
}

} // namespace Consensus::Hybrid
#endif
