#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$ROOT/plugins"
bad=0
for b in "$ROOT/CoolWallet.real" "$ROOT/bin/coolcoind" "$ROOT/bin/coolcoin-cli" "$ROOT/bin/ckpool" "$ROOT/bin/minerd"; do
  echo "Checking $b"
  [[ -x "$b" ]] || { echo "NOT EXECUTABLE"; bad=1; continue; }
  missing="$(ldd "$b" 2>&1 | grep 'not found' || true)"
  [[ -z "$missing" ]] || { echo "$missing"; bad=1; }
done
[[ -f "$ROOT/plugins/platforms/libqxcb.so" ]] || echo "NOTICE: libqxcb.so not found; GUI may use a different Qt platform plugin."
[[ -d "$ROOT/data/blocks" && -d "$ROOT/data/chainstate" ]] || { echo "Blockchain snapshot missing"; bad=1; }
grep -RIE '^(rpcuser|rpcpassword)=' "$ROOT" --exclude='initialize.sh' --exclude='VERIFY-PORTABILITY.sh' && { echo "Pre-generated credentials found"; bad=1; } || true
exit "$bad"
