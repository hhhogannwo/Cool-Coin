# CoolCoin hybrid PoW + PoS: block 50,000 proposal

**Status: DESIGN ONLY. This is not an active consensus upgrade.**

## Economic rules agreed
- Activation target: mainnet block **50,000**, contingent on testing and network readiness.
- Continue SHA-256 PoW; introduce genuine PoS validation rather than passive holder payouts.
- Target 10 minutes per block.
- Before first halving: **64 COOL combined subsidy** split as **48 COOL PoW + 16 COOL PoS**.
- First halving remains at block **500,000**; planned post-halving split 24 PoW + 8 PoS. Verify real subsidy code before implementing.
- Minimum qualified stake: **1,000 COOL**.
- Maximum *effective* stake per participant: **250,000 COOL**. More owned COOL does not produce additional staking weight.
- Theoretical expected validator/staker share = `effective_stake / total_active_effective_stake`. Actual earned amounts will vary with participation and validator assignment.
- Preserve existing blockchain history and existing balances.

## Security blockers to resolve before activation

1. **Cap enforcement:** A per-address, per-wallet, per-device, or per-validator-key cap does NOT enforce a per-person limit. A person can split stake over many keys. Decide whether to use a clearly disclosed per-staking-identity cap or a privacy-sensitive identity verification mechanism. Never promise Sybil-proof per-person enforcement without a design.
2. **Genuine PoS consensus:** Specify validator registration, eligibility, stake commitments, deterministic or verifiably unpredictable election, block approval votes, quorum, penalties, and rewards.
3. **Fork choice and attacks:** Specify how PoW accumulated work and PoS attestations work together, reorganizations, equivocation, grinding, and 51%/stake-majority attacks.
4. **Stake lifecycle:** Decide staking maturation, lock-up, unbonding, inactivity and lost-key behavior. No locking period has yet been approved.
5. **Reward accounting:** Deterministic integer rounding, remainder handling, miner and validator outputs, fees, immature rewards, missed validators, and no-staker scenarios must be consensus rules. Never mint more than the total permitted subsidy.
6. **Activation safety:** All participating nodes, miners/pools and wallets must upgrade before the activated rules are enforced; ensure pre-activation blocks still validate identically. Build and test against regtest/signet/testnet first.
7. **Supply accounting:** Audit actual initial subsidy, genesis subsidy, halving and any special issuance against the 64,000,000 COOL target.
8. **Checkpoint and source consistency:** Review shipped chainparams, genesis, checkpoints, ports, chain identifier, consensus parameters and canonical block histories. Do not rely on stale Bitcoin-derived values.

## Test gates (must pass before any production deployment)
- Historical mainnet block replay through height 49,999; no wallet or UTXO divergence.
- Regtest split simulations: 5 participants, 2 large participants, 100,000 simulated participants, stake at 999/1000/250000/250001, and attempts to bypass cap using many keys.
- Round-trip block validation with proper PoW and PoS commitments and signatures; rejects missing/invalid votes.
- Test failed/stale validators, no eligible stakers, long-range attacks and network partitions.
- Rewards exactly reconcile and remain below subsidy across heights 49,999; 50,000; 50,001; 499,999; 500,000.
- Version compatibility test: old nodes must not silently accept new invalid blocks; clear network upgrade guidance.
- Independent code review and reproducible builds.

## Implementation locations for initial investigation
- `src/kernel/chainparams.cpp`: mainnet chain consensus settings (including existing 500,000-block halving interval).
- `src/consensus/params.h`: chain-specific consensus parameters.
- `src/validation.cpp`: block validation and contextual checks.
- `src/primitives/block.h`, `src/primitives/transaction.h`, `src/pow.cpp`: likely block and PoW-related rules.
- Wallet, mining, RPC, functional tests, and GUI require coordinated upgrades.

**Do not patch only the block subsidy or activation height and call it PoW+PoS. That would not implement secure hybrid consensus.**
