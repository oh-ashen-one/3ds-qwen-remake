#!/bin/bash
# Ralph loop — Ashen Rift 3DS. Fresh Qwen instance per iteration.
# Memory = git + prd.json + progress.txt. Phase-gated: exits 42 on PHASE-COMPLETE.
# Based on snarktank/ralph (Geoffrey Huntley pattern).
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
MAX_ITERATIONS=${1:-200}
NOTIFY="$ROOT_DIR/scripts/ops/notify.sh"

cd "$ROOT_DIR"
[[ -f prd.json ]] || { echo "Missing prd.json"; exit 1; }
[[ -f progress.txt ]] || printf '# Ralph Progress Log\nStarted: %s\n---\n' "$(date)" > progress.txt

FAIL_STREAK=0
LAST_STORY=""

story_stats() {  # -> "done total"
  python3 - <<'PY'
import json
prd = json.load(open("prd.json"))
s = prd.get("userStories", [])
print(sum(1 for x in s if x.get("passes")), len(s))
PY
}

for i in $(seq 1 "$MAX_ITERATIONS"); do
  echo ""
  echo "=== Ralph iteration $i / $MAX_ITERATIONS (3ds) ==="
  BEFORE=$(story_stats)
  OUTPUT=$(python3 "$SCRIPT_DIR/qwen_iteration.py" 2>&1)
  CODE=$?
  echo "$OUTPUT"
  python3 "$SCRIPT_DIR/strip_markers.py" >/dev/null 2>&1 || true

  if [[ $CODE -eq 42 ]] || echo "$OUTPUT" | grep -q "PHASE-COMPLETE"; then
    exit 42
  fi
  if echo "$OUTPUT" | grep -q "<promise>COMPLETE</promise>"; then
    echo "Ralph completed all stories at iteration $i"
    "$NOTIFY" "🏁 ALL stories pass — loop complete." || true
    exit 0
  fi

  AFTER=$(story_stats)
  STORY_ID=$(echo "$OUTPUT" | grep -oE 'Story US-[0-9]+' | head -1 | cut -d' ' -f2)
  if [[ "$AFTER" != "$BEFORE" ]]; then
    FAIL_STREAK=0
    DONE=${AFTER% *}; TOTAL=${AFTER#* }
    TITLE=$(python3 -c "import json;print(next((s['title'] for s in json.load(open('prd.json'))['userStories'] if s['id']=='$STORY_ID'),''))" 2>/dev/null)
    if (( DONE % 3 == 0 )) && [[ -x tools/shoot.sh ]]; then
      tools/shoot.sh "story-$STORY_ID" >/dev/null 2>&1 || true
      "$NOTIFY" "✅ [$STORY_ID] $TITLE — $DONE/$TOTAL (phase $(cat PHASE))" shots/story-$STORY_ID/*.png || true
    else
      "$NOTIFY" "✅ [$STORY_ID] $TITLE — $DONE/$TOTAL (phase $(cat PHASE))" || true
    fi
  else
    if [[ -n "$STORY_ID" && "$STORY_ID" == "$LAST_STORY" ]]; then
      FAIL_STREAK=$((FAIL_STREAK + 1))
    else
      FAIL_STREAK=1
    fi
    if [[ $FAIL_STREAK -eq 5 ]]; then
      "$NOTIFY" "🔁 RUT [$STORY_ID] — 5 fails. Last verify: $(tail -c 400 "$SCRIPT_DIR/LAST_VERIFY.txt" 2>/dev/null)" || true
    fi
  fi
  LAST_STORY="$STORY_ID"
  sleep 1
done

echo "Ralph hit max iterations ($MAX_ITERATIONS)."
"$NOTIFY" "⏸️ loop hit max iterations without finishing phase $(cat PHASE)" || true
exit 1
