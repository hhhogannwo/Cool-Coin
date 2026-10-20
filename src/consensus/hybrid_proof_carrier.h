// Copyright (c) 2026 The CoolCoin developers
// EXPERIMENTAL on-transaction signature carrier, not a consensus protocol.
// A signature in OP_RETURN is only a key-control proof. It does not lock coins.
#ifndef COOLCOIN_CONSENSUS_HYBRID_PROOF_CARRIER_H
#define COOLCOIN_CONSENSUS_HYBRID_PROOF_CARRIER_H

#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/script.h>
#include <uint256.h>
#include <vector>

namespace Consensus::Hybrid {

struct StakeProofCarrier {
    CPubKey owner;
    uint256 nonce;
    std::vector<unsigned char> signature;
};

// Version, compressed public key, 32-byte nonce, DER signature length and
// signature. A single canonical OP_RETURN direct-push is mandatory.
inline CScript EncodeStakeProofCarrier(const StakeProofCarrier& proof)
{
    if (!proof.owner.IsFullyValid() || !proof.owner.IsCompressed() ||
        proof.signature.size() < 8 || proof.signature.size() > 72) return {};
    std::vector<unsigned char> bytes{'C','S','P','F',1};
    bytes.insert(bytes.end(), proof.owner.begin(), proof.owner.end());
    bytes.insert(bytes.end(), proof.nonce.begin(), proof.nonce.end());
    bytes.push_back(static_cast<unsigned char>(proof.signature.size()));
    bytes.insert(bytes.end(), proof.signature.begin(), proof.signature.end());
    return CScript() << OP_RETURN << bytes;
}

inline bool DecodeStakeProofCarrier(const CScript& script, StakeProofCarrier& output)
{
    CScript::const_iterator cursor = script.begin();
    opcodetype op;
    std::vector<unsigned char> payload;
    if (!script.GetOp(cursor, op) || op != OP_RETURN ||
        !script.GetOp(cursor, op, payload) || cursor != script.end() ||
        payload.size() < 79 || payload.size() > 143 ||
        payload[0] != 'C' || payload[1] != 'S' ||
        payload[2] != 'P' || payload[3] != 'F' || payload[4] != 1 ||
        payload[70] < 8 || payload[70] > 72 ||
        payload.size() != static_cast<size_t>(71 + payload[70])) return false;
    StakeProofCarrier decoded;
    decoded.owner = CPubKey(payload.begin() + 5, payload.begin() + 38);
    if (!decoded.owner.IsFullyValid() || !decoded.owner.IsCompressed()) return false;
    for (unsigned int n = 0; n < 32; ++n) decoded.nonce.begin()[n] = payload[38 + n];
    decoded.signature.assign(payload.begin() + 71, payload.end());
    if (EncodeStakeProofCarrier(decoded) != script) return false;
    output = std::move(decoded);
    return true;
}

// Any malformed or duplicate dedicated proof marker invalidates the scan.
// Other OP_RETURN payloads are ignored.
inline bool ScanStakeProofCarrier(const CTransaction& tx, StakeProofCarrier& result)
{
    bool found = false;
    StakeProofCarrier candidate;
    for (const auto& out : tx.vout) {
        const CScript& script = out.scriptPubKey;
        CScript::const_iterator cursor = script.begin();
        opcodetype op;
        std::vector<unsigned char> bytes;
        if (!script.GetOp(cursor, op) || op != OP_RETURN ||
            !script.GetOp(cursor, op, bytes) || bytes.size() < 4 ||
            bytes[0] != 'C' || bytes[1] != 'S' ||
            bytes[2] != 'P' || bytes[3] != 'F') continue;
        if (found || out.nValue != 0 || !DecodeStakeProofCarrier(script, candidate))
            return false;
        found = true;
    }
    if (!found) return false;
    result = std::move(candidate);
    return true;
}

} // namespace Consensus::Hybrid
#endif
