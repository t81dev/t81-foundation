# T81 Foundation: The Ternary-Native Computing Stack

<div align="center">
  <br/>
  <img src="docs/assets/img/banner.png" alt="T81 Foundation" width="100%"/>
  <br/><br/>

[![Paradigm: Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Computing-red?style=flat-square)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Design: Specification-First](https://img.shields.io/badge/Design-Specification%20First-blue?style=flat-square)](#)
[![CI Status](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml/badge.svg)](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml)
[![Core: C++20](https://img.shields.io/badge/Core-C%2B%2B20-0d1117?style=flat-square&logo=cplusplus)](#)
[![License: MIT/GPL-3.0](https://img.shields.io/badge/License-MIT%20%2F%20GPL--3.0-green?style=flat-square)](LICENSE-MIT)

*   *   *

[![build / macos-latest / clang](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=macos%20clang&style=flat-square&logo=apple)](https://github.com/t81dev/t81-foundation/actions)
[![build / windows-latest / clang-cl](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=windows%20clang-cl&style=flat-square&logo=windows&color=brightgreen)](https://github.com/t81dev/t81-foundation/actions)
[![build / windows-latest / msvc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=windows%20msvc&style=flat-square&logo=visualstudio&color=brightgreen)](https://github.com/t81dev/t81-foundation/actions)
[![build / ubuntu-latest / gcc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=ubuntu%20gcc&style=flat-square&logo=ubuntu)](https://github.com/t81dev/t81-foundation/actions)

*   *   *

[![Negation](https://img.shields.io/badge/Negation-7.18_Gops/s_(native)_vs_2.98_Gops/s_(classic)-brightgreen)](https://github.com/t81dev/t81-foundation/)
[![Range](https://img.shields.io/badge/Range-40×_greater_than___int128-blue)](https://github.com/t81dev/t81-foundation/)
[![Overflow](https://img.shields.io/badge/Overflow-NEVER-red)](https://github.com/t81dev/t81-foundation/)
[![Exact Math](https://img.shields.io/badge/Math-Perfect-yellow)](https://github.com/t81dev/t81-foundation/)

*   *   *

<div align="center">

[![English](https://img.shields.io/badge/Language-English-blue?style=flat-square)](/README.md)
[![简体中文](https://img.shields.io/badge/Language-%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87-red?style=flat-square)](/README.zh-CN.md)
[![Español](https://img.shields.io/badge/Language-Español-green?style=flat-square)](/README.es.md)
[![Русский](https://img.shields.io/badge/Language-Русский-brightgreen?style=flat-square)](/README.ru.md)
[![Português](https://img.shields.io/badge/Language-Português%20(Brasil)-blueviolet?style=flat-square)](/README.pt-BR.md)

*   *   *

</div>
<br>
<br/><br/>

</div>

## 1. Elevator Pitch

T81 is a sovereign, deterministic stack built around balanced ternary (−1, 0, +1). Everything from the core arithmetic types to the compiler, virtual machine, tensor library, and benchmarking toolchain is designed to demonstrate that ternary math can be exact, auditable, and performant when paired with modern C++ and SIMD hardware.

Core features:
- **Balanced ternary primitives**: `T81Int`, `T81Fraction`, `T81Float`, `T81Tensor` and friends implement exact arithmetic with zero hidden carries, round-trip safety, and Axion-friendly traps.
- **T81Lang compiler + TISC VM**: parse T81 code, emit TISC bytecode, and execute deterministically inside the HanoiVM.
- **Native + Classic benchmarking**: compare tryte-based (classic) vs AVX2-friendly (native) representations, reporting Classic/Native/Binary columns and latency/bandwidth metrics.
- **Weights tooling**: import SafeTensors/GGUF to `t81w`, inspect metadata, and quantize tensors into T3_K GGUF models (with new CLI `weights quantize`).

The stack is currently a late‑alpha / early‑beta collection of high-confidence numerics (well-tested core libs) wrapped around an experimental but usable compiler/VM pipeline.

## 2. Quick Start

### Build & Test

```bash
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### CLI Cheatsheet

```text
t81 compile <file.t81> [-o <file.tisc>]
t81 run <file.t81|.tisc>
t81 check <file.t81> (syntax + semantic validation)
t81 benchmark [benchmark flags]
t81 weights import <safetensors|gguf> [--format <safetensors|gguf>] [-o out.t81w]
t81 weights info <model.t81w>
t81 weights quantize <dir|file.safetensors> --to-gguf <out.gguf>
```

> Diagnostics: semantic and parsing errors now print the source file, line, and column.
> Match expressions now also emit canonical metadata (variant roots, guard presence, payload handles) via the CLI and Axion trace log so tooling can audit guards/payloads alongside loop hints.

Weights tooling highlights:
- `weights import` converts BitNet/SafeTensors/GGUF to the canonical `.t81w` with SHA3-512 metadata and density stats.
- `weights info` prints trits, limbs, storage (bits/trit), sparsity, format, checksum, and canonical CanonFS hints.
- `weights quantize … --to-gguf` runs the T3_K quantizer (128-element trit blocks, scale per block) and emits a GGUF file ready for llama.cpp with T3_K support.

## 3. Command Summary

| Command | What it does |
| --- | --- |
| `t81 compile` | Compile a `.t81` source file to TISC bytecode; semantic errors now include `file:line:column`. |
| `t81 run` | Compile (if needed) and execute TISC programs inside the HanoiVM. |
| `t81 check` | Fast parse + semantic validation of T81 source. |
| `t81 benchmark` | Runs `benchmarks/benchmark_runner`, updates `docs/benchmarks.md` with Classic/Native/Binary stats and highlights. |
| `t81 weights import` | Import BitNet/SafeTensors/GGUF to a native binary `.t81w`. |
| `t81 weights info` | Inspect `.t81w`: tensor count, trits, bits/trit, sparsity, checksum. |
| `t81 weights quantize` | Quantize SafeTensors into 52-byte/128-trit T3_K blocks stored in GGUF (new CLI entry). |

## 4. Benchmark & Documentation Highlights

[![Benchmarks](https://img.shields.io/badge/benchmarks-view_report-blueviolet?style=flat-square&logo=github)](https://github.com/t81dev/t81-foundation/blob/main/benchmarks/benchmark.md?plain=1)

Key stats (see `docs/benchmarks.md` for full report):
- **Negation**: native `BM_NegationSpeed` reaches 7.18 Gops/s (classic 2.98, binary 8.26 → 0.87× ratio) thanks to the single‑shuffle implementation.
- **Native arithmetic**: `BM_LimbAdd` reports 4.26 Mops/s on the SIMD path, while `BM_LimbArithThroughput` still shows 13.06 Mops/s for the classic tryte Kogge-Stone vs 376.94 Mops/s for binary (`__int128`) — the markdown table now surfaces Classic/Native/Binary columns and latency counters.
- **Memory bandwidth**: `BM_MemoryBandwidth` records 5.40 GB/s of read/write streaming, and `BM_PackingDensity` asserts ~1.58 bits/trit theoretical density.
- Run `./build/t81 benchmark` after arithmetic or weight changes to regenerate `docs/benchmarks.md`, refresh the Classic/Native/Binary rows, and keep the README badges accurate.
- The “T81 vs Q4_K_M vs BitNet b1.58” showcase now demonstrates the end-to-end workflow: convert SafeTensors/GGUF → `.t81w`, quantize to T3_K, and compare `llama-cli` throughput (via `scripts/weights-benchmark.sh`) between the native T3_K file and a Q4_K_M baseline.
- **Weight integration**: `t81 weights load` powers the new T81Lang `weights.load("<path>")` builtin, so HanoiVM code can hydrate `.t81w` models, inspect `WeightsModel` handles, and pass tensors through the same arithmetic pipeline that powers `t81 weights info`. See `docs/benchmarks.md` for the Classic/Native/Binary tables plus CLI workflow references.

- `docs/onboarding.md` – curated flow for new C++ developers (clone/build/first bug) before diving into the rest.
- `docs/guides/weights-integration.md` – walkthrough for the new `weights.load("<tensor>")` builtin, the `.t81w` CLI path, and links to the `examples/weights_load_demo.t81` sample that shows handle reuse.
- `docs/release.md` – release/versioning policy and checklist for maintainers who ship tags.
- `docs/ci.md` – explains how to reproduce CI/test suites locally and what GitHub Actions run.
- `docs/benchmarks.md` – auto-generated benchmark table + analysis.
- `docs/assets/...` – brand assets used in this README.
- `docs/system-status.md` and `docs/guides/` for narrative walkthroughs.
- Build/site preview with `cd docs && bundle exec jekyll serve`.

## 5. Repository Layout

| Path | Description |
| --- | --- |
| `/spec/` | Immutable project constitution (normative specs, RFCs). |
| `/include/t81/` | Public C++ headers (`t81::v1`) including the new weights/crypto helpers. |
| `/src/` | Implementation: compiler, VM, weights tooling, benchmarks, crypto helpers. |
| `/tests/` | Unit + regression tests (do not remove coverage). |
| `/benchmarks/` | Google Benchmark tables + the report runner. |
| `/docs/` | Jekyll guides & generated benchmark report (`docs/benchmarks.md`). |
| `/examples/` | T81 sample programs & demos. |

## 6. Next Steps

- Read `spec/index.md` (and linked specs such as `spec/t81lang-spec.md`, `spec/tisc-spec.md`, `spec/t81vm-spec.md`, `spec/t81-data-types.md`, `spec/axion-kernel.md`, `spec/canonfs-spec.md`) for the constitutional and interface-level definitions.
- Follow `ARCHITECTURE.md` for how components glue together and consult `TASKS.md` + `ROADMAP.md` for current priorities.
- Use `docs/onboarding.md` → `docs/cpp-quickstart.md` for the full newcomer flow, then dive into `docs/guides/` for targeted walkthroughs.
- Reference `docs/release.md` before bumping versions and creating tags; keep `CHANGELOG.md` and spec RFCs (see `spec/rfcs/template.md`) in sync before merging.
- Keep `docs/benchmarks.md` fresh by rerunning `./build/t81 benchmark` whenever arithmetic or weights tooling changes.
