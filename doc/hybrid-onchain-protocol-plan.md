# CoolCoin hybrid PoW/PoS: on-chain design blockers and proposed protocol

**Status: DESIGN DRAFT ONLY. Do not deploy, activate, or modify mainnet.**
Source integration branch: `feature/hybrid-pos-full-integration`.
Existing unit tests are *prototype* tests, not full consensus tests. New tests on this branch have not yet been run.

## Constraints already chosen for prototype
- Preserve the existing chain and existing balances.
- Candidate hybrid activation: block 50,000, subject to final release validation.
- Minimum stake: 1,000 COOL; maximum **effective** stake: 100,000 COOL per staking identity.
- Candidate block subsidy split before halving: 32 COOL PoW + 32 COOL PoS.
- Eligibility wait: 100 blocks in the height-aware ledger.
- Exit wait: 144 blocks minimum.
- SHA-256 PoW remains. Pool service on port 3333 is production infrastructure and
  MUST NOT be changed by experimental consensus work.

## Mandatory on-chain representation (NOT integrated with consensus)

**Bond**: choose a consensus-recognizable, consensus-enforced UTXO script and
canonical registration payload. Both must be included in a validated transaction.
The registration must bind genesis hash, validator signing key, **original UTXO**,
stake amount, nonce and chain location, without trusting RPC booleans.
The transaction's outpoint must be determined from consensus block transaction
ordering. Define exactly when 100 confirmations are reached.

**Exit**: authenticated consensus transaction referencing the uniquely bonded
outpoint and its validator identity, with exactly one exit per bonded record.
The exit transaction must not allow the bonded UTXO to be spent.

**Withdraw**: an authenticated spending transaction whose original bonded
UTXO remains unspendable until exit height + 144. Define the transaction
script conditions, signatures, fees and output amounts. Authorization MUST be
checked against the *same* data validated by consensus script interpreter;
do not rely on in-memory validator ID strings.

**Important**: A read-only `SpendsHeightLockedStake` or
`BlockSpendsBondedStake` callback cannot implement these rules; those helpers
do not inspect signatures or persist state. Withdrawing must be authorized by a
versioned on-chain format and transaction/script validation.

## Required safety invariants

1. All bonded outpoints stay locked in PENDING, BONDED and EXIT_PENDING.
2. A BOND cannot claim the same outpoint twice, even across different IDs.
3. A withdraw is valid **only** after the exit becomes mature on the active chain.
4. Parent-state validation rejects any exit-and-withdraw attempt in one block.
5. Every connect/disconnect produces identical state/UTXO tip under reorgs.
6. A restart at any block height recovers exactly the same claimed outpoints,
   bond phases, activation and exit heights.
7. The signed registration domain includes chain ID to prevent cross-network
   replay. No staking state or authorization comes from untrusted RPC booleans.
8. Real consensus validation rejects a block that spends a bonded outpoint.
9. Before and at activation, legacy valid blocks remain accepted per explicit
   fork rules; activation logic is tested across multiple nodes.
10. A dishonest validator cannot mint more than the allowed subsidy or
    substitute unauthorized validator payouts.

## Key unresolved protocol choices (REQUIRE AUDITED SPECIFICATION)
- Whether staking uses a new script version, a covenant-like transaction
  restriction, or a dedicated consensus transaction type; Bitcoin-derived
  standard script alone does not provide arbitrary validator state locks.
- How the stake cap handles Sybil splitting among unlimited validator keys.
- How PoS validators are selected and prove eligibility with unpredictable,
  bias-resistant randomness, avoiding stake grinding or proposer equivocation.
- Whether PoS is a co-signer/attestation on SHA-256 blocks or alternates with
  PoW. This affects difficulty, fork choice, reorg finality, and attack security.
- Where validator payout and state commitments are serialized, how they are
  signed/verified, and how subsidy halving changes the 32/32 split.
- How to store stake state and its undo atomically with block UTXO changes.
- Exact network upgrade schedule and compatibility rules.

## Release acceptance gates

- Review and version the complete transaction/locking/signature encoding.
- Implement and test real block validation (ConnectBlock) plus full rollback
  (DisconnectBlock), without global mutable stand-ins.
- Implement deterministic proposer selection and subsidy checks.
- Tests with two or more independently running regtest nodes across activation,
  invalid blocks, deep reorganizations and restart/reindex.
- Audit reward inflation, replay, equivocation, grinding, stake cap, witness
  and authorization bypasses.
- Only then discuss controlled public rollout; do not infer readiness from
  compiling 14 header-only prototype tests.

## Integration branch progress (unverified)

- Updated experimental effective-stake cap and base subsidy constants to 100,000 COOL and 32/32 COOL.
- Added a deterministic height/halving-aware proposed subsidy schedule preserving historical pre-activation rewards.
- Added a structural WITHDRAW preflight against the parent ledger, including exact locked-input matching and withdrawal maturity. **This does not authenticate the spending script or authorize funds by itself.**
- Added unit-test cases for the proposed subsidy schedule and structural withdrawal preflight; tests have **not** been run.
- **Not implemented:** authenticated EXIT proof, production `ConnectBlock`/`DisconnectBlock` stake state integration, atomic persistence, validator selection/attestations, miner payout creation, or full network activation.
- Do not merge or deploy this branch until all consensus invariants and independent-node test gates pass.
