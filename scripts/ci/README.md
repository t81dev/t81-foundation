# scripts/ci

CI policy and reproducibility gate scripts used by `.github/workflows/ci.yml`.

## Categories
- Reproducibility gates: `t81lang_repro_gate.py`, `t3k_repro_gate.py`, `llama_cpp_repro_gate.py`
- Determinism slice runner: `run_determinism_slice.sh`
- Governance/audit checks: workflow action pinning, permissions, architecture sync
- Numeric policy checks: legacy include/type usage and wrapper-thinness guards
- Benchmark guards: `check_simd_regression.py`, `check_vm_workload_benchmark_regression.py`
- CLI docs parity guard: `check_cli_docs_parity.py`
- CLI docs smoke guard: `check_cli_docs_smoke.py`
- CLI JSON contract guard: `check_cli_json_contracts.py`
- AI sandbox boundary guard: `check_ai_experiment_boundary.py`
- AI status document freshness guard: `check_ai_status_doc_freshness.py`
- AI status document freshness expectation contract (max-age failures + warn-age near-stale warnings): `ai_status_doc_freshness_expectations.json`
- AI evidence bundle collector: `collect_ai_evidence_bundle.py`
- AI model provenance hash gate: `check_ai_model_provenance_gate.py`
- AI model provenance signing keyring: `ai_model_provenance_keyring.json`
- AI policy event contract gate: `check_ai_policy_event_contract.py`
- RFC-0026 WLOAD policy-evidence readiness tracker gate: `check_ai_wload_policy_evidence.py`
- RFC-0026 WLOAD policy-evidence expectation contract (readiness, reason-code prefix policy, minimum observed count): `ai_wload_policy_evidence_expectations.json`
- AI policy Axion ledger signing keyring: `ai_policy_ledger_keyring.json`
- AI backend adapter contract gate: `check_ai_backend_adapter_contract.py`
- AI backend selection manifest signing keyring: `ai_backend_selection_keyring.json`
- AI opcode subset contract gate: `check_ai_opcode_subset_contract.py`
- AI opcode runtime evidence report: `generate_ai_opcode_runtime_report.py`
- AI opcode runtime provenance consistency gate: `check_ai_opcode_runtime_provenance.py`
- RFC-0026 runtime readiness tracker gate: `check_ai_rfc0026_readiness.py`
- AI opcode baseline history approval policy gate: `check_ai_opcode_baseline_history_approvals.py`
- AI opcode baseline approval signing keyring: `ai_opcode_baseline_approval_keyring.json`
- AI opcode baseline provenance expectation contract (required fields, safe-path, prefix/existence policy): `ai_opcode_baseline_provenance_expectations.json`
- AI benchmark spec contract gate: `check_ai_benchmark_spec_contract.py`
- AI benchmark format/mode capability matrix gate: `check_ai_benchmark_capability_matrix.py`
- AI benchmark capability expectation contract: `ai_benchmark_capability_expectations.json`
- AI inference format/mode capability matrix gate: `check_ai_inference_capability_matrix.py`
- AI inference capability expectation contract: `ai_inference_capability_expectations.json`
- Runtime lane capability alignment gate: `check_ai_runtime_capability_alignment.py`
- Runtime lane capability alignment expectation contract: `ai_runtime_capability_alignment_expectations.json`
- AI benchmark threshold approval policy gate: `check_ai_benchmark_threshold_approvals.py`
- RFC-0026 readiness expectation contract (readiness state, WLOAD evidence signal, blocker taxonomy): `ai_rfc0026_readiness_expectations.json`
- AI benchmark threshold approval signing keyring: `ai_benchmark_threshold_approval_keyring.json`
- AI benchmark runtime threshold baseline: `ai_benchmark_thresholds.json`
- AI benchmark threshold history windows: `ai_benchmark_thresholds_history.json`
- AI quantization codec contract gate: `check_ai_quantization_codec_contract.py`
- AI quantization profile approval policy gate: `check_ai_quantization_profile_approvals.py`
- AI quantization profile approval signing keyring: `ai_quantization_profile_approval_keyring.json`
- AI quantization codec profile baseline: `ai_quantization_codec_profile.json`
- AI quantization codec profile history windows: `ai_quantization_codec_profile_history.json`
- AI UX contract gate: `check_ai_ux_contract.py`
- Direct backend attestation signing keyring: `ai_direct_backend_attestation_keyring.json`
- AI cross-lane evidence lock gate: `check_ai_cross_lane_evidence.py`
- AI signed multi-lane evidence manifest gate: `check_ai_evidence_manifest.py`
- AI evidence manifest signing keyring: `ai_evidence_manifest_keyring.json`
- AI keyring expiry alert gate: `check_ai_keyring_expiry.py`
- AI keyring KMS metadata contract gate: `check_ai_keyring_kms_contract.py`
- RFC-0025 canonize-tensor toolchain gate: `check_ai_tloadhash_toolchain.py`
- Governed llama runtime evidence runner: `run_governed_llama_flow.py`
- Governed llama multi-seed replay attestation gate: `check_ai_governed_replay_attestation.py`
- Governed replay attestation signing keyring: `ai_governed_replay_keyring.json`

