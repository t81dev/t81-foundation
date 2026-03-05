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

## Local invocation examples
```bash
python3 scripts/ci/check_architecture_targets.py
python3 scripts/ci/check_vm_workload_benchmark_regression.py bench-vm-workload.json
python3 scripts/ci/check_cli_docs_parity.py --t81-bin build/t81 --manual docs/guides/cli-user-manual.md
python3 scripts/ci/check_cli_docs_smoke.py --manual docs/guides/cli-user-manual.md --cwd . --timeout-sec 20
python3 scripts/ci/check_cli_json_contracts.py --t81-bin build/t81 --repo-root .
python3 scripts/ci/check_ai_experiment_boundary.py
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
