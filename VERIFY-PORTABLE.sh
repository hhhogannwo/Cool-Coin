#!/usr/bin/env bash
set -u

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DATA="$ROOT/data"
CONF="$DATA/coolcoin.conf"
CLI="$ROOT/bin/coolcoin-cli"
CKLOG="$ROOT/logs/ckpool-console.log"
MINERLOG="$ROOT/logs/minerd.log"

PASS=0
FAIL=0

pass()
{
    echo "PASS: $*"
    PASS=$((PASS + 1))
}

fail()
{
    echo "FAIL: $*"
    FAIL=$((FAIL + 1))
}

echo "=================================================="
echo "       COOLWALLET PORTABLE FINAL VERIFICATION"
echo "=================================================="

echo
echo "===== EXECUTABLES ====="

for file in \
    CoolWallet \
    CoolWallet.real \
    bin/coolcoind \
    bin/coolcoin-cli \
    bin/ckpool \
    bin/minerd \
    scripts/common.sh \
    scripts/start-node.sh \
    scripts/start-mining.sh \
    scripts/stop-mining.sh
do
    if [ -x "$ROOT/$file" ]; then
        pass "$file"
    else
        fail "$file is missing or not executable"
    fi
done

echo
echo "===== SHARED LIBRARIES ====="

for file in \
    "$ROOT/CoolWallet.real" \
    "$ROOT/bin/coolcoind" \
    "$ROOT/bin/coolcoin-cli" \
    "$ROOT/bin/ckpool" \
    "$ROOT/bin/minerd"
do
    missing="$(
        env LD_LIBRARY_PATH="$ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
        ldd "$file" 2>/dev/null |
        grep 'not found' || true
    )"

    if [ -z "$missing" ]; then
        pass "$(basename "$file") libraries"
    else
        fail "$(basename "$file") has missing libraries"
        printf '%s\n' "$missing"
    fi
done

echo
echo "===== NODE AND BLOCKCHAIN ====="

if "$CLI" -datadir="$DATA" -conf="$CONF" \
    getblockchaininfo >/tmp/coolwallet-chain-info.$$ 2>/dev/null
then
    BLOCKS="$(grep '"blocks"' /tmp/coolwallet-chain-info.$$ | head -1)"
    HEADERS="$(grep '"headers"' /tmp/coolwallet-chain-info.$$ | head -1)"
    IBD="$(grep '"initialblockdownload"' /tmp/coolwallet-chain-info.$$ | head -1)"

    echo "$BLOCKS"
    echo "$HEADERS"
    echo "$IBD"

    if grep -q '"initialblockdownload": false' \
        /tmp/coolwallet-chain-info.$$
    then
        pass "blockchain is synchronized"
    else
        fail "blockchain is still synchronizing"
    fi
else
    fail "portable RPC is unavailable"
fi

rm -f /tmp/coolwallet-chain-info.$$

echo
echo "===== WALLET ====="

WALLETS="$(
    "$CLI" -datadir="$DATA" -conf="$CONF" \
    listwallets 2>/dev/null || true
)"

if grep -q '"coolwallet"' <<<"$WALLETS"; then
    pass "coolwallet is loaded"
else
    fail "coolwallet is not loaded"
fi

echo
echo "===== NETWORK ====="

CONNECTIONS="$(
    "$CLI" -datadir="$DATA" -conf="$CONF" \
    getconnectioncount 2>/dev/null || echo 0
)"

if [[ "$CONNECTIONS" =~ ^[0-9]+$ ]] &&
   [ "$CONNECTIONS" -gt 0 ]
then
    pass "$CONNECTIONS peer connection(s)"
else
    fail "no peer connections"
fi

echo
echo "===== MINING PROCESSES ====="

if pgrep -x ckpool >/dev/null; then
    pass "CKPool is running"
else
    fail "CKPool is not running"
fi

if pgrep -x minerd >/dev/null; then
    pass "CPU miner is running"
else
    fail "CPU miner is not running"
fi

if ss -ltn 2>/dev/null |
    grep -qE '127\.0\.0\.1:3333|0\.0\.0\.0:3333'
then
    pass "Stratum port 3333 is listening"
else
    fail "Stratum port 3333 is not listening"
fi

echo
echo "===== CKPOOL CONNECTION ====="

if grep -q 'Connected to bitcoind' "$CKLOG" 2>/dev/null; then
    pass "CKPool connected to the Cool Coin node"
else
    fail "CKPool did not connect to the Cool Coin node"
fi

if grep -qE '1 users +1 workers|[1-9][0-9]* users +[1-9][0-9]* workers' \
    "$CKLOG" 2>/dev/null
then
    pass "Stratum worker is registered"
else
    fail "no registered Stratum worker"
fi

echo
echo "===== LIVE HASHING ====="

if grep -qE 'hashes, +[0-9.]+ +khash/s|hashes, +[0-9.]+ +Mhash/s' \
    "$MINERLOG" 2>/dev/null
then
    pass "live SHA-256d hashes are being produced"
    grep -E 'hashes, +[0-9.]+ +(khash|Mhash)/s' \
        "$MINERLOG" |
        tail -5
else
    fail "no live hash output found"
fi

if grep -q 'Stratum requested work restart' \
    "$MINERLOG" 2>/dev/null
then
    pass "miner is receiving updated work"
else
    fail "no Stratum work restart found"
fi

echo
echo "===== CURRENT ERROR CHECK ====="

ERRORS="$(
    grep -Ei \
    '401 Unauthorized|No bitcoinds active|CRITICAL|stratum_subscribe timed out|connection refused' \
    "$CKLOG" "$MINERLOG" 2>/dev/null || true
)"

if [ -z "$ERRORS" ]; then
    pass "no continuing RPC or Stratum errors"
else
    fail "mining errors were found"
    printf '%s\n' "$ERRORS"
fi

echo
echo "===== PORTABILITY CHECK ====="

HARDCODED="$(
    grep -RInE \
    '/mnt/storage/CoolCoin|CoolWallet-Portable-Release|/home/mrcool[0-9]*/' \
    "$ROOT/scripts" \
    "$ROOT/config" \
    "$ROOT/CoolWallet" \
    "$ROOT/START-COOLWALLET.sh" \
    "$ROOT/STOP-COOLWALLET.sh" \
    2>/dev/null || true
)"

# The current extracted directory can legitimately occur in a generated
# runtime CKPool configuration. Exclude that exact current path.
HARDCODED="$(
    printf '%s\n' "$HARDCODED" |
    grep -vF "$ROOT/" || true
)"

if [ -z "$HARDCODED" ]; then
    pass "no foreign development paths"
else
    fail "foreign hard-coded paths found"
    printf '%s\n' "$HARDCODED"
fi

echo
echo "=================================================="
echo "PASSED: $PASS"
echo "FAILED: $FAIL"
echo "=================================================="

if [ "$FAIL" -eq 0 ]; then
    echo "FINAL RESULT: COOLWALLET PORTABLE PASSED"
    exit 0
else
    echo "FINAL RESULT: COOLWALLET PORTABLE NEEDS ATTENTION"
    exit 1
fi
