// Copyright (c) 2026 The CoolCoin developers
// Distributed under the MIT software license.
//
// EXPERIMENTAL read-only UTXO inspection. Does NOT establish stake ownership
// or lockup, and must not be called proof of stake consensus validation.
#ifndef COOLCOIN_CONSENSUS_HYBRID_UTXO_H
#define COOLCOIN_CONSENSUS_HYBRID_UTXO_H

#include <coins.h>
#include <consensus/hybrid_admission.h>
#include <consensus/consensus.h>
#include <primitives/transaction.h>

#include <cstdint>

namespace Consensus::Hybrid {

// Read only from a caller-provided UTXO view representing a specified tip.
// Neither wallets nor untrusted RPC clients may supply "authorized" flags.
inline ValidatedStakeEvidence InspectStakeOutpoint(const CCoinsView& view,
                                                    const StakeOutpoint& outpoint,
                                                    uint32_t tip_height)
{
    ValidatedStakeEvidence result;
    result.outpoint = outpoint;
    const COutPoint chain_outpoint{Txid::FromUint256(outpoint.txid), outpoint.vout};
    Coin coin;
    if (!view.GetCoin(chain_outpoint, coin) || coin.IsSpent()) return result;
    if (coin.nHeight > tip_height) return result; // fail closed on inconsistent view
    if (!MoneyRange(coin.out.nValue) || coin.out.nValue < MINIMUM_STAKE) return result;
    if (coin.out.scriptPubKey.empty() || coin.out.scriptPubKey.IsUnspendable()) return result;
    // Coinbase funds must mature before any staking eligibility assessment.
    if (coin.IsCoinBase() && tip_height - coin.nHeight + 1 < COINBASE_MATURITY) return result;
    result.amount = coin.out.nValue;
    result.confirmations = tip_height - coin.nHeight + 1;
    result.unspent = true;
    // These two flags remain false until consensus-level ownership and
    // lock validation is implemented. Register() MUST reject this evidence.
    result.owner_authorized = false;
    result.lock_enforced = false;
    return result;
}

} // namespace Consensus::Hybrid
#endif
