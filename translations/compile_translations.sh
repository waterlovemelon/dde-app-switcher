#!/bin/bash
# Compile .ts -> .qm using Qt6 lrelease from linglong container
# Usage: compile_translations.sh <input.ts> <output.qm>

LRELEASE="/var/lib/linglong/layers/8aac67af772d634de7b0fac4e9683321ee418434ebbecf470a05a6de9376f372/files/lib/qt6/bin/lrelease"
LINGLONG_LIB="/var/lib/linglong/layers/8aac67af772d634de7b0fac4e9683321ee418434ebbecf470a05a6de9376f372/files/lib"

if [ ! -x "$LRELEASE" ]; then
    echo "ERROR: lrelease not found at $LRELEASE" >&2
    echo "Install qt6-l10n-tools or linglong Qt6 runtime" >&2
    exit 1
fi

export LD_LIBRARY_PATH="${LINGLONG_LIB}/x86_64-linux-gnu:${LINGLONG_LIB}/x86_64-linux-gnu/qt6/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

exec "$LRELEASE" "$1" -qm "$2"
