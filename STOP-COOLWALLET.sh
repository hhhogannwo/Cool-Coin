#!/usr/bin/env bash

ROOT="$(cd -- "$(dirname -- "$0")" && pwd)"
source "$ROOT/scripts/common.sh"

"$ROOT/scripts/stop-mining.sh" 2>/dev/null || true

pkill -f "$ROOT/CoolWallet.real" 2>/dev/null || true

cli stop >/dev/null 2>&1 || true

echo "CoolWallet stopped."
