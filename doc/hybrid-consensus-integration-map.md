# Hybrid staking integration map: validated source observations

**Status: engineering map only; no new consensus behavior.** Source inspected
on `feature/hybrid-pos-50000-design` on October 19, 2026.

## Block-connection boundary

- `Chainstate::ConnectBlock()`, `src/validation.cpp:2189`, verifies that
  the UTXO view corresponds to its parent block. It calls `CheckBlock`
  but expressly warns that it does **not** replay all contextual checks.
- In the non-coinbase transaction loop, `Consensus::CheckTxInputs` and
  `SequenceLocks` check inputs and BIP68 sequence finality before script checks.
  `CheckInputScripts` runs around line 2446; `UpdateCoins` applies the
  transaction around line 2459. Any future stake-spend or registration
  validation needs consistent pre- and post-transaction ordering.
- Coinbase amount enforcement is around line 2469 and currently validates
  `fees + GetBlockSubsidy(...)`. A 48/16 hybrid reward split must have its
  **own consensus-validated allocation format** and must preserve total
  emission and existing halving behavior. This code does not implement it.

## Disconnect/reorg boundary

- `Chainstate::DisconnectBlock()` around line 2038 reverses UTXO updates,
  using disk-backed undo records. Validator registration and withdrawal
  state must be undone with exactly the same fork semantics.
- `ConnectTip()` around line 2922 wraps `ConnectBlock()` in a
  `CCoinsViewCache` and flushes that view. If stake state lives outside
  this atomicity boundary, crashes can cause divergence.
- `DisconnectTip()` around line 2811 calls `DisconnectBlock()`.
  Reorganization tests must cover repeated connect/disconnect of identical
  blocks and restarting after reorgs.

## Required preconditions BEFORE editing ConnectBlock

1. Define an unambiguous serialized staking transaction and enforceable
   output locking/exit format, with wallet compatibility and exact version.
2. Define deterministic application order, nonces, UTXO ownership verification,
   and all stake state transitions on each transaction.
3. Define a crash-consistent storage/reconstruction strategy tied to the UTXO
   tip and complete undo/reorg handling; no transient global registry.
4. Define coinbase and PoS payout format, fork-choice, eligibility randomness
   and stake cap anti-Sybil properties.
5. Write isolated functional regtest tests, including rejection of invalid
   blocks rather than only calling header helper functions.

**Do not add a conditional height-50,000 shortcut to ConnectBlock**:
that would be a live consensus fork without the required protocol.
