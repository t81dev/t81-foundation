#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/../../.." && pwd)"
cd "$repo_root"

t81_bin="${T81_BIN:-$repo_root/build/t81}"
model_builder="${MODEL_BUILDER:-$repo_root/build/t81_make_demo_model_gov}"
program_path="$repo_root/tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81"
deny_policy="$repo_root/examples/governance/governance-demo/deny.apl"
tmp_root="$(mktemp -d "${TMPDIR:-/tmp}/t81-canonfs-proof.XXXXXX")"
canon_root="$tmp_root/.t81_canonfs"
model_path="$tmp_root/demo.t81w"
allow_policy="$tmp_root/allow.apl"
import_json="$tmp_root/import.json"
allow_stdout="$tmp_root/allow.stdout"
allow_stderr="$tmp_root/allow.stderr"
allow_replay_stdout="$tmp_root/allow-replay.stdout"
allow_replay_stderr="$tmp_root/allow-replay.stderr"
deny_stdout="$tmp_root/deny.stdout"
deny_stderr="$tmp_root/deny.stderr"

cleanup() {
  rm -rf "$tmp_root"
}
trap cleanup EXIT

if [[ ! -x "$t81_bin" ]]; then
  echo "error: missing t81 binary at $t81_bin" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

if [[ ! -x "$model_builder" ]]; then
  echo "error: missing demo model builder at $model_builder" >&2
  echo "build it first: cmake --build build --target t81_make_demo_model_gov" >&2
  exit 1
fi

if [[ ! -f "$program_path" ]]; then
  echo "error: missing demo program at $program_path" >&2
  exit 1
fi

if [[ ! -f "$deny_policy" ]]; then
  echo "error: missing deny policy at $deny_policy" >&2
  exit 1
fi

mkdir -p "$canon_root"
"$model_builder" "$model_path" >/dev/null

"$t81_bin" canonfs import \
  "$model_path" \
  --canonfs-root "$canon_root" \
  --json >"$import_json"

canon_hash="$(
  python3 - "$import_json" <<'PY'
import json, sys
with open(sys.argv[1], "r", encoding="utf-8") as fh:
    data = json.load(fh)
print(data["imported_objects"][0])
PY
)"

model_checksum="$(
  "$t81_bin" weights info "$model_path" --json | \
    python3 -c 'import sys, json; print(json.load(sys.stdin)["checksum_sha3_512"])'
)"

cat >"$allow_policy" <<EOF
(policy
  (tier 2)
  (allowed-ternary-model-hashes ["sha3-512:$model_checksum"]))
EOF

normalize_exec_output() {
  local in_path="$1"
  python3 - "$in_path" <<'PY'
import re, sys
with open(sys.argv[1], "r", encoding="utf-8") as fh:
    text = fh.read()
text = re.sub(r"Compilation successful → .*", "Compilation successful", text)
print(text, end="")
PY
}

print_deny_summary() {
  local in_path="$1"
  python3 - "$in_path" <<'PY'
import sys
trap = ""
opcode = ""
compute = ""
with open(sys.argv[1], "r", encoding="utf-8") as fh:
    for raw in fh:
        line = raw.strip()
        if "Execution trapped:" in line:
            trap = line.split("Execution trapped:", 1)[1].strip()
        elif line.startswith("opcode:"):
            opcode = line.split(":", 1)[1].strip()
        elif line.startswith("compute executed:"):
            compute = line.split(":", 1)[1].strip()
print(f"trap: {trap}")
print(f"opcode: {opcode}")
print(f"compute_executed: {compute}")
PY
}

echo "T81 CanonFS + Axion Proof"
echo
echo "This demonstrates:"
echo "- immutable artifact identity"
echo "- deterministic execution"
echo "- pre-execution policy enforcement (no compute on deny)"
echo
echo '$ build/t81 canonfs import demo.t81w --json'
python3 - "$import_json" <<'PY'
import json, sys
with open(sys.argv[1], "r", encoding="utf-8") as fh:
    data = json.load(fh)
print(f'status: {data["status"]}')
print(f'canon_hash: {data["imported_objects"][0]}')
print(f'provenance_schema: {data["provenance_schema"]}')
PY
echo

echo '$ build/t81 code run tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81 --weights-model <CANON_HASH> --policy allow.apl'
T81_CANONFS_ROOT="$canon_root" \
  "$t81_bin" code run "$program_path" --weights-model "$canon_hash" --policy "$allow_policy" \
  >"$allow_stdout" 2>"$allow_stderr"
normalize_exec_output "$allow_stdout"
echo "status: ok"
echo

echo '$ build/t81 code run tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81 --weights-model <CANON_HASH> --policy allow.apl'
T81_CANONFS_ROOT="$canon_root" \
  "$t81_bin" code run "$program_path" --weights-model "$canon_hash" --policy "$allow_policy" \
  >"$allow_replay_stdout" 2>"$allow_replay_stderr"
normalize_exec_output "$allow_replay_stdout"
if [[ "$(normalize_exec_output "$allow_stdout")" == "$(normalize_exec_output "$allow_replay_stdout")" ]] && \
   [[ "$(cat "$allow_stderr")" == "$(cat "$allow_replay_stderr")" ]]; then
  echo "deterministic_replay: yes"
else
  echo "deterministic_replay: no"
  exit 1
fi
echo "status: ok"
echo

echo '$ build/t81 code run tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81 --weights-model <CANON_HASH> --policy deny.apl'
set +e
T81_CANONFS_ROOT="$canon_root" \
  "$t81_bin" code run "$program_path" --weights-model "$canon_hash" --policy "$deny_policy" \
  >"$deny_stdout" 2>"$deny_stderr"
deny_rc=$?
set -e
print_deny_summary "$deny_stderr"
echo "status: error"
echo "reason: policy_denied"
echo "exit_code: $deny_rc"
