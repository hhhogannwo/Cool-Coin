# Galactic Wallet 27.3.3 — Linux development preview packaging

**NOT A PUBLIC STAKING RELEASE.** The hybrid consensus and staking UI are unfinished. This workflow only bundles already-compiled binaries for isolated testing.

## Build on Ubuntu Linux

From the repository root, configure and build the GUI and node with appropriate Qt, SQLite and other development dependencies installed. If an existing build was configured with GUI support:

```bash
cd ~/CoolCoin-Development/CoolCoin-Hybrid
make -C build-hybrid -j2
```

Check the expected executable exists:

```bash
test -x build-hybrid/src/qt/bitcoin-qt && echo "GUI exists"
```

Create the preview archive and SHA-256 checksum:

```bash
bash contrib/galactic/package-linux-preview.sh
```

Output: `dist-galactic/CoolCoin-Galactic-Wallet-27.3.3-dev-linux-x86_64.tar.gz` and corresponding `.sha256`.

## Safety and compatibility

- Package label **27.3.3-dev** does not change the program's compiled version metadata.
- The archive is not a standalone AppImage; it depends on the target machine's libraries.
- No private keys, wallet files, blockchain data or RPC credentials are bundled.
- Do not launch against the default datadir or connect this preview to a production node.
- Verify dependencies, runtime behavior, wallet recovery, signature checks, and distribution licenses before any user release.
- Build and test separate Windows, macOS and Android artifacts on appropriate platforms.
- Never describe this preview as stake-enabled: 22 prototype tests passing does not establish live staking support.

## Publishing later

Once real consensus integration, wallet staking UI, security review, cross-platform build and independent-node tests pass, create a tagged GitHub Release and attach **verified** artifacts and checksums. Do not attach this preview as a stable release.
