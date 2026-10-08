#!/bin/sh
# Assemble one RV32I source with the generated tables appended and run it on
# Ripes RV32_ISS, printing the a0-a7 registers and the retired-instruction
# count.
#
# Usage (from the repository root):
#   tools/ripes.sh lesson1.s            program + tables.s
#   tools/ripes.sh lesson1.s --no-tables
#   tools/ripes.sh lesson1.s --all-regs print x0-x31 instead of a0-a7
#
# The Ripes report goes to a file first: piping its stdout directly sometimes
# yields empty output.
set -eu

RIPES=${RIPES:-$HOME/ripes/squashfs-root/AppRun}
src=${1:?usage: tools/ripes.sh FILE.s [--no-tables] [--all-regs]}
shift
tables=tables.s
all_regs=0
for arg in "$@"; do
    case $arg in
    --no-tables) tables= ;;
    --all-regs) all_regs=1 ;;
    *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT INT TERM

if [ -n "$tables" ]; then
    [ -f "$tables" ] || { echo "$tables not found; run ./gen_tables > tables.s" >&2; exit 1; }
    # The echo keeps the program's last line apart from the tables when the
    # program does not end in a newline.
    { cat "$src"; echo; cat "$tables"; } >"$tmp/build.s"
else
    cp "$src" "$tmp/build.s"
fi

"$RIPES" --mode cli --src "$tmp/build.s" -t asm --proc RV32_ISS --regs --iret \
    >"$tmp/out.txt" 2>&1 || true

if grep -q 'ERROR' "$tmp/out.txt"; then
    grep -v '^qt\.' "$tmp/out.txt" >&2
    exit 1
fi

if [ "$all_regs" -eq 1 ]; then
    grep -E '^x[0-9]+:' "$tmp/out.txt"
else
    # a0-a7 are x10-x17.
    grep -E '^x1[0-7]:' "$tmp/out.txt" |
        sed -E 's/^x1([0-7]):/a\1 (x1\1):/'
fi
# The count is on the line after the "instructions retired" header.
iret=$(sed -n '/instructions retired/{n;p;q}' "$tmp/out.txt")
echo "instructions retired: $iret"
