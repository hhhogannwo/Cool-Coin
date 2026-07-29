#!/usr/bin/env bash
set -Eeuo pipefail
source "$(dirname "$0")/common.sh"
for f in "$RUNTIME/minerd.pid" "$RUNTIME/ckpool.pid"; do
  [[ -f "$f" ]] || continue
  p="$(cat "$f" 2>/dev/null || true)"
  [[ "$p" =~ ^[0-9]+$ ]] && kill "$p" 2>/dev/null || true
  rm -f "$f"
done
[[ -f "$CONF" ]] && rpc stop >/dev/null 2>&1 || true
