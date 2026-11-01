#!/usr/bin/env bash
# Sends each synthetic page to real models and checks what they read. Costs money and uses
# free-model quota, so it is not part of ./test.sh. Needs OPENROUTER_API_KEY, Qt 6, Python with Pillow.
#
#   OPENROUTER_API_KEY=... tests/eval/run.sh [repeats] [model ...]
set -euo pipefail
cd -- "$(dirname -- "$0")"
: "${OPENROUTER_API_KEY:?Set OPENROUTER_API_KEY}"
REPEATS=${1:-1}; shift || true
MODELS=("$@"); [ ${#MODELS[@]} -gt 0 ] || MODELS=(nvidia/nemotron-3.5-super-vl:free anthropic/claude-haiku-5.5)
WORK=$(mktemp -d); trap 'rm -rf "$WORK"' EXIT
c++ -std=c++17 -fPIC payload.cpp -o "$WORK/payload" $(pkg-config --cflags --libs Qt6Network Qt6Gui Qt6Core)
python3 pages.py "$WORK/pages" >/dev/null
for name in $(python3 -c "import json;print(' '.join(json.load(open('cases.json'))))"); do
  for model in "${MODELS[@]}"; do
    effort=; [[ $model == anthropic/* ]] && effort=none
    for i in $(seq "$REPEATS"); do
      out="$WORK/$name.$(echo "$model" | tr '/:' '__').$i.json"
      "$WORK/payload" "$WORK/pages/$name.png" "$model" $effort > "$WORK/request.json"
      # The tablet waits 180 s; so does this.
      curl -s -m 180 -H "Authorization: Bearer $OPENROUTER_API_KEY" -H "Content-Type: application/json" \
        -d @"$WORK/request.json" https://openrouter.ai/api/v1/chat/completions > "$out" || true
      python3 - "$name" "$model" "$out" <<'PY'
import json, re, sys
name, model, path = sys.argv[1:]
case = json.load(open('cases.json'))[name]
try:
    content = json.load(open(path))['choices'][0]['message']['content']
    answer = json.loads(re.search(r'\{.*\}', content, re.S).group(0))
except Exception:
    print(f'NO ANSWER  {name:16} {model}'); sys.exit()
said = answer.get('text', '') + ' ' + answer.get('command', '')
ok = case['mention'] in said and not (case.get('avoid') and case['avoid'] in answer.get('text', ''))
print(f"{'PASS' if ok else 'FAIL':10} {name:16} {model:36} {answer.get('kind', '?'):6} {answer.get('text', '')[:70]!r}")
PY
    done
  done
done
