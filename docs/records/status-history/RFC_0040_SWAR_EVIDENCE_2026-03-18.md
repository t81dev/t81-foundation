# RFC-0040 SWAR Evidence Snapshot

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [RFC-0040 SWAR Evidence Snapshot](#rfc-0040-swar-evidence-snapshot)
  - [Scope](#scope)
  - [Verification](#verification)

<!-- T81-TOC:END -->


Status: Active
Date: 2026-03-18
Owner: @t81dev

## Scope

Evidence refresh for RFC-0040 implementation closure after VM, Setun, JIT, and
CanonFS integration landed.

## Verification

Commands run locally on Darwin ARM64:

```sh
scripts/ci/run_rfc0040_swar_evidence.sh build
```

Observed results:

- `t81_vm_rfc0040_swar_test`: pass
- `jit_trace_equivalence_test`: pass, including SWAR JIT policy enforcement
- `BM_ComputeTAnd_Phase2C_SWAR/64`: `65.37 ns`
- `BM_ComputeTAnd_Phase2A/64`: `394.52 ns`
- `BM_ComputeTAnd_Phase2B_LUT/64`: `58.22 ns`
- `BM_ComputeTAnd_Phase2C_SWAR/256`: `68.94 ns`
- `BM_ComputeTAnd_Phase2A/256`: `966.69 ns`
- `BM_ComputeTAnd_Phase2B_LUT/256`: `96.38 ns`
- `BM_ComputeTOr_Phase2C_SWAR/64`: `54.42 ns`
- `BM_ComputeTOr_Phase2A/64`: `389.05 ns`
- `BM_ComputeTOr_Phase2B_LUT/64`: `57.49 ns`
- `BM_ComputeTOr_Phase2C_SWAR/256`: `61.22 ns`
- `BM_ComputeTOr_Phase2A/256`: `967.68 ns`
- `BM_ComputeTOr_Phase2B_LUT/256`: `93.43 ns`
- `BM_ComputeTNot_Phase2C_SWAR/64`: `52.09 ns`
- `BM_ComputeTNot_Phase2A/64`: `248.27 ns`
- `BM_ComputeTNot_Phase2B_LUT/64`: `55.84 ns`
- `BM_ComputeTNot_Phase2C_SWAR/256`: `57.42 ns`
- `BM_ComputeTNot_Phase2A/256`: `584.04 ns`
- `BM_ComputeTNot_Phase2B_LUT/256`: `81.29 ns`

Interpretation:

- RFC-0040 SWAR paths clearly outperform the Phase 2A reference path on the
  ARM64 host used for this evidence refresh.
- LUT remains competitive, and on this refreshed ARM64 run it is slightly ahead
  of SWAR for the smallest `64`-element cases and still ahead at `256`, but
  SWAR remains the promoted exact-trit VM/JIT path and remains materially ahead
  of the Phase 2A reference implementation.
- Cross-architecture bit-exact evidence still depends on running the existing
  same script on x86_64 alongside ARM64; this snapshot covers the local ARM64
  host only and leaves the x86_64 refresh as the remaining RFC-0040 evidence
  step.
