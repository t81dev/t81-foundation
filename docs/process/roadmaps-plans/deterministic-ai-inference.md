# Roadmap: Deterministic AI Inference at Scale

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Roadmap: Deterministic AI Inference at Scale](#roadmap-deterministic-ai-inference-at-scale)
  - [Executive Summary](#executive-summary)
  - [1. Problem Statement](#1-problem-statement)
  - [2. The T81 Solution Stack](#2-the-t81-solution-stack)
    - [2.1 Core Numerics](#21-core-numerics)
    - [2.2 Structural & Symbolic Types](#22-structural-&-symbolic-types)
    - [2.3 System & Utility Types](#23-system-&-utility-types)
  - [3. Implementation Plan](#3-implementation-plan)
    - [Phase 1: Closing the "Libm Gap" (The Math Kernel)](#phase-1-closing-the-"libm-gap"-the-math-kernel)
    - [Phase 2: Deterministic Tensor Operations](#phase-2-deterministic-tensor-operations)
    - [Phase 3: Model Architecture Porting](#phase-3-model-architecture-porting)
    - [Phase 4: Validation at Scale](#phase-4-validation-at-scale)
  - [4. Risks & Mitigations](#4-risks-&-mitigations)
  - [5. Success Metrics](#5-success-metrics)

<!-- T81-TOC:END -->


## Executive Summary

This roadmap outlines the strategic plan to enable **guaranteed reproducible inference** for Large Language Models (LLMs) and other deep learning architectures using the T81 platform. Current AI systems suffer from non-determinism due to floating-point drift across hardware architectures (the "Libm Gap") and non-associative parallel reductions. By leveraging the bit-exact properties of T81's balanced ternary datatypes, we aim to ensure that `Prompt + Seed + Model = Identical Token Stream`, universally.

## 1. Problem Statement

*   **Floating Point Drift**: IEEE 754 floating-point operations can yield bit-level differences on different hardware (x86 vs ARM vs GPU) and even different compiler flags, especially for transcendental functions (`sin`, `exp`, `log`).
*   **Parallel Non-Determinism**: Parallel floating-point addition is non-associative `(a+b)+c != a+(b+c)`. Dynamic scheduling in GPUs makes reduction results order-dependent.
*   **The "Libm Gap"**: T81 currently relies on host `std::sin`, `std::exp`, etc., for `T81Float`, inheriting host non-determinism.

## 2. The T81 Solution Stack

We will utilize the full spectrum of T81 datatypes to construct a strictly deterministic inference pipeline.

### 2.1 Core Numerics
*   **`T81Float<72, 9>`**: The primary datatype for model **weights** and **activations**. Provides high precision and dynamic range. Operations must be decoupled from host FPU.
*   **`T81Fixed<192, 80>` (`dmath`)**: The backbone of our deterministic math kernel. Used for intermediate calculations in transcendental functions to ensure bit-exact results regardless of the host.
*   **`T81Int`**:
    *   **Token IDs**: Representing vocabulary indices.
    *   **Embedding Tables**: Efficient lookups.
    *   **Control Flow**: Loop counters and architectural parameters.
*   **`T81Complex`**: Essential for **Rotary Positional Embeddings (RoPE)**, a standard in modern LLMs (e.g., Llama). Will require a deterministic backend synchronized with `dmath`.
*   **`T81Quaternion`**: Used for 3D spatial reasoning in embodied AI agents and potential 4D positional encodings.
*   **`T81Prob`**: For final **Logits** and **Sampling**. Deterministic sampling requires a stable probability space and a content-addressable entropy source (Axion RNG).
*   **`T81Fraction`**: Exact representation of scaling factors (e.g., attention head scaling $1/\sqrt{d_k}$) to avoid initial precision loss during model setup.
*   **`T81BigInt`**: Handling large accumulated sums in reductions or precise cryptographic operations within the model inference (e.g., secure aggregation).
*   **`T81Polynomial`**: For implementing deterministic spline activations or specialized layer approximations.

### 2.2 Structural & Symbolic Types
*   **`T81Tensor`**: The primary N-dimensional container. Enforces memory alignment and strict evaluation order for reductions.
*   **`T729Tensor`**: Specialized, highly optimized tensor storage for packed ternary operations (9 trits per tryte), offering higher density for quantized weights.
*   **`DistributedTensor`**: Enables deterministic model parallelism (sharding) across multiple nodes. The distributed protocol must guarantee message ordering to preserve determinism.
*   **`T81Matrix`**: Specialized 2D operations for smaller, dense layers or specific linear algebra subroutines where full tensor overhead is unnecessary.
*   **`T81Graph` / `T81Tree` / `T81Network`**: Representing the model architecture itself (computation graph) and potentially structured state space models (SSMs) or graph neural networks (GNNs).
*   **`T81Symbol` / `T81Symbolic`**: For symbolic execution and optimizing the computation graph (e.g., fusing operations) *before* execution, ensuring the optimized graph is identical across platforms.
*   **`T81Vector` / `T81List` / `T81Map` / `T81Set`**: Managing dynamic inference state, beam search candidates, and key-value caches with deterministic iteration order.

### 2.3 System & Utility Types
*   **`T81Entropy`**: The source of deterministic randomness. Seeds must be injected and managed explicitly.
*   **`T81Time`**: Handling timeouts and logical clocks in a deterministic manner (e.g., step counters instead of wall-clock time).
*   **`T81Thread`**: Deterministic threading models (e.g., cooperative multitasking or strictly ordered thread pools) to prevent race conditions from affecting the output.
*   **`T81IOStream` / `T81Stream`**: Reproducible serialization of model states and token streams.
*   **`T81Result` / `T81Maybe` / `T81Promise`**: Deterministic error handling and asynchronous control flow. Errors must be part of the deterministic state machine.
*   **`T81Agent`**: The higher-level abstraction for the AI entity itself, encapsulating the model state, memory, and policy.

## 3. Implementation Plan

### Phase 1: Closing the "Libm Gap" (The Math Kernel)
**Goal**: Remove all dependencies on host `libm` (`<cmath>`) for core inference operations.
*   **Task 1.1**: Complete the `dmath` library (pure integer/ternary implementation of `exp`, `log`, `sin`, `cos`, `tanh`, `sqrt`).
*   **Task 1.2**: Wire `T81Float` transcendental methods to use `dmath` exclusively in "Strict Mode".
*   **Task 1.3**: Implement `T81Complex` and `T81Quaternion` operations using `dmath` for advanced positional embeddings.

### Phase 2: Deterministic Tensor Operations
**Goal**: Ensure `T81Tensor` and `T729Tensor` operations are robust and order-independent.
*   **Task 2.1**: Implement deterministic parallel reductions (e.g., tree summation) for `reduce_sum`, `matmul`.
*   **Task 2.2**: Optimize `matmul` for `T81Float` and `T729Tensor` using integer-based accumulation where possible.
*   **Task 2.3**: Verify `softmax`, `gelu`, `layer_norm`, and `rms_norm` implementations against the `dmath` backend.
*   **Task 2.4**: Implement `DistributedTensor` logic ensuring deterministic sharding and recombination.

### Phase 3: Model Architecture Porting
**Goal**: Run a standard Transformer architecture fully within T81.
*   **Task 3.1**: Port a "NanoGPT" or small Llama-style model to use `T81Tensor` ops.
    *   *Layers*: Embedding -> RMSNorm -> Attention (with RoPE & KV Cache) -> FeedForward (SwiGLU/GELU) -> Output.
*   **Task 3.2**: Implement the **Sampler**.
    *   Input: `T81Tensor` (Logits).
    *   Process: `softmax` -> `T81Prob` -> `top_k` / `top_p`.
    *   Entropy: Use `T81Entropy` for deterministic seed injection.
*   **Task 3.3**: Utilize `T81Graph` to define and freeze the model structure.

### Phase 4: Validation at Scale
**Goal**: Prove reproducibility.
*   **Task 4.1**: Create the `Determinism Suite`. A test harness that runs the same inference job on:
    *   x86_64 (Linux/Windows)
    *   ARM64 (Apple Silicon/Linux)
    *   WASM (Browser)
*   **Task 4.2**: Bit-for-bit comparison of the output token stream. Any deviation is a critical bug.

## 4. Risks & Mitigations

| Risk | Mitigation |
| :--- | :--- |
| **Performance** | `dmath` (software float) is slower than hardware FPU. **Mitigation**: JIT compilation of T81 ops to optimized integer sequences; FPGA/ASIC offload plan (longer term). |
| **Precision** | Ternary floats behave differently than IEEE 754. **Mitigation**: Fine-tuning or post-training quantization calibration for T81 numerics. |
| **Memory** | `T81Tensor` objects can be large. **Mitigation**: Optimize `T81Tensor` allocator; utilize `T729Tensor` for compressed storage; implement "copy-on-write" or view semantics. |

## 5. Success Metrics

1.  **Zero Drift**: 100% bit-exact output for a generated story (100+ tokens) across 3 distinct hardware architectures.
2.  **Coverage**: Full support for Llama 3 / GPT-2 style architectures.
3.  **Efficiency**: Inference speed within 10x of unoptimized Python/PyTorch baseline (acceptable for "Strict Determinism" tier).
