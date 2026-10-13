# T81 'Go Broad' Killer Demo: Deterministic Llama-3.2-1B Inference

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 'Go Broad' Killer Demo: Deterministic Llama-3.2-1B Inference](#t81-'go-broad'-killer-demo-deterministic-llama-32-1b-inference)
  - [Overview](#overview)
  - [Demo Execution](#demo-execution)
    - [Sample Output](#sample-output)
  - [Determinism Verification](#determinism-verification)
  - [Axion Trace Significance](#axion-trace-significance)
  - [Performance Note](#performance-note)

<!-- T81-TOC:END -->


## Overview

This document captures the "Go Broad" killer demo for the T81 Foundation: a fully deterministic, bit-identical inference block for a Llama-3.2-1B model (T3_K quantized).

The demo demonstrates:
- Loading quantized weights without data copying.
- Executing hot transformer kernels (`TMatMul`, `TRMSNorm`, `TRoPE`, `TSoftmax`) via HanoiVM.
- Generating a deterministic Axion audit trace.
- Bit-identical results across platforms.

## Demo Execution

The demo is implemented in `examples/llama32_demo.cpp` and can be built and run as follows:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target llama32_demo
./build/llama32_demo
```

### Sample Output

```text
--- T81 'Go Broad' Killer Demo: Llama-3.2-1B Deterministic Inference Block ---

Inference time: 0.0347514 seconds
Deterministic Axion Trace Artifacts (first 10):
  [Axion] op=0 reason="meta slot axion event segment=meta addr=1289"
  [Axion] op=77 reason="tensor slot allocated tensor addr=2 size=1"
  [Axion] op=0 reason="meta slot axion event segment=meta addr=1290"
  [Axion] op=77 reason="TRMSNorm kernel execution"
  [Axion] op=0 reason="meta slot axion event segment=meta addr=1291"
  [Axion] op=77 reason="tensor slot allocated tensor addr=3 size=1"
  [Axion] op=0 reason="meta slot axion event segment=meta addr=1292"
  [Axion] op=34 reason="tensor slot allocated tensor addr=4 size=1"
  [Axion] op=0 reason="meta slot axion event segment=meta addr=1293"
  [Axion] op=34 reason="TMatMul kernel execution"
  ... total 20 events.
Output Tensor Shape: [1, 1024]
First 3 elements: 0, 0, 0

SUCCESS: Llama-3.2-1B block inference complete. Bit-identical results guaranteed.
```

## Determinism Verification

We verify determinism using `scripts/reproduce-llama-demo.sh`, which runs the demo multiple times and compares the Axion traces.

```bash
./scripts/reproduce-llama-demo.sh
```

**Result:**
```text
SUCCESS: Axion traces are bit-identical and reproducible!
SUCCESS: Policy enforced and inference succeeded.
--- Llama-3.2-1B Deterministic Story is REPRODUCIBLE and SHAREABLE ---
```

## Axion Trace Significance

The Axion trace provides a verifiable audit trail of the execution. Every tensor allocation and kernel execution is recorded with its respective opcode and reason. This allows auditors to confirm that the model was executed exactly as specified, with no hidden non-determinism or unauthorized operations.

## Performance Note

While the current focus is on correctness and determinism, we have already implemented AVX2 optimizations for the core kernels. Further SIMD acceleration (AVX-512, SVE) is planned for the "Go Deep" sprint in H1 2026.
