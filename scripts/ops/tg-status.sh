#!/bin/bash
# tg-status.sh — manual poke: story table + fresh screenshot to Telegram.
# Runs on the STUDIO.
set -uo pipefail
cd "$(dirname "$0")/../.."
STATUS=$(python3 - <<'PY'
import json
prd = json.load(open("prd.json"))
s = prd.get("userStories", [])
done = sum(1 for x in s if x.get("passes"))
cur = next((x for x in s if not x.get("passes")), None)
lines = [f"stories {done}/{len(s)}  phase {open('PHASE').read().strip()}"]
if cur:
    lines.append(f"current: {cur['id']} {cur['title']}")
lines.append("")
for x in s:
    lines.append(f"{'✅' if x.get('passes') else '·'} {x['id']} {x['title'][:40]}")
print("\n".join(lines[:40]))
PY
)
tools/shoot.sh poke >/dev/null 2>&1 || true
scripts/ops/notify.sh "$STATUS" shots/poke/*.png
