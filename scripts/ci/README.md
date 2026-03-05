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
- AI policy event contract gate: `check_ai_policy_event_contract.py`
- AI backend adapter contract gate: `check_ai_backend_adapter_contract.py`
- AI opcode subset contract gate: `check_ai_opcode_subset_contract.py`
- AI opcode runtime evidence report: `generate_ai_opcode_runtime_report.py`
- AI benchmark spec contract gate: `check_ai_benchmark_spec_contract.py`
- AI quantization codec contract gate: `check_ai_quantization_codec_contract.py`
- AI UX contract gate: `check_ai_ux_contract.py`
- AI cross-lane evidence lock gate: `check_ai_cross_lane_evidence.py`
- RFC-0025 canonize-tensor toolchain gate: `check_ai_tloadhash_toolchain.py`
- Governed llama runtime evidence runner: `run_governed_llama_flow.py`

## Local invocation examples
```bash
python3 scripts/ci/check_architecture_targets.py
python3 scripts/ci/check_vm_workload_benchmark_regression.py bench-vm-workload.json
python3 scripts/ci/check_cli_docs_parity.py --t81-bin build/t81 --manual docs/guides/cli-user-manual.md
python3 scripts/ci/check_cli_docs_smoke.py --manual docs/guides/cli-user-manual.md --cwd . --timeout-sec 20
python3 scripts/ci/check_cli_json_contracts.py --t81-bin build/t81 --repo-root .
python3 scripts/ci/check_ai_experiment_boundary.py
python3 scripts/ci/collect_ai_evidence_bundle.py --ai-bin build/experiments/ai/ux_tools/t81_ai --out-dir build/ai-evidence --runs 3 --model-fixture tests/fixtures/llama_cpp_repro/model.gguf
python3 scripts/ci/check_ai_model_provenance_gate.py --model build/ai-provenance/test_model.gguf --manifest build/ai-provenance/test_model.manifest.json --self-test-deny
python3 scripts/ci/check_ai_policy_event_contract.py --out-dir build/ai-policy
python3 scripts/ci/check_ai_backend_adapter_contract.py --out-dir build/ai-backend
python3 scripts/ci/check_ai_opcode_subset_contract.py --out-dir build/ai-opcodes --runtime-report build/ai-opcodes-runtime/ai_opcode_runtime_report.json --ctest-log build/ai-opcodes/ai_phase1_opcode_ctest.log
python3 scripts/ci/generate_ai_opcode_runtime_report.py --repo-root . --out-dir build/ai-opcodes-runtime
python3 scripts/ci/check_ai_benchmark_spec_contract.py --out-dir build/ai-benchmark
python3 scripts/ci/check_ai_quantization_codec_contract.py --out-dir build/ai-quantization
python3 scripts/ci/check_ai_ux_contract.py --ai-bin build/experiments/ai/ux_tools/t81_ai --out-dir build/ai-ux --runtime-model tests/fixtures/llama_cpp_repro/model.gguf
python3 scripts/ci/check_ai_cross_lane_evidence.py --out-dir build/ai-cross-lane --evidence-bundle build/ai-evidence/ai_evidence_bundle.json --ux-contract build/ai-ux/ai_ux_contract.json --ux-inference build/ai-ux/ai_inference_run.json --ux-quantization build/ai-ux/ai_quantization_inspect.json --ux-benchmark build/ai-ux/ai_benchmark_run.json --tloadhash-toolchain build/ai-rfc0025/ai_tloadhash_toolchain.json
python3 scripts/ci/check_ai_tloadhash_toolchain.py --t81-bin build/t81 --input-file tests/fixtures/llama_cpp_repro/model.gguf --out-dir build/ai-rfc0025
python3 scripts/ci/run_governed_llama_flow.py --t81-bin build-llama-local/t81 --model models/tinyllama-1.1b.Q2_K.gguf --out-dir build/ai-governed
python3 scripts/ci/t81lang_repro_gate.py --help
python3 scripts/ci/t3k_repro_gate.py --help
python3 scripts/ci/llama_cpp_repro_gate.py --help
bash scripts/ci/run_determinism_slice.sh build
```

## Optional llama.cpp repro gate

The llama.cpp repro gate is optional and fixture-gated:

- Fixture directory: `tests/fixtures/llama_cpp_repro/`
- Required files:
  - `model.gguf` (not committed)
  - `model_hash.txt` (not committed; expected `sha3-512:<hex>`)
  - `policy.apl`
  - `prompt.txt`
- CI enable switch:
  - repository variable `T81_ENABLE_LLAMA_REPRO=1`

Helper:

```bash
python3 scripts/ci/llama_model_hash.py tests/fixtures/llama_cpp_repro/model.gguf \
  --t81-bin build-llama/t81 \
  --out tests/fixtures/llama_cpp_repro/model_hash.txt
```
