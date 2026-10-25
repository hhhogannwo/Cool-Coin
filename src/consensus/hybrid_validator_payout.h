// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL: validator payout script construction only.
// NOT integrated with blockchain consensus or coinbase validation.

#ifndef COOLCOIN_CONSENSUS_HYBRID_VALIDATOR_PAYOUT_H
#define COOLCOIN_CONSENSUS_HYBRID_VALIDATOR_PAYOUT_H

#include <addresstype.h>
#include <script/script.h>
#include <uint256.h>

#include <string>

namespace Consensus::Hybrid {

// The validator ID must already have been authenticated as
// Hash160(owner public key) during bond registration.
// Reject noncanonical IDs rather than allowing permissive SetHex parsing.
inline bool ValidatorPayoutScript(const std::string& validator_id,
                                 CScript& payout_script)
{
    if (validator_id.size() != 40) return false;

    for (const char c : validator_id) {
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }

    uint160 keyhash;
    keyhash.SetHex(validator_id);

    payout_script = GetScriptForDestination(WitnessV0KeyHash(keyhash));
    return true;
}

} // namespace Consensus::Hybrid

#endif // COOLCOIN_CONSENSUS_HYBRID_VALIDATOR_PAYOUT_H
