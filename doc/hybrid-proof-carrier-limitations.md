# Experimental embedded BOND proof: relay and consensus limitations

The `CSTK` intent and `CSPF` proof carrier are **prototype data
encodings**, not a deployable staking transaction format.

## What the 19th test covers

A BOND transaction carries a validator identifier, referenced outpoint,
compressed public key, nonce and owner ECDSA proof. The preflight reads
these fields from transaction outputs and verifies them against a
caller-provided parent-chain UTXO view, rejecting duplicate proof markers.

## What this does NOT solve

- Normal Bitcoin-derived data-carrier policy may reject these large
  OP_RETURN proof outputs (CSPF is 79 to 143 bytes, plus script overhead),
  or more than one data-carrier output per transaction. **Do not broadcast
  these prototype transactions on mainnet.**
- A signed registration digest does not authorize an actual UTXO spend,
  and BOND does not lock a UTXO at script or consensus level.
- The preflight currently uses a *separately represented* registration
  height and genesis hash; a genuine consensus path must derive those
  values itself, not trust RPC input.
- Real BOND signatures must bind the exact authenticated transaction
  operation and canonical serialization, or an attacker may transplant
  a standalone signature into a different transaction.
- EXIT and WITHDRAW need signed, versioned and enforced transaction rules.
- Node mempool, block validation, durable stake state, reward allocation,
  proposer selection, undo/reorg and rollout remain unimplemented.

**Next integration decision:** prefer a versioned witness/transaction
extension or enforceable script design (requiring explicit network upgrade)
over assuming an OP_RETURN prototype can act as a locked stake UTXO.
