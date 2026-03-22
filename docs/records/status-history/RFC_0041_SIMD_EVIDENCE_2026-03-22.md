# RFC-0041 SIMD Evidence Note — 2026-03-22

**RFC:** RFC-0041 (SIMD Formalization)
**Status at time of snapshot:** accepted
**Host:** Apple Silicon AArch64 (ARM Neoverse-class, macOS Darwin 25.3.0)
**Build:** clang++ -std=c++20 -O3 (AppleClang 17+)
**Date:** 2026-03-22

---

## Summary

This note closes the three RFC-0041 pending items from the 2026-03-18 evidence snapshot:

1. **Deprecation wording** — completed: `t81/experimental/packed_trit_vector.hpp` now
   emits a `#pragma message` at direct-include sites; suppressed via
   `T81_PACKED_TRIT_VECTOR_STABLE_INCLUDE` guard from the stable header.
2. **RFC-0041 evidence note** — this document.
3. **x86_64 evidence refresh** — pending CI x86_64 runner; ARM64 evidence is current
   (see §Benchmark Results below).

---

## Deprecation Wording (closed 2026-03-22)

`include/t81/experimental/packed_trit_vector.hpp` now carries:

- File-level comment block documenting the migration targets (`t81/packed_trit_vector.hpp`,
  `t81/simd/simd.hpp`, `t81/swar/swar.hpp`) and the two-release-cycle removal timeline.
- `#pragma message` warning at direct-include sites (suppressed from `t81/packed_trit_vector.hpp`
  via the `T81_PACKED_TRIT_VECTOR_STABLE_INCLUDE` guard).

Migration guide: `docs/process/migration/RFC_0040_SWAR_MIGRATION.md`

---

## Benchmark Results — AArch64 (2026-03-22)

Benchmarks run from `benchmarks/benchmark_runner` with filter `BM_Compute|PackedTrit|SWAR`.
Input size: 1024 trits (128 bytes packed). All timings are real-time wall-clock (ns or µs).

### Trit-NOT

| Benchmark | Time |
| :--- | ---: |
| `BM_ComputeTNot_Phase2A` (ref scalar) | 242.81 µs |
| `BM_ComputeTNot_Phase2B_LUT` | 7.41 µs |
| `BM_ComputeTNot_Phase2C_SWAR` | 794.29 ns |
| `BM_ComputeTNot_Phase2D_AVX2` (NEON path) | 1.68 µs |
| `BM_ComputeTNot_Phase2D_InPlace` | 257.39 ns |

**Interpretation:** SWAR (`794 ns`) outperforms the NEON dispatch path (`1.68 µs`) on this
ARM64 host. The NEON path incurs dispatch overhead that exceeds kernel benefit at 1024 trits.
SWAR is the recommended default for TNot on Neoverse-class ARM64.

### Trit-AND

| Benchmark | Time |
| :--- | ---: |
| `BM_ComputeTAnd_Phase2A` (ref scalar) | 470.42 µs |
| `BM_ComputeTAnd_Phase2B_LUT` | 10.86 µs |
| `BM_ComputeTAnd_Phase2C_SWAR` | 1.09 µs |
| `BM_ComputeTAnd_Phase2D_AVX2` (NEON path) | 1.83 µs |
| `BM_ComputeTAnd_Phase2D_InPlace` | 1.43 µs |
| `BM_Kernel_TAnd_SWAR` (raw kernel) | 557.19 ns |

**Interpretation:** Raw SWAR kernel (`557 ns`) and SWAR dispatch (`1.09 µs`) both beat
the NEON path (`1.83 µs`). TAnd should use SWAR as default on this host. The InPlace
variant (`1.43 µs`) eliminates an allocation but does not beat SWAR dispatch at this size.

### Trit-OR

| Benchmark | Time |
| :--- | ---: |
| `BM_ComputeTOr_Phase2A` (ref scalar) | 460.93 µs |
| `BM_ComputeTOr_Phase2B_LUT` | 10.80 µs |
| `BM_ComputeTOr_Phase2C_SWAR` | 1.16 µs |
| `BM_ComputeTOr_Phase2D_AVX2` (NEON path) | 1.27 µs |
| `BM_ComputeTOr_Phase2D_InPlace` | 661.20 ns |

**Interpretation:** TOr NEON (`1.27 µs`) and SWAR (`1.16 µs`) are nearly equivalent.
InPlace SWAR (`661 ns`) is the fastest single-measurement path. The 2026-03-18 snapshot
identified TOr as the only clear NEON candidate; this snapshot confirms the margin is
narrow (≈8%) and size-dependent. Policy: TOr may use NEON when size ≥ threshold; SWAR
remains safe default.

### Trit-XOR

| Benchmark | Time |
| :--- | ---: |
| `BM_ComputeTXor_Phase2A` (ref scalar) | 474.86 µs |
| `BM_ComputeTXor_Phase2B_LUT` | 13.64 µs |
| `BM_ComputeTXor_Phase2C` | 11.45 µs |

**Interpretation:** XOR does not have a SWAR or NEON accelerated path yet; LUT is the
current fast path. No regression from 2026-03-18.

### Dispatch overhead

| Benchmark | Time |
| :--- | ---: |
| `BM_Overhead_DirectSWAR` | 1.33 ns |
| `BM_ComputeTritVector_ComputeOnly` | 1.42 µs |

**Interpretation:** Direct SWAR call overhead is `1.33 ns` — negligible. The 1.42 µs
`ComputeOnly` figure is the full 1024-trit AND-NOT chain with no output serialization.

---

## x86_64 Evidence Status

x86_64 evidence refresh is **pending CI runner**. The ARM64 numbers above are current.
The prior x86_64 snapshot (from 2026-03-18 ARM evidence doc, col 5) shows SWAR at
`68–80 ns` and AVX2 at `71–83 ns` for the 1024-trit workload. Refreshed x86_64 numbers
will be recorded in a follow-up snapshot once the CI x86_64 run completes.

---

## Compatibility / Deprecation Status

| Item | Status |
| :--- | :--- |
| `t81::PackedTritVector` (stable alias) | Active — `t81/packed_trit_vector.hpp` |
| `t81::ComputeTritVector` (stable alias) | Active — `t81/packed_trit_vector.hpp` |
| `t81::simd::ComputeTritVector` | Active — `t81/simd/simd.hpp` |
| `t81::experimental::PackedTritVector` (direct) | **Deprecated** — `#pragma message` warning emitted on direct include |
| `t81::experimental::ComputeTritVector` (direct) | **Deprecated** — `#pragma message` warning emitted on direct include |
| Removal timeline | 2 release cycles from 2026-03-22 |

---

## Cross-Reference

- Prior ARM64 snapshot: `RFC_0041_SIMD_EVIDENCE_2026-03-18.md`
- SWAR evidence: `RFC_0040_SWAR_EVIDENCE_2026-03-18.md`
- Migration guide: `docs/process/migration/RFC_0040_SWAR_MIGRATION.md`
