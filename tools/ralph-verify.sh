#!/bin/bash
# Canonical judge: the same chain CI runs. Stories append greps after this.
# Usage: tools/ralph-verify.sh   (add "quick" to skip verify-build/package)
set -uo pipefail
source "$(dirname "$0")/env.sh"
cd "$(dirname "$0")/.."

run() {
  echo "--- $* ---"
  OUT=$("$@" 2>&1)
  CODE=$?
  # Full output only on failure; tail on success (keeps LAST_VERIFY readable).
  if [ $CODE -ne 0 ]; then
    echo "$OUT" | tail -80
    echo "FAILED: $* (exit $CODE)"
    exit $CODE
  fi
  echo "$OUT" | tail -3
}

run make validate-assets
run make test-host
run make
if [ "${1:-}" != "quick" ]; then
  run make verify-build
fi
echo "ralph-verify: ALL GREEN"
