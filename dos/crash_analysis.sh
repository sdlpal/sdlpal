#!/bin/bash

EXE="sdlpal.exe"
LOG="crash.log"

if [ ! -f "$EXE" ]; then
    echo "Error: Executable '$EXE' not found."
    exit 1
fi

if [ ! -f "$LOG" ]; then
    echo "Error: Crash log '$LOG' not found."
    exit 1
fi

echo "=== Crash Analysis ==="
signal_line=$(grep -m1 "^Crash:" "$LOG")
if [ -n "$signal_line" ]; then
    echo "$signal_line"
else
    echo "No crash header found."
fi
echo ""

echo "Call stack with source lines:"
grep -o '0x[0-9a-f]\{8\}' "$LOG" | while read addr; do
    line=$(i386-pc-msdosdjgpp-addr2line -e "$EXE" "$addr" 2>/dev/null)
    if [ "$line" = "??:0" ]; then
        line="(no debug info)"
    fi
    printf "  %s  %s\n" "$addr" "$line"
done