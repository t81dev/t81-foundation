#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# T81 Real-Model Validation Experiment (Phases 1 - 9)
# ==============================================================================

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

t81_bin="${T81_BIN:-$repo_root/build/t81}"
make_assess_demo="${MAKE_ASSESS_DEMO:-$repo_root/build/t81_make_assess_fixed_demo}"

if [[ ! -x "$t81_bin" ]]; then
  echo "error: missing t81 binary at $t81_bin" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

tmp_root="$(mktemp -d /tmp/t81-real-model-val.XXXXXX)"
trap 'rm -rf "$tmp_root"' EXIT

model_dir="$tmp_root/models/tiny-random-llama"
mkdir -p "$model_dir"
safetensors_path="$model_dir/model.safetensors"
model_t81w="$tmp_root/tiny-random-llama.t81w"
tampered_t81w="$tmp_root/tiny-random-llama-tampered.t81w"
canon_root="$tmp_root/.t81_canonfs"
mkdir -p "$canon_root"

echo "=========================================================="
echo " T81 Real-Model Validation — Phase 1: Establish Artifact"
echo "=========================================================="

model_source_repo="hf-internal-testing/tiny-random-LlamaForCausalLM"
model_url="https://huggingface.co/hf-internal-testing/tiny-random-LlamaForCausalLM/resolve/main/model.safetensors"

if [[ ! -f "$safetensors_path" ]]; then
  echo "Downloading real Hugging Face model artifact..."
  python3 -c '
import urllib.request, sys
url = "'"$model_url"'"
dst = "'"$safetensors_path"'"
req = urllib.request.Request(url, headers={"User-Agent": "T81-Real-Model-Validator/1.9"})
with urllib.request.urlopen(req) as resp, open(dst, "wb") as f:
    data = resp.read()
    f.write(data)
    print(f"Downloaded {len(data)} bytes from {url}")
'
fi

raw_size="$(python3 -c 'import os; print(os.path.getsize("'"$safetensors_path"'"))')"
raw_sha3_512="$(python3 -c 'import hashlib; print(hashlib.sha3_512(open("'"$safetensors_path"'", "rb").read()).hexdigest())')"
raw_sha3_256="$(python3 -c 'import hashlib; print(hashlib.sha3_256(open("'"$safetensors_path"'", "rb").read()).hexdigest())')"

echo "Source Repo:    $model_source_repo"
echo "Source URL:     $model_url"
echo "Filename:       model.safetensors"
echo "Byte Size:      $raw_size bytes"
echo "Source SHA3-512: $raw_sha3_512"

echo
echo "Importing Hugging Face SafeTensors into native .t81w format..."
"$t81_bin" weights import "$safetensors_path" -o "$model_t81w" >/dev/null

imported_size="$(python3 -c 'import os; print(os.path.getsize("'"$model_t81w"'"))')"
imported_sha3_512="$(python3 -c 'import hashlib; print(hashlib.sha3_512(open("'"$model_t81w"'", "rb").read()).hexdigest())')"

canonfs_model_hash="$("$t81_bin" canonfs put-file "$model_t81w" --canonfs-root "$canon_root")"
canonfs_model_hash="${canonfs_model_hash//$'\n'/}"

echo "Imported .t81w Size:     $imported_size bytes"
echo "Imported File SHA3-512: $imported_sha3_512"
echo "CanonFS Identity:       $canonfs_model_hash"

echo
echo "=========================================================="
echo " T81 Real-Model Validation — Phase 2: Canonical Identity"
echo "=========================================================="

t81_version="$("$t81_bin" --version | head -n 1)"
host_arch="$(uname -m)"
host_os="$(uname -s)"

echo "Artifact SHA3-512: $imported_sha3_512"
echo "CanonFS Hash:      $canonfs_model_hash"
echo "T81 Version:       $t81_version"
echo "Host OS/Arch:      $host_os / $host_arch"
echo "Runtime Mode:      strict_deterministic"
echo "Backend:           t81_reference_vm"

echo
echo "=========================================================="
echo " T81 Real-Model Validation — Phase 3: Axion Governance"
echo "=========================================================="

policy_path="$tmp_root/allow.apl"
cat > "$policy_path" <<EOF
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:$imported_sha3_512"])
  (require-axion-event (reason "task:answer_fixed.v1")))
EOF

policy_hash="$(python3 -c 'import hashlib; print("sha3-256:" + hashlib.sha3_256(open("'"$policy_path"'", "rb").read()).hexdigest())')"
echo "Policy File: $policy_path"
echo "Approved Model Hash: sha3-512:$imported_sha3_512"

echo
echo "=========================================================="
echo " T81 Real-Model Validation — Phase 4: Real Inference"
echo "=========================================================="

prompt_input="hello world"

run1_output="$("$t81_bin" ai task answer-fixed \
  --model tiny-random-llama \
  --model-file "$model_t81w" \
  --policy "$policy_path" \
  --input "$prompt_input" \
  --canonfs-root "$canon_root" \
  --mode strict_deterministic)"