## Local invocation examples
```bash
python3 scripts/ci/check_architecture_targets.py
python3 scripts/ci/check_vm_workload_benchmark_regression.py bench-vm-workload.json
python3 scripts/ci/check_cli_docs_parity.py --t81-bin build/t81 --manual docs/guides/cli-user-manual.md
python3 scripts/ci/check_cli_docs_smoke.py --manual docs/guides/cli-user-manual.md --cwd . --timeout-sec 20
python3 scripts/ci/check_cli_json_contracts.py --t81-bin build/t81 --repo-root .
python3 scripts/ci/check_ai_experiment_boundary.py
python3 scripts/ci/check_ai_status_doc_freshness.py --expectations-file scripts/ci/ai_status_doc_freshness_expectations.json --out-json build/ai-status/ai_status_doc_freshness_report.json  # also writes .md summary
python3 scripts/ci/collect_ai_evidence_bundle.py --ai-bin build/experiments/ai/ux_tools/t81_ai --out-dir build/ai-evidence --runs 3 --model-fixture tests/fixtures/llama_cpp_repro/model.gguf
python3 scripts/ci/check_ai_model_provenance_gate.py --model build/ai-provenance/test_model.gguf --manifest build/ai-provenance/test_model.manifest.json --signing-keyring scripts/ci/ai_model_provenance_keyring.json --min-lineage-entries 2 --required-lineage-events artifact_ingest,artifact_promotion_candidate --self-test-deny
python3 scripts/ci/check_ai_policy_event_contract.py --out-dir build/ai-policy --ledger-keyring scripts/ci/ai_policy_ledger_keyring.json
python3 scripts/ci/check_ai_wload_policy_evidence.py --policy-contract build/ai-policy/ai_policy_event_contract.json --runtime-trace build/ai-policy/ai_runtime_trace.json --expectations-file scripts/ci/ai_wload_policy_evidence_expectations.json --out-json build/ai-policy/ai_wload_policy_evidence.json
python3 scripts/ci/check_ai_backend_adapter_contract.py --out-dir build/ai-backend --ai-bin build/experiments/ai/ux_tools/t81_ai --runtime-model build/ai-backend/runtime_backend_probe_model.gguf --policy-contract build/ai-policy/ai_policy_event_contract.json --runtime-trace build/ai-policy/ai_runtime_trace.json --policy-ledger-snapshot build/ai-policy/ai_axion_policy_ledger_snapshot.json --selection-signing-keyring scripts/ci/ai_backend_selection_keyring.json
python3 scripts/ci/check_ai_opcode_subset_contract.py --out-dir build/ai-opcodes --runtime-report build/ai-opcodes-runtime/ai_opcode_runtime_report.json --ctest-log build/ai-opcodes/ai_phase1_opcode_ctest.log
python3 scripts/ci/generate_ai_opcode_runtime_report.py --repo-root . --out-dir build/ai-opcodes-runtime --ctest-log build/ai-opcodes/ai_phase1_opcode_ctest.log --baseline-hashes experiments/ai/opcodes/PHASE1_BASELINE_HASHES.json --baseline-hashes-history experiments/ai/opcodes/PHASE1_BASELINE_HASHES_HISTORY.json --as-of-date 2026-03-05
python3 scripts/ci/check_ai_opcode_runtime_provenance.py --runtime-report build/ai-opcodes-runtime/ai_opcode_runtime_report.json --provenance-expectations scripts/ci/ai_opcode_baseline_provenance_expectations.json --out-json build/ai-opcodes-runtime/ai_opcode_runtime_provenance_report.json
python3 scripts/ci/check_ai_rfc0026_readiness.py --opcode-report build/ai-opcodes-runtime/ai_opcode_runtime_report.json --benchmark-capability-matrix build/ai-benchmark/ai_benchmark_capability_matrix.json --inference-capability-matrix build/ai-ux/ai_inference_capability_matrix.json --runtime-capability-alignment build/ai-opcodes-runtime/ai_runtime_capability_alignment.json --wload-policy-evidence-report build/ai-policy/ai_wload_policy_evidence.json --opcode-baseline-approval-report build/ai-opcodes-runtime/ai_opcode_baseline_approval_report.json --expectations-file scripts/ci/ai_rfc0026_readiness_expectations.json --out-dir build/ai-opcodes-runtime
python3 scripts/ci/check_ai_opcode_baseline_history_approvals.py --history-file experiments/ai/opcodes/PHASE1_BASELINE_HASHES_HISTORY.json --signing-keyring scripts/ci/ai_opcode_baseline_approval_keyring.json --provenance-expectations scripts/ci/ai_opcode_baseline_provenance_expectations.json --out-json build/ai-opcodes-runtime/ai_opcode_baseline_approval_report.json
python3 scripts/ci/check_ai_benchmark_spec_contract.py --out-dir build/ai-benchmark --ai-bin build/experiments/ai/ux_tools/t81_ai --runtime-model tests/fixtures/llama_cpp_repro/model.gguf --thresholds-file scripts/ci/ai_benchmark_thresholds.json --thresholds-history-file scripts/ci/ai_benchmark_thresholds_history.json --as-of-date 2026-03-05 --trend-window-count 3
python3 scripts/ci/check_ai_benchmark_capability_matrix.py --out-dir build/ai-benchmark --ai-bin build/experiments/ai/ux_tools/t81_ai --model-file tests/fixtures/llama_cpp_repro/model.gguf --formats gguf,t3k --modes strict_deterministic --expectations-file scripts/ci/ai_benchmark_capability_expectations.json --required gguf:strict_deterministic
python3 scripts/ci/check_ai_benchmark_threshold_approvals.py --history-file scripts/ci/ai_benchmark_thresholds_history.json --signing-keyring scripts/ci/ai_benchmark_threshold_approval_keyring.json --out-json build/ai-benchmark/ai_benchmark_threshold_approval_report.json
python3 scripts/ci/check_ai_inference_capability_matrix.py --out-dir build/ai-ux --ai-bin build/experiments/ai/ux_tools/t81_ai --model-file tests/fixtures/llama_cpp_repro/model.gguf --formats gguf,t3k --modes strict_deterministic --expectations-file scripts/ci/ai_inference_capability_expectations.json --required gguf:strict_deterministic
python3 scripts/ci/check_ai_runtime_capability_alignment.py --benchmark-matrix build/ai-benchmark/ai_benchmark_capability_matrix.json --inference-matrix build/ai-ux/ai_inference_capability_matrix.json --required-pairs gguf:strict_deterministic,t3k:strict_deterministic --expectations-file scripts/ci/ai_runtime_capability_alignment_expectations.json --out-json build/ai-opcodes-runtime/ai_runtime_capability_alignment.json
python3 scripts/ci/check_ai_quantization_codec_contract.py --out-dir build/ai-quantization --ai-bin build/experiments/ai/ux_tools/t81_ai --runtime-model tests/fixtures/llama_cpp_repro/model.gguf --codec-profile-file scripts/ci/ai_quantization_codec_profile.json --codec-profile-history-file scripts/ci/ai_quantization_codec_profile_history.json --as-of-date 2026-03-05 --trend-window-count 3
python3 scripts/ci/check_ai_quantization_profile_approvals.py --history-file scripts/ci/ai_quantization_codec_profile_history.json --signing-keyring scripts/ci/ai_quantization_profile_approval_keyring.json --out-json build/ai-quantization/ai_quantization_profile_approval_report.json
python3 scripts/ci/check_ai_ux_contract.py --ai-bin build/experiments/ai/ux_tools/t81_ai --out-dir build/ai-ux --runtime-model tests/fixtures/llama_cpp_repro/model.gguf --t81-bin build-llama-ai/t81 --llama-hash-probe scripts/ci/llama_model_hash.py --direct-backend-signing-keyring scripts/ci/ai_direct_backend_attestation_keyring.json
python3 scripts/ci/check_ai_cross_lane_evidence.py --out-dir build/ai-cross-lane --evidence-bundle build/ai-evidence/ai_evidence_bundle.json --ux-contract build/ai-ux/ai_ux_contract.json --ux-inference build/ai-ux/ai_inference_run.json --ux-quantization build/ai-ux/ai_quantization_inspect.json --ux-benchmark build/ai-ux/ai_benchmark_run.json --tloadhash-toolchain build/ai-rfc0025/ai_tloadhash_toolchain.json --governed-flow build/ai-governed/governed_llama_flow.json
python3 scripts/ci/check_ai_evidence_manifest.py --out-dir build/ai-manifest --evidence-bundle build/ai-evidence/ai_evidence_bundle.json --vm-trace build/ai-vm-trace/ai_vm_trace_evidence.json --cross-lane build/ai-cross-lane/ai_cross_lane_evidence.json --backend-contract build/ai-backend/ai_backend_adapter_contract.json --ux-contract build/ai-ux/ai_ux_contract.json --tloadhash-toolchain build/ai-rfc0025/ai_tloadhash_toolchain.json --signing-keyring scripts/ci/ai_evidence_manifest_keyring.json --promotion-window-start 2026-03-01 --promotion-window-end 2026-03-31
python3 scripts/ci/check_ai_keyring_expiry.py --keyring scripts/ci/ai_evidence_manifest_keyring.json --keyring scripts/ci/ai_model_provenance_keyring.json --keyring scripts/ci/ai_policy_ledger_keyring.json --keyring scripts/ci/ai_backend_selection_keyring.json --keyring scripts/ci/ai_governed_replay_keyring.json --keyring scripts/ci/ai_direct_backend_attestation_keyring.json --keyring scripts/ci/ai_benchmark_threshold_approval_keyring.json --keyring scripts/ci/ai_quantization_profile_approval_keyring.json --keyring scripts/ci/ai_opcode_baseline_approval_keyring.json --warn-days 30 --fail-days 0 --out-json build/ai-keyring/ai_keyring_expiry_report.json
python3 scripts/ci/check_ai_keyring_kms_contract.py --keyring scripts/ci/ai_evidence_manifest_keyring.json --keyring scripts/ci/ai_model_provenance_keyring.json --keyring scripts/ci/ai_policy_ledger_keyring.json --keyring scripts/ci/ai_backend_selection_keyring.json --keyring scripts/ci/ai_governed_replay_keyring.json --keyring scripts/ci/ai_direct_backend_attestation_keyring.json --keyring scripts/ci/ai_benchmark_threshold_approval_keyring.json --keyring scripts/ci/ai_quantization_profile_approval_keyring.json --keyring scripts/ci/ai_opcode_baseline_approval_keyring.json --max-active-days-limit 120 --out-json build/ai-keyring/ai_keyring_kms_contract_report.json
python3 scripts/ci/check_ai_tloadhash_toolchain.py --t81-bin build/t81 --input-file tests/fixtures/llama_cpp_repro/model.gguf --out-dir build/ai-rfc0025
python3 scripts/ci/run_governed_llama_flow.py --t81-bin build-llama-local/t81 --model models/tinyllama-1.1b.Q2_K.gguf --out-dir build/ai-governed
python3 scripts/ci/check_ai_governed_replay_attestation.py --t81-bin build-llama-local/t81 --model tests/fixtures/llama_cpp_repro/model.gguf --out-dir build/ai-governed --seeds 0,1,2 --replays-per-seed 2 --baseline-governed-flow build/ai-governed/governed_llama_flow.json --signing-keyring scripts/ci/ai_governed_replay_keyring.json
python3 scripts/ci/t81lang_repro_gate.py --help
python3 scripts/ci/t3k_repro_gate.py --help
python3 scripts/ci/llama_cpp_repro_gate.py --help
bash scripts/ci/run_determinism_slice.sh build
```

