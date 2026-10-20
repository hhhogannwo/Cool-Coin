# CoolCoin hybrid PoS staking deposit and withdrawal design (DRAFT)

**Status: design only; no consensus activation.** This document specifies candidate
rules for a future hard fork at height 50,000. Values and transaction structures
are not finalized. No mainnet node should deploy these unimplemented rules.

## Existing mechanisms verified in source

- `src/script/interpreter.cpp` handles `OP_CHECKLOCKTIMEVERIFY` and
  `OP_CHECKSEQUENCEVERIFY` when their respective script verification flags
  are enabled. Both check transaction context rather than maintaining a
  persistent validator registration.
- `ConnectBlock` in `src/validation.cpp` calls `CheckTxInputs` and
  `SequenceLocks`; this is the enforcement path we would need to extend
  for consensus-level staking restrictions.
- An ordinary spendable UTXO is NOT a staking deposit. A validator-key
  signature is NOT the same as authorization by the UTXO's spending script.

## Proposed on-chain lifecycle

`UNBONDED -> BOND_PENDING -> BONDED -> EXIT_PENDING -> WITHDRAWN`

1. **Bond creation**: A wallet creates an explicit, versioned staking-deposit
   transaction with stake amount, beneficiary/withdrawal key commitment,
   validator key, chain domain, nonce, and activation policy. The script and
   registration must be *consensus recognizable*, not simply wallet labels.
2. **Bond admission**: A new consensus rule checks deposit existence, amount
   (at least 1,000 COOL), unspent status and coinbase maturity, the spending
   authority for the deposit, nonce uniqueness, and deterministic eligibility
   height. Registration must be linked to a concrete outpoint and verified
   from transaction/witness data, never caller-supplied `owner_authorized` flags.
3. **Bonded period**: Spending the bonded deposit outside an allowed staking
   transition must be consensus-invalid. A future spend guard must consult
   deterministic validator state assembled from previously validated blocks;
   the current `StakeAdmissionBook` is insufficient.
4. **Exit request**: Owner authorizes exit under a defined on-chain format.
   Stop using stake for future validator selection at a deterministic height.
   Define a penalty/challenge/unbonding period before funds can be spent.
5. **Withdrawal**: After unbonding, owner may spend the bonded UTXO through
   a specifically validated withdrawal path. Never permit someone holding
   only the validator key to withdraw the staker's funds.
6. **Reorgs**: Every state transition and consumed registration nonce must be
   reapplied/undone when blocks connect/disconnect, including withdrawals
   and same-block conflicts. Restart must reconstruct identical state.

## Security boundaries

**CLTV/CSV alone are insufficient**: They can constrain when a script is
spendable but cannot by themselves track validator participation, prevent
reuse of a stake outpoint across registrations, or implement early-exit
restrictions if unrestricted alternate spend paths exist.

**No privileged unlock**: Do not use arbitrary booleans, RPC commands,
validator signatures alone, wall clock time, or local registry entries as
consensus proof of an allowed spend. Use validated on-chain state.

**Fairness limitation**: The 250,000 COOL effective stake cap is per validator
identity unless a verifiable identity aggregation rule is adopted. Users can
create multiple keys. Do not claim a per-person cap.

## Outstanding consensus decisions before implementing script

- Choose deposit script structure: covenant-compatible script design,
  explicit new transaction type with a consensus-validated state transition,
  or another reviewed mechanism. Existing CLTV/CSV support alone does not
  satisfy withdrawal/registration semantics.
- Fix whether the unbonding delay is measured in blocks or median-time-past,
  the length of the delay, how equivocation evidence is adjudicated, and how
  penalties affect stake.
- Determine activation timing and ability to opt out on legacy nodes.
- Define validator selection, snapshot timing, randomness, and fork-choice.
- Define deterministic 48/16 block payouts across halvings and whether
  empty PoS allocations remain unminted.
- Build regtest scenarios: bond -> activate -> illegal spend rejection ->
  exit -> early withdrawal rejection -> valid withdrawal; plus forks/reorgs,
  crashes, duplicate/parallel registration, replay across networks,
  invalid signatures and malicious block producers.

**Release gate:** Only after these choices are specified, implemented in
block consensus, tested on isolated regtest networks, and independently
reviewed can a live-network upgrade be considered. Keep production chain,
pool, datadir and current wallet keys separate from this branch.
