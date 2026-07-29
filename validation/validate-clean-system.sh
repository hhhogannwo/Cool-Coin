#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="${1:-$(pwd)}"
cd "$ROOT"
echo "Distribution:"
cat /etc/os-release || true
echo
./VERIFY-PORTABILITY.sh
echo
timeout 180 ./scripts/start-node.sh
./coolwallet-cli getblockchaininfo
./coolwallet-cli listwallets
./STOP-COOLWALLET.sh
echo "VALIDATION PASSED"
