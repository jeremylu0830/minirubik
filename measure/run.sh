#!/usr/bin/env bash
# Run a Ripes assembly program at several SIZE values and record retired
# instructions, Ripes-reported execution time, and peak host RSS per run.
#
# The source must contain a line of the form ".equ SIZE, <value>"; that line
# is rewritten for each run in a temporary copy, the original is not touched.
#
# Usage: measure/run.sh [-p PROC] [-n REPEATS] [-s SRC] [-o CSV] SIZE...
#   -p PROC     Ripes processor model (default RV32_ISS)
#   -n REPEATS  runs per SIZE (default 3)
#   -s SRC      assembly source (default measure/mem.s)
#   -o CSV      append results to this file (default: stdout only)
# Env: RIPES (path to Ripes binary), TIMEOUT (seconds per run, default 3600)
#
# Output columns (CSV):
#   proc,size_bytes,run,iret,exectime_ms,maxrss_kib
# maxrss_kib is "Maximum resident set size" from GNU time, in KiB.

set -euo pipefail

RIPES=${RIPES:-$HOME/ripes/squashfs-root/AppRun}
TIMEOUT=${TIMEOUT:-3600}
PROC=RV32_ISS
REPEATS=3
SRC=$(dirname "$0")/mem.s
CSV=

while getopts "p:n:s:o:h" opt; do
    case $opt in
    p) PROC=$OPTARG ;;
    n) REPEATS=$OPTARG ;;
    s) SRC=$OPTARG ;;
    o) CSV=$OPTARG ;;
    *) sed -n '2,17p' "$0"; exit 1 ;;
    esac
done
shift $((OPTIND - 1))
[ $# -ge 1 ] || { sed -n '2,17p' "$0"; exit 1; }

[ -x "$RIPES" ] || { echo "Ripes not found at $RIPES" >&2; exit 1; }
[ -x /usr/bin/time ] || { echo "/usr/bin/time (GNU time) is required" >&2; exit 1; }
grep -qE '^[[:space:]]*\.equ[[:space:]]+SIZE[[:space:]]*,' "$SRC" ||
    { echo "$SRC has no '.equ SIZE, ...' line" >&2; exit 1; }

# A SIZE that is zero or not a multiple of 4 makes a p != end loop never stop.
for size in "$@"; do
    bytes=$((size))
    if [ "$bytes" -le 0 ] || [ $((bytes % 4)) -ne 0 ]; then
        echo "SIZE $size must be a positive multiple of 4" >&2
        exit 1
    fi
done

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

header="proc,size_bytes,run,iret,exectime_ms,maxrss_kib"
echo "$header"
if [ -n "$CSV" ] && [ ! -s "$CSV" ]; then echo "$header" > "$CSV"; fi

for size in "$@"; do
    bytes=$((size))
    sed -E "s/^([[:space:]]*\.equ[[:space:]]+SIZE[[:space:]]*,).*/\1 $bytes/" \
        "$SRC" > "$work/prog.s"
    for run in $(seq 1 "$REPEATS"); do
        # Ripes output goes to files: piping its stdout can lose the report.
        if ! timeout "$TIMEOUT" /usr/bin/time -f "%M" -o "$work/rss" \
            "$RIPES" --mode cli --src "$work/prog.s" -t asm --proc "$PROC" \
            --iret --exectime --json --output "$work/report.json" \
            > "$work/stdout" 2> "$work/stderr"; then
            echo "run failed: proc=$PROC size=$bytes run=$run" >&2
            grep -v 'qt.qpa' "$work/stderr" "$work/stdout" >&2 || true
            exit 1
        fi
        read -r iret ms < <(python3 - "$work/report.json" <<'EOF'
import json, sys
d = json.load(open(sys.argv[1]))
pick = lambda word: next(v for k, v in d.items() if word in k.lower())
print(pick("retired"), pick("time"))
EOF
)
        line="$PROC,$bytes,$run,$iret,$ms,$(tail -n1 "$work/rss")"
        echo "$line"
        if [ -n "$CSV" ]; then echo "$line" >> "$CSV"; fi
    done
done
