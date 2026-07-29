# CoolWallet Portable for Linux x86_64

Run:

```bash
./CoolWallet
```

On first launch it generates private RPC credentials, starts the daemon, creates
or loads `coolwallet`, and opens the GUI. It does not contain the developer's
wallet or RPC password.

Mining is opt-in:

```bash
./scripts/start-mining.sh
```

Stop safely:

```bash
./STOP-COOLWALLET.sh
```

RPC examples:

```bash
./coolwallet-cli getblockchaininfo
./coolwallet-cli getwalletinfo
./coolwallet-cli getpeerinfo
```

Supported release targets must be tested individually. A package built on a very
new Linux distribution can still require a newer glibc than older distributions.
For widest compatibility, compile the binaries on Ubuntu 22.04 or an equivalently
old build environment, then run the validation matrix provided with the builder.
