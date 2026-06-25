#!/bin/bash
# Run all Blargg cpu_instrs individual ROMs against ./bin/gbtest.
# Usage: tools/run_blargg.sh [dir-with-gb-files]
#
# Default dir: test-roms/cpu_instrs/individual
# Exit code: 0 if all pass, 1 otherwise.

set -u

BIN=build/gbtest
DIR="${1:-test-roms/cpu_instrs/individual}"

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not found. Run 'make bin/gbtest' first." >&2
    exit 2
fi
if [ ! -d "$DIR" ]; then
    echo "error: $DIR not found. Run 'git submodule update --init'." >&2
    exit 2
fi

# Ordered list (Blargg expects earlier tests to pass before later ones).
# Filenames contain spaces/commas, so we use a bash array.
ROMS=(
    "01-special.gb"
    "02-interrupts.gb"
    "03-op sp,hl.gb"
    "04-op r,imm.gb"
    "05-op rp.gb"
    "06-ld r,r.gb"
    "07-jr,jp,call,ret,rst.gb"
    "08-misc instrs.gb"
    "09-op r,r.gb"
    "10-bit ops.gb"
    "11-op a,(hl).gb"
)

pass=0
fail=0
total=${#ROMS[@]}
failed_list=""

for name in "${ROMS[@]}"; do
    rom="$DIR/$name"
    if [ ! -f "$rom" ]; then
        printf "[%2d/%d] %-30s ... MISSING\n" "$((pass + fail + 1))" "$total" "$name"
        fail=$((fail + 1))
        failed_list="$failed_list\n  $name (missing)"
        continue
    fi

    # Run the harness; stdout = serial output, stderr = verdict line.
    out=$("./$BIN" "$rom" 2>/tmp/gbtest.err)
    rc=$?

    case "$rc" in
        0) verdict="PASS";    pass=$((pass + 1)) ;;
        1) verdict="FAIL";    fail=$((fail + 1)); failed_list="$failed_list\n  $name" ;;
        2) verdict="TIMEOUT"; fail=$((fail + 1)); failed_list="$failed_list\n  $name (timeout)" ;;
        *) verdict="ERR(rc=$rc)"; fail=$((fail + 1)); failed_list="$failed_list\n  $name (rc=$rc)" ;;
    esac

    n=$((pass + fail))
    printf "[%2d/%d] %-30s ... %s\n" "$n" "$total" "$name" "$verdict"

    # For failures, print captured serial output to help debugging.
    if [ "$rc" -ne 0 ] && [ -n "$out" ]; then
        printf "%s\n" "$out" | sed 's/^/        | /'
    fi
done

echo
echo "Riepilogo: $pass/$total PASS, $fail/$total FAIL"
if [ "$fail" -gt 0 ]; then
    printf "Falliti:%b\n" "$failed_list"
    exit 1
fi
exit 0
