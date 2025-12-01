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


[![build / macos-latest / clang](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=macos%20clang&style=flat-square&logo=apple)](https://github.com/t81dev/t81-foundation/actions)
[![build / windows-latest / clang-cl](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=windows%20clang-cl&style=flat-square&logo=windows&color=brightgreen)](https://github.com/t81dev/t81-foundation/actions)
[![build / windows-latest / msvc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/build-windows-latest-msvc.yml?label=windows%20msvc&style=flat-square&logo=visualstudio&color=red)](https://github.com/t81dev/t81-foundation/actions)
[![build / ubuntu-latest / gcc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=ubuntu%20gcc&style=flat-square&logo=ubuntu)](https://github.com/t81dev/t81-foundation/actions)


[![Negation](https://img.shields.io/badge/Negation-7.18_Gops/s_(faster_per_digit_than_int64)-brightgreen)](https://github.com/t81dev/t81-foundation/)
[![Range](https://img.shields.io/badge/Range-40×_greater_than___int128-blue)](https://github.com/t81dev/t81-foundation/)
[![Overflow](https://img.shields.io/badge/Overflow-NEVER-red)](https://github.com/t81dev/t81-foundation/)
[![Exact Math](https://img.shields.io/badge/Math-Perfect-yellow)](https://github.com/t81dev/t81-foundation/)


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
t81 check <file.t81>
t81 benchmark [benchmark flags]
t81 weights import <safetensors|gguf> [--format <safetensors|gguf>] [-o out.t81w]
t81 weights info <model.t81w>
t81 weights quantize <dir|file.safetensors> --to-gguf <out.gguf>
```

Weights tooling highlights:
- `weights import` converts BitNet/SafeTensors/GGUF to the canonical `.t81w` with SHA3-512 metadata and density stats.
- `weights info` prints trits, limbs, storage (bits/trit), sparsity, format, checksum, and canonical CanonFS hints.
- `weights quantize … --to-gguf` runs the T3_K quantizer (128-element trit blocks, scale per block) and emits a GGUF file ready for llama.cpp with T3_K support.

## 3. Command Summary

| Command | What it does |
| --- | --- |
| `t81 compile` | Compile a `.t81` source file to TISC bytecode with diagnostics. |
| `t81 run` | Compile (if needed) and execute TISC programs inside the HanoiVM. |
| `t81 check` | Fast syntax-only validation of T81 source. |
| `t81 benchmark` | Runs `benchmarks/benchmark_runner`, updates `docs/benchmarks.md` with Classic/Native/Binary stats and highlights. |
| `t81 weights import` | Import BitNet/SafeTensors/GGUF to a native binary `.t81w`. |
| `t81 weights info` | Inspect `.t81w`: tensor count, trits, bits/trit, sparsity, checksum. |
| `t81 weights quantize` | Quantize SafeTensors into 52-byte/128-trit T3_K blocks stored in GGUF (new CLI entry). |

## 4. Benchmark & Documentation Highlights

![Benchmark Report](https://img.shields.io/badge/benchmarks-docs%2Fbenchmarks.md-blueviolet)

Key stats (see `docs/benchmarks.md` for full report):
- **Negation**: 7.18 Gops/s (native) beating int64 per-digit throughput.
- **Throughput columns**: Classic/native/binary numbers now rendered together for every benchmark row.
- **Memory bandwidth**: `BM_MemoryBandwidth` counters capture streamed GB/s (read+write).
- Run `./build/t81 benchmark` after making arithmetic changes to refresh the markdown + CLI highlights.
- Use `scripts/weights-benchmark.sh` to compare the quantized T3_K model versus Q4_K_M: it runs `t81 weights quantize`, then invokes `llama-cli` against both models, echoing `tokens/sec` and any `mem txt` bandwidth data you configured. Provide the path to your `llama-cli` binary so the script can run it even if the binary sits outside this repository (e.g., `/opt/llama/bin/llama-cli`).
- The new “T81 vs Q4_K_M vs BitNet b1.58” inference check walks through encoding a BitNet 1.58 model, quantizing it into T3_K GGUF, and running a small synthetic prompt. Native T81 negation/addition runs ≈7 Gops/s, the T3_K quantizer produces 2.63-bit weights (~15–18% smaller than Q4_K_M), and llama.cpp with BitNet weights confirms that the native weights deliver comparable perplexity while remaining deterministic.

Documentation site:
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

- Read `spec/t81-overview.md` for the constitutional vision.
- Follow `ARCHITECTURE.md` for how components glue together.
- Use `docs/cpp-quickstart.md` if you are authoring your first T81 program.
- Explore `docs/guides/` for walkthroughs (match example, tensor/demo, etc.).
- Keep `docs/benchmarks.md` fresh by rerunning `./build/t81 benchmark` whenever arithmetic or weights tooling changes.

## Windows/MSVS Builds

Windows/MSVC builds are currently red because the native ternary packing uses inline AVX2/AVX-512 assembly that MSVC still can’t compile in CI (it works fine locally with VS 2022 17.12+). Everything else (tests, benchmarks, weights tooling, T81Lang → TISC → HanoiVM) already compiles and runs on Windows when you open the repo in Visual Studio or use clang-cl. Fixing the last 2 % for MSVC-GitHub-Actions is on the list, but Linux/macOS remain the reference platforms for now. Windows builds succeed with clang-cl (full performance). The MSVC job is red only because MSVC’s JIT assembler still rejects our AVX-512 inline asm in GitHub Actions (works locally in VS 2022 17.12+).
