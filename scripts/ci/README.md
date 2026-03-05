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
- AI evidence bundle collector: `collect_ai_evidence_bundle.py`
- AI model provenance hash gate: `check_ai_model_provenance_gate.py`
- AI model provenance signing keyring: `ai_model_provenance_keyring.json`
- AI policy event contract gate: `check_ai_policy_event_contract.py`
- AI policy Axion ledger signing keyring: `ai_policy_ledger_keyring.json`
- AI backend adapter contract gate: `check_ai_backend_adapter_contract.py`
- AI backend selection manifest signing keyring: `ai_backend_selection_keyring.json`
- AI opcode subset contract gate: `check_ai_opcode_subset_contract.py`
- AI opcode runtime evidence report: `generate_ai_opcode_runtime_report.py`
- AI benchmark spec contract gate: `check_ai_benchmark_spec_contract.py`
- AI benchmark runtime threshold baseline: `ai_benchmark_thresholds.json`
- AI quantization codec contract gate: `check_ai_quantization_codec_contract.py`
- AI UX contract gate: `check_ai_ux_contract.py`
- AI cross-lane evidence lock gate: `check_ai_cross_lane_evidence.py`
- AI signed multi-lane evidence manifest gate: `check_ai_evidence_manifest.py`
- AI evidence manifest signing keyring: `ai_evidence_manifest_keyring.json`
- RFC-0025 canonize-tensor toolchain gate: `check_ai_tloadhash_toolchain.py`
- Governed llama runtime evidence runner: `run_governed_llama_flow.py`
- Governed llama multi-seed replay attestation gate: `check_ai_governed_replay_attestation.py`

## Local invocation examples
```bash
python3 scripts/ci/check_architecture_targets.py
python3 scripts/ci/check_vm_workload_benchmark_regression.py bench-vm-workload.json
python3 scripts/ci/check_cli_docs_parity.py --t81-bin build/t81 --manual docs/guides/cli-user-manual.md
python3 scripts/ci/check_cli_docs_smoke.py --manual docs/guides/cli-user-manual.md --cwd . --timeout-sec 20
python3 scripts/ci/check_cli_json_contracts.py --t81-bin build/t81 --repo-root .
python3 scripts/ci/check_ai_experiment_boundary.py
python3 scripts/ci/collect_ai_evidence_bundle.py --ai-bin build/experiments/ai/ux_tools/t81_ai --out-dir build/ai-evidence --runs 3 --model-fixture tests/fixtures/llama_cpp_repro/model.gguf
python3 scripts/ci/check_ai_model_provenance_gate.py --model build/ai-provenance/test_model.gguf --manifest build/ai-provenance/test_model.manifest.json --signing-keyring scripts/ci/ai_model_provenance_keyring.json --self-test-deny
python3 scripts/ci/check_ai_policy_event_contract.py --out-dir build/ai-policy --ledger-keyring scripts/ci/ai_policy_ledger_keyring.json
python3 scripts/ci/check_ai_backend_adapter_contract.py --out-dir build/ai-backend --ai-bin build/experiments/ai/ux_tools/t81_ai --runtime-model build/ai-backend/runtime_backend_probe_model.gguf --policy-contract build/ai-policy/ai_policy_event_contract.json --runtime-trace build/ai-policy/ai_runtime_trace.json --policy-ledger-snapshot build/ai-policy/ai_axion_policy_ledger_snapshot.json --selection-signing-keyring scripts/ci/ai_backend_selection_keyring.json
python3 scripts/ci/check_ai_opcode_subset_contract.py --out-dir build/ai-opcodes --runtime-report build/ai-opcodes-runtime/ai_opcode_runtime_report.json --ctest-log build/ai-opcodes/ai_phase1_opcode_ctest.log
python3 scripts/ci/generate_ai_opcode_runtime_report.py --repo-root . --out-dir build/ai-opcodes-runtime
python3 scripts/ci/check_ai_benchmark_spec_contract.py --out-dir build/ai-benchmark --ai-bin build/experiments/ai/ux_tools/t81_ai --runtime-model tests/fixtures/llama_cpp_repro/model.gguf --thresholds-file scripts/ci/ai_benchmark_thresholds.json
python3 scripts/ci/check_ai_quantization_codec_contract.py --out-dir build/ai-quantization --ai-bin build/experiments/ai/ux_tools/t81_ai --runtime-model tests/fixtures/llama_cpp_repro/model.gguf
python3 scripts/ci/check_ai_ux_contract.py --ai-bin build/experiments/ai/ux_tools/t81_ai --out-dir build/ai-ux --runtime-model tests/fixtures/llama_cpp_repro/model.gguf --t81-bin build-llama-ai/t81 --llama-hash-probe scripts/ci/llama_model_hash.py
python3 scripts/ci/check_ai_cross_lane_evidence.py --out-dir build/ai-cross-lane --evidence-bundle build/ai-evidence/ai_evidence_bundle.json --ux-contract build/ai-ux/ai_ux_contract.json --ux-inference build/ai-ux/ai_inference_run.json --ux-quantization build/ai-ux/ai_quantization_inspect.json --ux-benchmark build/ai-ux/ai_benchmark_run.json --tloadhash-toolchain build/ai-rfc0025/ai_tloadhash_toolchain.json
python3 scripts/ci/check_ai_evidence_manifest.py --out-dir build/ai-manifest --evidence-bundle build/ai-evidence/ai_evidence_bundle.json --vm-trace build/ai-vm-trace/ai_vm_trace_evidence.json --cross-lane build/ai-cross-lane/ai_cross_lane_evidence.json --backend-contract build/ai-backend/ai_backend_adapter_contract.json --ux-contract build/ai-ux/ai_ux_contract.json --tloadhash-toolchain build/ai-rfc0025/ai_tloadhash_toolchain.json --signing-keyring scripts/ci/ai_evidence_manifest_keyring.json --promotion-window-start 2026-03-01 --promotion-window-end 2026-03-31
python3 scripts/ci/check_ai_tloadhash_toolchain.py --t81-bin build/t81 --input-file tests/fixtures/llama_cpp_repro/model.gguf --out-dir build/ai-rfc0025
python3 scripts/ci/run_governed_llama_flow.py --t81-bin build-llama-local/t81 --model models/tinyllama-1.1b.Q2_K.gguf --out-dir build/ai-governed
python3 scripts/ci/check_ai_governed_replay_attestation.py --t81-bin build-llama-local/t81 --model tests/fixtures/llama_cpp_repro/model.gguf --out-dir build/ai-governed --seeds 0,1,2 --replays-per-seed 2 --baseline-governed-flow build/ai-governed/governed_llama_flow.json
python3 scripts/ci/t81lang_repro_gate.py --help
python3 scripts/ci/t3k_repro_gate.py --help
python3 scripts/ci/llama_cpp_repro_gate.py --help
bash scripts/ci/run_determinism_slice.sh build
```

## Keyring material sources

The AI signing keyring JSON files support two key material modes:

- `material_b64`: inline base64 key material (current repository baseline).
- `material_env`: environment variable name that supplies base64 key material at runtime.

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
