---
layout: page
title: Benchmarks
---

# T81 Foundation: Benchmarks

This document describes the benchmarks used to measure the performance and correctness of the T81 core libraries, comparing them against standard binary C++ types.

**Key Files:**
- **Runners:** `benchmarks/runner/*.cpp`
- **Main:** `benchmarks/runner/benchmark_runner.cpp`

______________________________________________________________________

## How to Run

The benchmark suite is built as a single executable.

```bash
# Build the benchmark runner
cmake --build build --target benchmark_runner

# Run the benchmarks and print a report to the console
./build/benchmark_runner
```

______________________________________________________________________

## Known Issues

**`std::overflow_error` Termination:** The `BM_overflow_ternary_auto` benchmark is *designed* to throw a `std::overflow_error` to test the `t81::core::Cell`'s built-in safety features. Currently, this exception is unhandled by the benchmark runner, which causes the entire suite to terminate prematurely.

**This is a known and prioritized issue.** The benchmark itself is correct, but the runner needs to be made more robust.

______________________________________________________________________

## Benchmark Suite

### Arithmetic Throughput

- **Benchmarks:** `BM_ArithThroughput_T81Cell`, `BM_ArithThroughput_Int64`
- **File:** [`arith_throughput.cpp`](../benchmarks/runner/arith_throughput.cpp)
- **Measures:** The speed of basic arithmetic operations (`+`, `-`, `*`, `/`).
- **Purpose:** To compare the raw computational throughput of the ternary `Cell` against a native `int64_t`.

### Negation Speed

- **Benchmarks:** `BM_NegationSpeed_T81Cell`, `BM_NegationSpeed_Int64`
- **File:** [`negation_speed.cpp`](../benchmarks/runner/negation_speed.cpp)
- **Measures:** The speed of the unary negation operator (`-x`).
- **Purpose:** To demonstrate a key theoretical advantage of balanced ternary, where negation is a single, fast operation (a trit-wise swap of `+` and `-`), compared to two's complement negation in binary.

### Overflow Detection

- **Benchmarks:** `BM_overflow_ternary_auto`, `BM_overflow_binary_silent`, `BM_overflow_binary_checked`
- **File:** [`overflow_detection.cpp`](../benchmarks/runner/overflow_detection.cpp)
- **Measures:** The performance cost of detecting integer overflows.
- **Purpose:** To quantify the overhead of the `Cell`'s built-in, automatic overflow detection (via exceptions) against both unsafe, silent binary overflow and safe, manual binary overflow checking.

### Packing Density

- **Benchmarks:** `BM_PackingDensity_Theoretical`, `BM_PackingDensity_Achieved`, `BM_PackingDensity_Practical`
- **File:** [`packing_density.cpp`](../benchmarks/runner/packing_density.cpp)
- **Measures:** The information density of the ternary representation.
- **Purpose:** These are conceptual benchmarks that do not measure speed. They illustrate the theoretical information content of a trit (~1.585 bits), the achieved density in the C++ `Cell` implementation, and the practical size advantage of a `Cell` compared to an `int16_t` of equivalent range.

### Roundtrip Accuracy

- **Benchmark:** `BM_RoundtripAccuracy_T81Cell`
- **File:** [`roundtrip_accuracy.cpp`](../benchmarks/runner/roundtrip_accuracy.cpp)
- **Measures:** The correctness of converting an integer to a `Cell` and back.
- **Purpose:** To verify that the `from_int` and `to_int` conversions are lossless within the valid range of a `Cell`, which is a fundamental correctness guarantee of the system.
