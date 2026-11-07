# Installing & Building T81 Foundation

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Installing & Building T81 Foundation](#installing-&-building-t81-foundation)
  - [Prerequisites](#prerequisites)
  - [Clone and Build](#clone-and-build)
- [Configure for Release with Ninja](#configure-for-release-with-ninja)
- [Build all targets in parallel](#build-all-targets-in-parallel)
  - [Verify Installation](#verify-installation)
- [Run the core C++ test suite](#run-the-core-c++-test-suite)
    - [Reproducibility Gate](#reproducibility-gate)
  - [Running Examples](#running-examples)
    - [Compiling T81Lang Code](#compiling-t81lang-code)
  - [Running Benchmarks](#running-benchmarks)
- [Build the benchmark targets](#build-the-benchmark-targets)
- [Default smoke profile; outputs to JSON](#default-smoke-profile-outputs-to-json)
- [Human-usable full profile:](#human-usable-full-profile)
- [Exhaustive research/nightly profile:](#exhaustive-researchnightly-profile)
- [Custom filtered local iteration using the CLI wrapper:](#custom-filtered-local-iteration-using-the-cli-wrapper)
  - [Including T81 in Your CMake Project](#including-t81-in-your-cmake-project)

<!-- T81-TOC:END -->


T81 Foundation is a vertically integrated, ternary-native ecosystem. It is distributed as C++ source code and compiled via CMake. It currently supports Linux (GCC/Clang), macOS (Apple Clang), and Windows Server (MSVC, best-effort).

## Prerequisites
- **CMake** 3.16+
- **C++ Compiler** with C++23 support (C++20 supported via `-DT81_USE_CXX23=OFF`)
  - *Verified locally with GCC 14, Clang 18, and Apple Clang.*
- **Python** 3.10+ (Required for reproducibility gates and governance checks)
- **Ninja** or **Make** (Ninja recommended)

## Clone and Build
Because T81 uses a deterministic build process and vendored submodules for critical dependencies, start by cloning the repository:

```bash
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# Configure for Release with Ninja
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build all targets in parallel
cmake --build build --parallel
```

## Verify Installation
T81 ships with an exhaustive determinism and conformance test suite.

```bash
# Run the core C++ test suite
ctest --test-dir build --output-on-failure
```

### Reproducibility Gate
To guarantee that your local build produces bit-for-bit identical Canonical Hash emissions as the normative T81 reference, run the Python reproducibility gate:

```bash
mkdir -p build/t81lang-repro
python3 scripts/ci/t81lang_repro_gate.py \
  --t81-bin build/t81 \
  --fixtures-dir tests/fixtures/t81lang_determinism \
  --workdir build/t81lang-repro \
  --hash-out build/t81lang-repro/hash.txt \
  --expected-hash-file tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt
```
*If this gate passes, your local T81 binary is structurally and mathematically sound against the core specification.*

## Running Examples
Try running the shipped native demo binaries to ensure the VM and Tensor layers are operational:

```bash
./build/t81_demo
./build/t81_tensor_ops
./build/t81_ir_roundtrip
```

### Compiling T81Lang Code
You can now use the `t81` CLI frontend to compile and run T81Lang code directly on the TISC ISA:

```bash
./build/t81 code check examples/core-language/hello_world.t81
./build/t81 code build examples/core-language/hello_world.t81 -o build/hello_world.tisc
./build/t81 code run build/hello_world.tisc
```

## Running Benchmarks
T81 ships a rigorous benchmark suite covering core numerics, memory bandwidth, SIMD vectorization, CanonFS, and VM dispatch efficiency.

The runner defaults to a fast local `smoke` profile.

```bash
# Build the benchmark targets
cmake --build build --target benchmark_runner

# Default smoke profile; outputs to JSON
./build/benchmarks/benchmark_runner \
  --benchmark_format=json \
  --benchmark_out=bench.json
```

**Advanced Benchmarking Profiles:**
```bash
# Human-usable full profile:
T81_BENCHMARK_PROFILE=full ./build/benchmarks/benchmark_runner \
  --benchmark_format=json \
  --benchmark_out=bench-full.json

# Exhaustive research/nightly profile:
T81_BENCHMARK_PROFILE=deep ./build/benchmarks/benchmark_runner \
  --benchmark_format=json \
  --benchmark_out=bench-deep.json

# Custom filtered local iteration using the CLI wrapper:
./build/t81 internal benchmark --benchmark_filter='BM_(ArithThroughput|T81LangCompile).*'
```

For performance profiling methodology, see the `benchmarks/README.md` and the developer guides.

## Including T81 in Your CMake Project
T81's core API surface (the `include/t81` directory) can be consumed downstream as a standard CMake package.

First, install it locally:
```bash
cmake --install build --prefix /tmp/t81_install
```

Then consume it in your project's `CMakeLists.txt`:
```cmake
find_package(T81Foundation CONFIG REQUIRED)
target_link_libraries(my_consumer_app PRIVATE T81::t81_core)
```

See `examples/consumer_cmake/` for a minimal, fully working downstream project.

> **Note:** All determinism guarantees are strictly bounded by the [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md).
