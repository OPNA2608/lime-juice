#!/bin/bash
#
# lime-juice: C++ port of Tomyun's "Juice" de/recompiler for PC-98 games
# Copyright (C) 2026 Fuzion
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# corpus test: decompile every MES file, report per-game results
# uses default engine (AI5) unless overridden via ENGINE env var

JUICE="../build/juice"
CORPUS="${1:-/workspace/lime-juice/files_for_testing/full-test-corpus}"
ENGINE="${ENGINE:-}"
EXTRAOP="${EXTRAOP:-}"

if [ ! -f "$JUICE" ]; then
    echo "error: juice binary not found at $JUICE"
    exit 1
fi

# strip ANSI escape codes from a string
strip_ansi() {
    sed 's/\x1b\[[0-9;]*m//g'
}

total_games=0
total_files=0
total_ok=0
total_fail=0
total_warn=0

engine_flag=""
if [ -n "$ENGINE" ]; then
    engine_flag="-e $ENGINE"
fi

extraop_flag=""
if [ -n "$EXTRAOP" ]; then
    extraop_flag="-E"
fi

echo "=== juice corpus test ==="
if [ -n "$ENGINE" ]; then
    echo "engine override: $ENGINE"
fi

if [ -n "$EXTRAOP" ]; then
    echo "extraop: enabled"
fi
echo ""

for gamedir in "$CORPUS"/*/; do
    gamename=$(basename "$gamedir")
    mesfiles=()

    while IFS= read -r -d '' f; do
        mesfiles+=("$f")
    done < <(find "$gamedir" -iname "*.mes" -print0 2>/dev/null)

    if [ ${#mesfiles[@]} -eq 0 ]; then
        continue
    fi

    total_games=$((total_games + 1))

    # print game header
    echo -e "\033[97m--- $gamename (${#mesfiles[@]} files) ---\033[0m"

    game_ok=0
    game_fail=0
    game_warn=0
    first_error=""

    for mesfile in "${mesfiles[@]}"; do
        total_files=$((total_files + 1))

        # run juice, capture raw output and strip ANSI for matching
        raw_output=$("$JUICE" -d -f $engine_flag $extraop_flag "$mesfile" 2>&1)
        clean=$(echo "$raw_output" | strip_ansi)

        if echo "$clean" | grep -q '\.rkt$'; then
            # clean success (no warning text)
            game_ok=$((game_ok + 1))
            total_ok=$((total_ok + 1))
        elif echo "$clean" | grep -q '\.rkt'; then
            # .rkt present but with warning text after it
            game_warn=$((game_warn + 1))
            total_warn=$((total_warn + 1))
            echo "  $clean"
        else
            # failure
            game_fail=$((game_fail + 1))
            total_fail=$((total_fail + 1))
            if [ -z "$first_error" ]; then
                first_error="$clean"
            fi
        fi
    done

    # print game summary
    if [ "$game_fail" -eq 0 ] && [ "$game_warn" -eq 0 ]; then
        echo -e "  \033[92mOK\033[0m  ${game_ok}/${#mesfiles[@]}"
    elif [ "$game_fail" -eq 0 ]; then
        echo -e "  \033[93mWARN\033[0m  ${game_ok} ok, ${game_warn} warn / ${#mesfiles[@]}"
    else
        echo -e "  \033[91mFAIL\033[0m  ${game_ok} ok, ${game_fail} fail / ${#mesfiles[@]}"
        if [ -n "$first_error" ]; then
            echo "  first error: $first_error"
        fi
    fi
    echo ""
done

echo "=== summary ==="
echo "games: $total_games"
echo "files: $total_files (ok: $total_ok, warn: $total_warn, fail: $total_fail)"

if [ "$total_fail" -eq 0 ] && [ "$total_warn" -eq 0 ]; then
    echo ""
    echo -e "\033[92mall files decompiled successfully!\033[0m"
fi
