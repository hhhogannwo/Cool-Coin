echo "== Deploying Qt runtime =="

pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-qt5-tools

WINDEPLOYQT="$(find /ucrt64/bin -maxdepth 1 -type f -iname '*deployqt*.exe' | head -n1)"

if [[ ! -f "$WINDEPLOYQT" ]]; then
    echo "ERROR: windeployqt.exe not found"
    find /ucrt64/bin -maxdepth 1 -iname '*deployqt*' -print
    exit 1
fi

echo "Using: $WINDEPLOYQT"

"$WINDEPLOYQT" \
    --release \
    --no-translations \
    "$STAGE/CoolWallet.exe"
