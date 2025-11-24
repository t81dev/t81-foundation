# T81 C++ Quickstart

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 C++ Quickstart](#t81-c++-quickstart)
  - [1) Include the umbrella header](#1-include-the-umbrella-header)
  - [2) Build options](#2-build-options)
    - [CMake (recommended)](#cmake-recommended)
    - [Bazel](#bazel)
    - [Make (shim)](#make-shim)
  - [3) Run examples](#3-run-examples)
  - [4) Notes & Caveats](#4-notes-&-caveats)
  - [5) Headers index (most common)](#5-headers-index-most-common)
  - [6) VM Notes](#6-vm-notes)

<!-- T81-TOC:END -->










This guide shows how to consume the new header-only C++ API and run examples/tests.

## 1) Include the umbrella header

```cpp
#include <t81/t81.hpp>        // BigInt, Fraction, Tensor, CanonFS, config, etc.
#include <t81/tensor/ops.hpp> // optional: transpose/slice/reshape ops
```

## 2) Build options

### CMake (recommended)

```bash
cmake -S . -B build -DT81_BUILD_EXAMPLES=ON -DT81_BUILD_TESTS=ON
cmake --build build --parallel
```

### Bazel

```bash
bazel build //:t81_demo
bazel test  //:t81_*_test
```

### Make (shim)

```bash
make           # builds demo + tests
make run-tests # runs all tests
```

## 3) Run examples

```bash
./build/t81_demo
./build/t81_tensor_ops
./build/t81_ir_roundtrip
./build/axion_demo
```

## 4) Notes & Caveats

- **Base-243 codec** is a deterministic stub (`include/t81/codec/base243.hpp`). It maps bytes/chars modulo 243 and is **not** a real base conversion. Swap in a canonical codec later without changing call sites.
- **Base-81 hashing** stubs live in `include/t81/hash/*` and are **non-cryptographic**. Replace with a real digest + Base-81 when ready.
- **Fractions** are signed, denominator-positive, and reduced by `gcd`. Division uses naive paths pending full BigInt division.
- **Tensor ops** are dependency-free and CPU-only; they’re meant for tests and small utilities.

## 5) Headers index (most common)

- `t81/t81.hpp` — umbrella
- `t81/bigint.hpp` — `T243BigInt` (signed, base-243)
- `t81/fraction.hpp` — `T81Fraction` (signed, reduced)
- `t81/tensor.hpp` — `T729Tensor`
- `t81/tensor/ops.hpp` — transpose/slice/reshape
- `t81/io/tensor_loader.hpp` — text IO helpers
- `t81/canonfs.hpp`, `t81/canonfs_io.hpp` — CanonFS types & wire IO
- `t81/ir/{opcodes,insn,encoding}.hpp` — minimal IR and 32B encoding
- `t81/hash/{base81,canonhash}.hpp` — Base-81 & CanonHash stubs
- `t81/codec/base243.hpp` — Base-243 codec surface (stub)
- `t81/axion/api.hpp` — tiny Axion façade (stub)
- `t81/vm/state.hpp` — VM state (registers, pools, trace helpers)

## 6) VM Notes

- Structural helpers (`MAKE_OPTION_*`, `MAKE_RESULT_*`) were added to the ISA.
  Make sure your VM build includes the new opcodes; otherwise you’ll see
  `IllegalInstruction` traps when compiling Option/Result-heavy code.
- Tensor handles are pooled like floats/fractions. Inspect `vm->state()` to
  understand what the compiler emitted.
- Axion policies (APL) are not required yet, but the VM header already reserves
  space—unknown bytes should be ignored deterministically.