run1_status="$(printf '%s\n' "$run1_output" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("status", ""))')"
run1_result_ref="$(printf '%s\n' "$run1_output" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("result_ref", ""))')"
run1_prov_ref="$(printf '%s\n' "$run1_output" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("provenance_ref", ""))')"
run1_answer="$(printf '%s\n' "$run1_output" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("answer", ""))')"
run1_policy_res="$(printf '%s\n' "$run1_output" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("policy_result", ""))')"

echo "Execution Status: $run1_status"
echo "Policy Verdict:   $run1_policy_res"
echo "Result Ref:       $run1_result_ref"
echo "Provenance Ref:   $run1_prov_ref"
echo "Output Answer:    $run1_answer"

if [[ "$run1_status" != "pass" || "$run1_policy_res" != "allowed" ]]; then
  echo "error: Phase 4 inference failed!" >&2
  exit 1
fi

echo
echo "=========================================================="
echo " T81 Real-Model Validation — Phase 5: Deterministic Replay"
echo "=========================================================="

run2_output="$("$t81_bin" ai task answer-fixed \
  --model tiny-random-llama \
  --model-file "$model_t81w" \
  --policy "$policy_path" \
  --input "$prompt_input" \
  --canonfs-root "$canon_root" \
  --mode strict_deterministic)"

run2_result_ref="$(printf '%s\n' "$run2_output" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("result_ref", ""))')"
run2_prov_ref="$(printf '%s\n' "$run2_output" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("provenance_ref", ""))')"
run2_answer="$(printf '%s\n' "$run2_output" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("answer", ""))')"

echo "Run 1 Result Ref: $run1_result_ref"
echo "Run 2 Result Ref: $run2_result_ref"
echo "Run 1 Prov Ref:   $run1_prov_ref"
echo "Run 2 Prov Ref:   $run2_prov_ref"

replay_pass=true
if [[ "$run1_result_ref" != "$run2_result_ref" || "$run1_prov_ref" != "$run2_prov_ref" || "$run1_answer" != "$run2_answer" ]]; then
  echo "FAIL: Deterministic replay mismatch between Run 1 and Run 2!" >&2
  replay_pass=false
  exit 1
else
  echo "Replay Comparison: EXACT MATCH (run1_result_ref == run2_result_ref)"
fi

echo
echo "=========================================================="
echo " T81 Real-Model Validation — Phase 6: Tamper Experiment"
echo "=========================================================="

cp "$model_t81w" "$tampered_t81w"

# Tamper exactly 1 byte at offset 100
python3 -c '
with open("'"$tampered_t81w"'", "r+b") as f:
    f.seek(100)
    b = f.read(1)[0]
    f.seek(100)
    f.write(bytes([b ^ 0xFF]))
'

tampered_sha3_512="$(python3 -c 'import hashlib; print(hashlib.sha3_512(open("'"$tampered_t81w"'", "rb").read()).hexdigest())')"

echo "Original Model Hash: sha3-512:$imported_sha3_512"
echo "Tampered Model Hash: sha3-512:$tampered_sha3_512"

if [[ "$imported_sha3_512" == "$tampered_sha3_512" ]]; then
  echo "error: Tampered hash failed to change!" >&2
  exit 1
fi

echo "Attempting execution of tampered model artifact under original policy..."
set +e
tamper_err_out="$("$t81_bin" ai task answer-fixed \
  --model tiny-random-llama \
  --model-file "$tampered_t81w" \
  --policy "$policy_path" \
  --input "$prompt_input" \
  --canonfs-root "$canon_root" \
  --mode strict_deterministic 2>&1)"
tamper_exit_code=$?
set -e

echo "Tampered Execution Exit Code: $tamper_exit_code"
echo "Captured Diagnostics:"
echo "$tamper_err_out"

model_tamper_pass=false
if [[ $tamper_exit_code -eq 13 ]] && printf '%s\n' "$tamper_err_out" | grep -q "SecurityFault" && printf '%s\n' "$tamper_err_out" | grep -q "compute executed: no"; then
  echo "Pre-Execution Rejection: CONFIRMED (SecurityFault trapped before inference began)"
  model_tamper_pass=true
else
  echo "FAIL: Tampered artifact was not rejected before execution!" >&2
  exit 1
fi

echo
echo "=========================================================="
echo " T81 Real-Model Validation — Phase 7: Bundle Verification"
echo "=========================================================="

# Create a host action and store a downstream bundle using existing CLI
action_path="$tmp_root/actions/allow.marker"
mkdir -p "$(dirname "$action_path")"
cat > "$action_path" <<EOF
decision=ALLOW
reason_code=VALIDATED_REAL_MODEL
source_result_ref=$run1_result_ref
EOF

action_ref="$("$t81_bin" canonfs put-file "$action_path" --canonfs-root "$canon_root")"
action_ref="${action_ref//$'\n'/}"

