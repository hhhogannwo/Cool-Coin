# Hybrid CoolCoin fork at height 50,000: implementation safety gate

Status: **DESIGN/PROTOTYPE ONLY — NOT ACTIVATED**.

The `src/consensus/hybrid_*.h` helpers are isolated; their passing unit
tests do not imply consensus enforcement. Never deploy the current development
branch to a production node or run it against an existing wallet/data directory.

## Confirmed development defaults (not finalized consensus rules)

- Proposed activation block: 50,000.
- PoW: SHA-256; pre-halving subsidy proposal 48 COOL per block.
- PoS: proposed 16 COOL per block, shared proportionally across eligible stake.
- Eligibility: 1,000 COOL minimum, 250,000 COOL maximum *effective* weight.
- Eligibility maturity: proposed 100 confirmations.

**Activation requires full-node agreement.** Merely specifying the height in
`hybrid_params.h` does not implement the fork. Existing supply and halving
rules must be reviewed so reward allocations cannot inflate total issuance.

## Blocking decisions / implementation work

1. **Consensus-visible staking deposits**: Specify an actual serialized
   on-chain registration and deposit format. Validate committed outpoints
   against the parent block UTXO view and enforce ownership using the relevant
   script or a separate explicitly signed spending authorization. A validator
   key signature alone proves control of a validator key, *not* deposited funds.
2. **Locks and withdrawal**: Define a consensus-enforced staking covenant or
   registration mechanism, activation period, minimum bond, cooldown, exit
   semantics and penalties. The current in-memory SpendGuard is **not**
   transaction validation. Reject ordinary spends of locked UTXOs in consensus,
   while providing a valid withdrawal transition.
3. **Durable, reorg-safe state**: Recompute deterministic validator state on
   every ConnectBlock and reverse it on DisconnectBlock (or equivalent), with
   crash-safe persistence and same-block ordering defined. The current maps
   and nonce set are volatile and can be bypassed after restart.
4. **Validator eligibility and anti-Sybil fairness**: A 250,000 COOL cap per
   arbitrary validator ID can be bypassed by creating more IDs. Choose a rule
   that is actually verifiable without a central identity authority, or
   document that the cap only limits each validator registration.
5. **PoS selection/security**: Define consensus-authenticated randomness,
   eligibility snapshot timing, equivocation defense, winner verification,
   finality/fork choice, missed-validator handling, PoW coexistence, and
   double-signing consequences.
6. **Reward validation**: Specify how 16 COOL is allocated and included in a
   block, exact rounding and unminted remainder behavior, deterministic
   recipient ordering, empty-validator behavior, coinbase maturity and fees.
   Check emissions and subsidy halvings across activation and beyond.
7. **Activation and networking**: Add explicit consensus enforcement at the
   activation boundary, block/header serialization rules if changed,
   version handshakes, chain split/fork protections, and transition behavior
   for older nodes. A schedule based solely on an assumed future height
   does not provide a safe activation plan.
8. **Tests before release**: Unit, property/fuzz, functional multi-node and
   regtest tests for double-registration, chain reorgs, replays, withdrawals,
   immature UTXOs, restart persistence, incentives and malicious blocks.
   Perform external security review and coordinate miner/node upgrades.

## Current testing scope

- Prototype key-signature registration test passes.
- Prototype stake admission checks pass using caller-supplied booleans; **not
  proof of coin ownership or a locked output**.
- UTXO view smoke/in-memory cache tests pass for value, confirmations,
  immature coinbase, missing/spent/unspendable outputs.
- Volatile committed-output spend detection test passes, but is **not wired
  to real mempool or block validation**.

## Explicit safety rules

- Preserve chain history, money supply and all existing UTXOs.
- Do not modify the live pool at port 3333, the live node's data directory,
  existing private key/address prefixes, or production binaries.
- Do not merge or release as mainnet-ready without an independently reviewed,
  fully tested consensus specification.
