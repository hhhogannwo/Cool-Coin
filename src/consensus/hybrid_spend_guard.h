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

} // namespace Consensus::Hybrid
#endif