record_out="$("$t81_bin" artifact write-store-record \
  --schema t81.ai.task.assess-fixed.host-action-record.v1 \
  --field source_result_ref="$run1_result_ref" \
  --field source_provenance_ref="$run1_prov_ref" \
  --field decision="ALLOW" \
  --field reason_code="VALIDATED_REAL_MODEL" \
  --field termination_reason="single_step_max_score" \
  --field selected_action="write_allow_marker" \
  --field selected_path="actions/allow.marker" \
  --field action_ref="$action_ref" \
  --canonfs-root "$canon_root")"

record_ref="$(printf '%s\n' "$record_out" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("record_ref", ""))')"

bundle_out="$("$t81_bin" artifact store-bundle \
  --schema t81.ai.task.assess-fixed.bundle.v1 \
  --field source_result_ref="$run1_result_ref" \
  --field source_provenance_ref="$run1_prov_ref" \
  --field action_ref="$action_ref" \
  --field record_ref="$record_ref" \
  --canonfs-root "$canon_root")"

bundle_ref="$(printf '%s\n' "$bundle_out" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("bundle_ref", ""))')"

echo "Generated Bundle Ref: $bundle_ref"

bundle_json_path="$tmp_root/bundle.json"
"$t81_bin" canonfs get "$bundle_ref" --canonfs-root "$canon_root" --out "$bundle_json_path" --json >/dev/null

echo "Verifying generated decision bundle..."
bundle_val_out="$("$t81_bin" artifact validate-record "$bundle_json_path" --schema t81.ai.task.assess-fixed.bundle.v1)"
echo "$bundle_val_out"

bundle_val_status="$(printf '%s\n' "$bundle_val_out" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("status", ""))')"

if [[ "$bundle_val_status" != "pass" ]]; then
  echo "FAIL: Bundle verification failed!" >&2
  exit 1
fi

echo
echo "=========================================================="
echo " T81 Real-Model Validation — Phase 8: Bundle Tampering"
echo "=========================================================="

tampered_bundle_path="$tmp_root/bundle_tampered.json"
python3 -c '
import json
data = json.load(open("'"$bundle_json_path"'"))
data["source_result_ref"] = "sha3-256:TAMPERED_INVALID_REF_3D9"
with open("'"$tampered_bundle_path"'", "w") as f:
    json.dump(data, f, indent=2)
'

echo "Attempting verification on tampered bundle artifact..."
set +e
bundle_tamp_val_out="$("$t81_bin" artifact validate-record "$tampered_bundle_path" --schema t81.ai.task.assess-fixed.bundle.v1 2>&1)"
bundle_tamp_code=$?
set -e

echo "Tampered Bundle Validation Exit Code: $bundle_tamp_code"
echo "Captured Output: $bundle_tamp_val_out"

bundle_tamper_pass=false
if [[ $bundle_tamp_code -ne 0 ]] && printf '%s\n' "$bundle_tamp_val_out" | grep -q "invalid"; then
  echo "Bundle Tamper Detection: CONFIRMED (verification failure triggered)"
  bundle_tamper_pass=true
else
  echo "FAIL: Tampered bundle was not detected by verifier!" >&2
  exit 1
fi

echo
echo "=========================================================="
echo " T81 Real-Model Validation — Summary Report"
echo "=========================================================="

echo "REAL MODEL VALIDATION"
echo "====================="
echo
echo "Artifact:      $model_source_repo ($safetensors_path, $raw_size bytes)"
echo "Identity:      $canonfs_model_hash (sha3-512:$imported_sha3_512)"
echo "Policy:        $policy_hash (sha3-512:$imported_sha3_512 bound)"
echo "Execution:     pass ($run1_result_ref)"
echo "Replay:        pass (run1_result_ref == run2_result_ref)"
echo "Bundle:        pass ($bundle_ref)"
echo "Model Tamper:  pass (SecurityFault trapped before inference; exit 13; compute executed: no)"
echo "Bundle Tamper: pass (VERIFICATION FAILURE detected on tampered ref)"
echo
echo "Overall Result:"
echo "PASS"
echo
echo "Strongest Evidence:"
echo "Importing a real Hugging Face SafeTensors model (hf-internal-testing/tiny-random-LlamaForCausalLM) into T81 establishes a bit-exact canonical identity ($canonfs_model_hash). Executing the model under an Axion policy produces identical, machine-verifiable task result references across repeated runs ($run1_result_ref). Modifying a single byte of the model artifact causes immediate pre-execution policy rejection with SecurityFault (exit code 13, compute executed: no) prior to inference."
echo
echo "First Failure Boundary:"
echo "None. All 8 validation phases (artifact import, canonical identity, policy authorization, deterministic inference, replay equality, pre-execution model tamper rejection, bundle creation/verification, and bundle tamper detection) completed successfully."
echo
echo "Production-Relevant Limitation:"
echo "The current native VM execution path processes float-quantized SafeTensors and T3_K ternary models under bounded single-step or multi-step decode horizons; full production serving requires expanded GPU/accelerator backends and streaming KV-cache hardware support."
echo
echo "Recommended Next Experiment:"
echo "Execute a real-world multi-token GGUF model (such as Qwen/Qwen2.5-0.5B-Instruct-GGUF) through the native T3_K quantized GGUF import lane with carried KV-cache state under strict deterministic Axion governance."
echo
