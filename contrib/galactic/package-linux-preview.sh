#!/usr/bin/env bash
# Package an existing CoolCoin Linux development build without private data.
# Usage: bash contrib/galactic/package-linux-preview.sh [build-directory]
set -Eeuo pipefail
umask 077

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)"
build="${1:-$repo/build-hybrid}"
build="$(cd "$build" && pwd -P)"
version="27.3.3-dev"
name="CoolCoin-Galactic-Wallet-${version}-linux-x86_64"
out="$repo/dist-galactic"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT

if [[ "$(uname -s)" != Linux || "$(uname -m)" != x86_64 ]]; then
    echo "ERROR: Linux x86_64 only" >&2; exit 1
fi
if [[ "$build" == "$HOME/.bitcoin"* || "$build" == "$HOME/.coolcoin"* ]]; then
    echo "ERROR: refusing wallet data directory" >&2; exit 1
fi
if [[ ! -x "$build/src/qt/bitcoin-qt" ]]; then
    echo "ERROR: GUI missing: $build/src/qt/bitcoin-qt" >&2
    echo "Build the GUI first; test_bitcoin alone is not a wallet application." >&2
    exit 1
fi
for bin in bitcoin-qt bitcoind bitcoin-cli; do
    if [[ "$bin" == bitcoin-qt ]]; then file="$build/src/qt/$bin"; else file="$build/src/$bin"; fi
    [[ -x "$file" ]] || { echo "ERROR: missing $file" >&2; exit 1; }
done
mkdir -p "$stage/$name/bin" "$out"
for bin in bitcoin-qt bitcoind bitcoin-cli; do
    if [[ "$bin" == bitcoin-qt ]]; then file="$build/src/qt/$bin"; else file="$build/src/$bin"; fi
    install -m 0755 "$file" "$stage/$name/bin/$bin"
done
cat > "$stage/$name/README.txt" <<'EOF'
CoolCoin Galactic Wallet v27.3.3 DEVELOPMENT PREVIEW
NOT A STAKING-READY RELEASE. NOT FOR PRODUCTION FUNDS.
The version is a package label, not proof that application version metadata
or a complete staking GUI has been updated.
The hybrid consensus protocol and staking payouts are not implemented.
No wallet.dat, wallet directory, seed, key, configuration, or blockchain
data is included. Back up existing wallet data before any software change.
Do not run this preview against your production datadir or production node.
For isolated testing, explicitly use a separate -datadir and -regtest.
Dependencies (including Qt) are not bundled; this is NOT an AppImage.
EOF
printf '%s\n' '#!/usr/bin/env bash' 'set -euo pipefail' 'here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"' 'echo "DEVELOPMENT PREVIEW: not staking-ready; do not use production data."' 'echo "Run manually with an isolated datadir and -regtest."' 'echo "Example: $here/bin/bitcoin-qt -regtest -datadir=/path/to/isolated-datadir"' > "$stage/$name/START-README.sh"
chmod 0755 "$stage/$name/START-README.sh"
tar -C "$stage" -czf "$out/$name.tar.gz" "$name"
(cd "$out" && sha256sum "$name.tar.gz" > "$name.tar.gz.sha256")
echo "Created: $out/$name.tar.gz"
echo "Checksum: $out/$name.tar.gz.sha256"
echo "Do not publish as a staking-ready release."
