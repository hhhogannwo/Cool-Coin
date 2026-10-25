// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL: bind a BOND intent to a verified owner of an unspent P2WPKH
// output. This does NOT implement coin locking, script consensus, or PoS.
#ifndef COOLCOIN_CONSENSUS_HYBRID_BOND_AUTH_H
#define COOLCOIN_CONSENSUS_HYBRID_BOND_AUTH_H

#include <consensus/hybrid_event_envelope.h>
#include <consensus/hybrid_owner_proof.h>
#include <consensus/hybrid_utxo.h>
#include <hash.h>

namespace Consensus::Hybrid {

// 'tip_height' is the parent UTXO height; 'registration_height' is the
// candidate block height. Inputs must be taken from chainstate after ordinary
// script checks, never client-supplied metadata. Caller must still enforce
// locking and prevent spending this outpoint in the same block.
inline bool VerifyBondIntentOwner(const CCoinsView& view,
                                  const StakeEnvelope& intent,
                                  const CPubKey& key,
                                  const std::vector<unsigned char>& signature,
                                  const uint256& genesis,
                                  const uint256& nonce,
                                  uint32_t tip_height,
                                  uint32_t registration_height)
{
    if (intent.intent != StakeIntent::BOND ||
        !key.IsFullyValid() || !key.IsCompressed() ||
        intent.validator_id != Hash160(key) ||
        registration_height != tip_height + 1 ||
        tip_height == UINT32_MAX) return false;

    const auto evidence = InspectStakeOutpoint(view, intent.outpoint, tip_height);
    if (!evidence.unspent || evidence.confirmations < PROPOSED_MIN_CONFIRMATIONS)
        return false;

    Coin coin;
    const COutPoint outpoint(Txid::FromUint256(intent.outpoint.txid), intent.outpoint.vout);
    if (!view.GetCoin(outpoint, coin) || coin.IsSpent()) return false;
    const uint256 digest = RegistrationDigest(genesis, key,
        intent.outpoint.txid, intent.outpoint.vout, evidence.amount,
        registration_height, nonce);
    return VerifyP2WPKHRegistration(coin.out, key, digest, signature);
}


// Experimental verification against a caller-supplied transaction-bound digest.
// The caller must construct this digest from the authenticated transaction,
// excluding its canonical proof carrier.
inline bool VerifyBondIntentOwnerDigest(
    const CCoinsView& view,
    const StakeEnvelope& intent,
    const CPubKey& key,
    const std::vector<unsigned char>& signature,
    const uint256& digest,
    uint32_t tip_height,
    uint32_t registration_height)
{
    if (intent.intent != StakeIntent::BOND ||
        !key.IsFullyValid() || !key.IsCompressed() ||
        intent.validator_id != Hash160(key) ||
        tip_height == UINT32_MAX ||
        registration_height != tip_height + 1) return false;

    const auto evidence = InspectStakeOutpoint(
        view, intent.outpoint, tip_height);

    if (!evidence.unspent ||
        evidence.confirmations < PROPOSED_MIN_CONFIRMATIONS)
        return false;

    Coin coin;
    const COutPoint outpoint(
        Txid::FromUint256(intent.outpoint.txid),
        intent.outpoint.vout);

    if (!view.GetCoin(outpoint, coin) || coin.IsSpent())
        return false;

    return VerifyP2WPKHRegistration(
        coin.out, key, digest, signature);
}

} // namespace Consensus::Hybrid
#endif