## Keyring material sources

The AI signing keyring JSON files support two key material modes:

- `material_b64`: inline base64 key material (current repository baseline).
- `material_env`: environment variable name that supplies base64 key material at runtime.
- `kms_key_ref`: canonical KMS reference for the key material source (enforced by `check_ai_keyring_kms_contract.py`).

When both fields are present for an entry, `material_env` takes precedence when set; if it is
unset/empty, gates fall back to `material_b64` for compatibility.

Current AI CI secret variable names:

- `T81_AI_EVIDENCE_MANIFEST_KEY_2026Q1`
- `T81_AI_EVIDENCE_MANIFEST_KEY_2026Q2`
- `T81_AI_MODEL_PROVENANCE_KEY_2026Q1`
- `T81_AI_MODEL_PROVENANCE_KEY_2026Q2`
- `T81_AI_POLICY_LEDGER_KEY_2026Q1`
- `T81_AI_POLICY_LEDGER_KEY_2026Q2`
- `T81_AI_BACKEND_SELECTION_KEY_2026Q1`
- `T81_AI_BACKEND_SELECTION_KEY_2026Q2`
- `T81_AI_GOVERNED_REPLAY_KEY_2026Q1`
- `T81_AI_GOVERNED_REPLAY_KEY_2026Q2`
- `T81_AI_DIRECT_BACKEND_KEY_2026Q1`
- `T81_AI_DIRECT_BACKEND_KEY_2026Q2`
- `T81_AI_BENCHMARK_APPROVAL_KEY_2026Q1`
- `T81_AI_BENCHMARK_APPROVAL_KEY_2026Q2`
- `T81_AI_QUANTIZATION_APPROVAL_KEY_2026Q1`
- `T81_AI_QUANTIZATION_APPROVAL_KEY_2026Q2`
- `T81_AI_OPCODE_BASELINE_APPROVAL_KEY_2026Q1`
- `T81_AI_OPCODE_BASELINE_APPROVAL_KEY_2026Q2`

## Governed llama runtime evidence lane (RFC-0025 required)

The llama.cpp governed runtime lane is required in AI Experiments CI and uses the sanctioned fixture set:

- Fixture directory: `tests/fixtures/llama_cpp_repro/`
- Expected files:
  - `model.gguf`
  - `model_hash.txt` (`sha3-512:<hex>`)
  - `policy.apl`
  - `prompt.txt`

Helper:

```bash
python3 scripts/ci/llama_model_hash.py tests/fixtures/llama_cpp_repro/model.gguf \
  --t81-bin build-llama/t81 \
  --out tests/fixtures/llama_cpp_repro/model_hash.txt
```
