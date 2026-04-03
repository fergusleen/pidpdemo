#!/bin/sh

set -e

SCRIPT_DIR=`cd "\`dirname "$0"\`" && pwd`
BASE_DIR=`cd "$SCRIPT_DIR/.." && pwd`
LOG_DIR="$BASE_DIR/logs"
PAGE_LOG="$LOG_DIR/smoke-page.log"
MISS_PAGE_LOG="$LOG_DIR/smoke-missing-page.log"
MISS_CMD_LOG="$LOG_DIR/smoke-missing-command.log"

mkdir -p "$LOG_DIR"

cd "$BASE_DIR"
make clean all

printf "1\nQ\n" | ./pidpdemo -p -d pages > "$PAGE_LOG"
printf "2\nQ\n" | ./pidpdemo -p -d no-such-pages > "$MISS_PAGE_LOG"
printf "B\nQ\n" | ./pidpdemo -p -d pages > "$MISS_CMD_LOG"

grep "PiDP-11 Demonstration System" "$PAGE_LOG" >/dev/null
grep "This is a working PDP-11 style Unix system" "$PAGE_LOG" >/dev/null
grep "could not be opened" "$MISS_PAGE_LOG" >/dev/null
grep "not available on this system" "$MISS_CMD_LOG" >/dev/null

echo "Smoke test passed."
