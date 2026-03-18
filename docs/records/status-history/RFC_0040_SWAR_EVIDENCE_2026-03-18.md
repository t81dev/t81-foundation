# RFC-0040 SWAR Evidence Snapshot

Status: Active
Date: 2026-03-18
Owner: @t81dev

## Scope

Evidence refresh for RFC-0040 implementation closure after VM, Setun, JIT, and
CanonFS integration landed.

## Verification

Commands run locally on Darwin ARM64:

```sh
cmake --build build --target t81_vm_rfc0040_swar_test jit_trace_equivalence_test benchmark_runner -j4
./build/t81_vm_rfc0040_swar_test
./build/jit_trace_equivalence_test
./build/benchmarks/benchmark_runner \
  --benchmark_filter='BM_(ComputeTAnd_Phase2A|ComputeTOr_Phase2A|ComputeTNot_Phase2A|ComputeTAnd_Phase2B_LUT|ComputeTOr_Phase2B_LUT|ComputeTNot_Phase2B_LUT|ComputeTAnd_Phase2C_SWAR|ComputeTOr_Phase2C_SWAR|ComputeTNot_Phase2C_SWAR)/(64|256)' \
  --benchmark_min_time=0.01s \
  --benchmark_format=json \
  --benchmark_out=build/rfc0040_swar_bench.json
```

Observed results:

- `t81_vm_rfc0040_swar_test`: pass
- `jit_trace_equivalence_test`: pass, including SWAR JIT policy enforcement
- `BM_ComputeTAnd_Phase2C_SWAR/64`: `63.57 ns`
- `BM_ComputeTAnd_Phase2A/64`: `460.03 ns`
- `BM_ComputeTAnd_Phase2C_SWAR/256`: `69.98 ns`
- `BM_ComputeTAnd_Phase2A/256`: `1128.70 ns`
- `BM_ComputeTOr_Phase2C_SWAR/64`: `62.69 ns`
- `BM_ComputeTOr_Phase2A/64`: `902.17 ns`
- `BM_ComputeTOr_Phase2C_SWAR/256`: `77.85 ns`
- `BM_ComputeTOr_Phase2A/256`: `1093.64 ns`
- `BM_ComputeTNot_Phase2C_SWAR/64`: `58.41 ns`
- `BM_ComputeTNot_Phase2A/64`: `249.58 ns`
- `BM_ComputeTNot_Phase2C_SWAR/256`: `67.20 ns`
- `BM_ComputeTNot_Phase2A/256`: `591.77 ns`

Interpretation:

- RFC-0040 SWAR paths clearly outperform the Phase 2A reference path on the
  ARM64 host used for this evidence refresh.
- LUT remains competitive for some very small cases, but SWAR remains the
  intended promoted exact-trit path and scales materially better than the
  reference implementation.
- Cross-architecture bit-exact evidence still depends on running the existing
  CI matrix on x86_64 alongside ARM64; this snapshot only covers the local
  ARM64 host.
