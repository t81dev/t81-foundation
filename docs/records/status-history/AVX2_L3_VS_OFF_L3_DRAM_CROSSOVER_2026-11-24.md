# Native `x86_64` AVX2 Crossover Note: L3-Resident (`PR #863`) vs. Off-L3 `> 480 MiB` DRAM (`PR #864`)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Native `x86_64` AVX2 Crossover Note: L3-Resident (`PR #863`) vs. Off-L3 `> 480 MiB` DRAM (`PR #864`)](#native-`x86_64`-avx2-crossover-note-l3-resident-`pr-#863`-vs-off-l3-`>-480-mib`-dram-`pr-#864`)
  - [1. Host & Measurement Provenance](#1-host-&-measurement-provenance)
  - [2. L3-Resident (`L = 64`) vs. Off-L3 DRAM (`> 480 MiB`) Comparison](#2-l3-resident-`l-=-64`-vs-off-l3-dram-`>-480-mib`-comparison)
    - [2.1 In-L3 Regime (`L = 64`, `< 480 MiB`)](#21-in-l3-regime-`l-=-64`-`<-480-mib`)
    - [2.2 Off-L3 DRAM-Stressed Regime (`> 480 MiB` Working Set, Run `#45328018712`)](#22-off-l3-dram-stressed-regime-`>-480-mib`-working-set-run-`#45328018712`)
  - [3. Empirical Crossover Takeaways & Caveats](#3-empirical-crossover-takeaways-&-caveats)

<!-- T81-TOC:END -->


**Date:** 2026-11-24  
**Status:** Non-normative empirical measurement summary (single-thread `x86_64` software benchmark; does **not** establish general ternary hardware superiority; preserves all research holds `F01 M1–M6 STOP`, `SCG-08 Gate 2 NO-GO`, `H01`/10A `BLOCKED`, `AXION-AI-001 HD01–HD06`).

---

## 1. Host & Measurement Provenance

- **Host Hardware:** `Intel(R) Xeon(R) 6975P-C` (`x86_64`, 4 vCPUs on KVM `ubuntu-24.04`, 1 NUMA node, `15.3 GiB` RAM, `L1d 96 KiB`, `L2 4 MiB`, **`L3 480 MiB`**, `avx2` + `avx512_vpopcntdq`).
- **Build Flags:** `-O3 -march=native -DNDEBUG`, single-threaded `BM_DualRailMemoryBound.cpp` (`R1`: ternary activations × ternary weights, `4096×1024` per layer, exact `int64_t` oracle `max_abs_err == 0`).
- **Pinned Runs:**
  - **In-L3 Baseline (`PR #863`):** GitHub Actions Run [`45325242627`](https://github.com/t81dev/t81-foundation/actions/runs/45325242627) (`L = 64`, footprints `7.06–130.32 MB`, fully resident in `480 MiB` L3).
  - **Off-L3 `> 480 MiB` DRAM Sweep (`PR #864`):** GitHub Actions Run [`45328018712`](https://github.com/t81dev/t81-foundation/actions/runs/45328018712) (`headSha: 7f5199d53`, merged at `e51970a13`, `L ∈ {64, 640, 1024, 2048, 5120}` with RAM-guarded preset `dram_sweep_gt480mib`).

---

## 2. L3-Resident (`L = 64`) vs. Off-L3 DRAM (`> 480 MiB`) Comparison

### 2.1 In-L3 Regime (`L = 64`, `< 480 MiB`)

| Kernel | Sparsity | Run `#45325242627` (`BM_MemBound64`) | Run `#45328018712` (`DramSweep/64`) | Effective BW (`#45328018712`) |
| :--- | :---: | :---: | :---: | :---: |
| `DualRail_Dense` (`64 MB`) | 50% / 75% / 90% | `2.544 / 2.530 / 2.554 ms` | `2.954 / 3.072 / 3.686 ms` | `22.72 / 21.84 / 18.21 GB/s` |
| `SBDR128_SIMD` (`34.3 / 17.3 / 7.1 MB`) | 50% / 75% / 90% | `2.433 / 1.354 / 0.751 ms` | `2.629 / 1.366 / 0.751 ms` | `13.67 / 13.25 / 9.87 GB/s` |
| `INT8_Sparse128` (`130.3 / 65.3 / 26.3 MB`) | 50% / 75% / 90% | `13.477 / 2.805 / 1.262 ms` | `12.784 / 6.547 / 1.339 ms` | `10.69 / 10.46 / 20.58 GB/s` |

### 2.2 Off-L3 DRAM-Stressed Regime (`> 480 MiB` Working Set, Run `#45328018712`)

| Kernel | `L` | Sparsity | Raw `real_time` | Per-Layer (`µs/L`) | Effective BW |
| :--- | :---: | :---: | :---: | :---: | :---: |
| `DualRail_Dense` | `640` (`640 MB`) | 50% | `65.102 ms` (`2` iters) | `101.72 µs/L` | `10.31 GB/s` |
| `DualRail_Dense` | `640` (`640 MB`) | 75% | `65.594 ms` (`2` iters) | `102.49 µs/L` | `10.23 GB/s` |
| `DualRail_Dense` | `640` (`640 MB`) | 90% | `66.087 ms` (`2` iters) | `103.26 µs/L` | `10.15 GB/s` |
| `SBDR128_SIMD` | `1024` (`~548 MB`) | 50% | `76.644 ms` (`2` iters) | `74.85 µs/L` | `7.50 GB/s` |
| `SBDR128_SIMD` | `2048` (`~552 MB`) | 75% | `75.493 ms` (`2` iters) | `36.86 µs/L` | `7.67 GB/s` |
| `SBDR128_SIMD` | `5120` (`~565 MB`) | 90% | `87.109 ms` (`2` iters) | `17.01 µs/L` | `6.80 GB/s` |
| `INT8_Sparse128` | `1024` (`~2.08 GB`) | 50% | `197.604 ms` (`1` iter) | `192.97 µs/L` | `11.07 GB/s` |
| `INT8_Sparse128` | `2048` (`~2.09 GB`) | 75% | `215.776 ms` (`1` iter) | `105.36 µs/L` | `10.15 GB/s` |
| `INT8_Sparse128` | `5120` (`~2.10 GB`) | 90% | `250.417 ms` (`1` iter) | `48.91 µs/L` | `8.80 GB/s` |

---

## 3. Empirical Crossover Takeaways & Caveats

1. **Confirmed L3 → DRAM Cliff:** Scaling `DualRail_Dense` from `L = 64` (`64 MB`, in-L3) to `L = 640` (`640 MB`, off-L3) drops effective bandwidth from **`22.72 → 10.31 GB/s`** and increases per-layer latency from **`46.15 → 101.72 µs/L`** (`2.20×` DRAM cliff).
2. **Off-L3 `R1` Speedups (`µs/L`, `SBDR128_SIMD` vs. Controls):**
   - vs. **`INT8_Sparse128`**: **`2.58×`** (`50%`, `74.85` vs `192.97 µs/L`), **`2.86×`** (`75%`, `36.86` vs `105.36 µs/L`), **`2.88×`** (`90%`, `17.01` vs `48.91 µs/L`).
   - vs. **`DualRail_Dense`**: **`1.36×`** (`50%`, `74.85` vs `101.72 µs/L`), **`2.78×`** (`75%`, `36.86` vs `102.49 µs/L`), **`6.07×`** (`90%`, `17.01` vs `103.26 µs/L`).
3. **Scope & Methodological Limits:**
   - Results are **single-thread `R1` (ternary activations × ternary weights) only** on a shared KVM runner (`1–2` Google Benchmark iterations at large `L`, so run-to-run variance is non-zero).
   - `R2` (`int8` activations × ternary weights) remains governed by the preserved negative findings (`H06-MEM-03/05A/06`: use `INT4W`/`INT8W`, not CPU Dual-Rail unpack).
   - Software memory-bandwidth gains from 2-bit/block-sparse packing on binary `x86_64` CPUs do **not** constitute evidence of physical ternary silicon advantage (`H01`/10A remains `BLOCKED`).
