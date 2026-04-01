include_guard(GLOBAL)

# Core build toggles
option(T81_BUILD_EXAMPLES "Build T81 C++ examples" ON)
option(T81_BUILD_TESTS "Build T81 C++ unit tests" ON)
option(T81_BUILD_BENCHMARKS "Build T81 benchmark suite" ON)
option(T81_BUILD_FUZZ_TESTS "Build frontend fuzz target" ON)
option(T81_RUN_FRONTEND_FUZZ_IN_CTEST "Run frontend fuzz target via ctest matrix" OFF)
option(T81_ENABLE_LIBFUZZER "Link fuzz_parser/fuzz_vm with libFuzzer driver (-fsanitize=fuzzer); requires Clang with libFuzzer support" OFF)
option(T81_ENABLE_ASAN "Enable AddressSanitizer for supported compilers" OFF)
option(T81_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for supported compilers" OFF)

# T81_HYBRID_MLP: route MLP projections (gate/up/down) through FP32 FMADD
# instead of ternary cmp+sel, keeping attention projections (Q/K/V/O) ternary.
# Throughput uplift: ~3× on MLP layers; compiler autovectorises to FMADD.
# GOVERNANCE: bypasses the ternary weight invariant for MLP layers.
# Axion policy must explicitly permit FP32 dispatch on gate/up/down tensors.
# Not suitable for RFC-0034-certified deployments without policy approval.
option(T81_HYBRID_MLP "FP32 MLP projections + ternary attention (throughput; requires Axion policy)" OFF)
option(T81_BUILD_PLAYGROUND "Build in-browser Wasm playground (requires Emscripten)" OFF)
option(T81_USE_CXX23 "Build with C++23 language mode (fallback: C++20)" ON)
option(T81_TRITWISE_PROFILE "Enable profiling for tritwise operations" OFF)
option(T81_STRICT_DETERMINISTIC_FLOAT "Use deterministic float math paths where available" ON)
option(T81_ENABLE_LLAMA_CPP "Enable experimental llama.cpp adapter (non-DCP)" OFF)
option(T81_BUILD_DIAGNOSTICS "Build off-core floating-point diagnostic tools (NOT part of the deterministic core)" OFF)
option(T81_BUILD_TUI "Build TUI frontends: t81 studio (human) and t81 agent (AI-native) — RFC-0033" ON)
option(T81_EXPORT_LLAMA_ADAPTER
  "Export/install t81_llama_adapter (currently unsupported with vendored llama.cpp)"
  OFF)
option(T81_WARN_STRICT "Strict warning scan mode (warn-strict preset)" OFF)

# Optional language/runtime surfaces
option(T81_ENABLE_LLVM "Enable LLVM IR backend" OFF)
option(T81_ENABLE_MLIR "Enable MLIR frontend (requires T81_ENABLE_LLVM=ON)" OFF)
option(T81_ENABLE_C_FRONTEND "Enable experimental C-subset frontend (requires MLIR/LLVM/libclang)" OFF)
option(T81_ENABLE_RUST_FRONTEND "Enable experimental Rust-subset frontend scaffold (requires MLIR/LLVM/rustc)" OFF)
option(T81_ENABLE_PYTHON_FRONTEND "Enable experimental Python-subset frontend (requires Python3 + C frontend + MLIR/LLVM)" OFF)
option(T81_ENABLE_TERNARYOS "Enable TernaryOS HAL, shell, and user environment" ON)
option(T81_ENABLE_DPE "Enable DPE task graph primitives (RFC-DPE-0002 through RFC-DPE-0009)" ON)
option(T81_ENABLE_AI_EXPERIMENTS "Build the experimental AI CLI compatibility surface" OFF)
option(T81_BUILD_PYTHON_BINDINGS "Build the _t81 pybind11 extension module" OFF)

# Llama reproducibility gate inputs
set(T81_LLAMA_REPRO_MODEL "" CACHE FILEPATH "Model path for llama.cpp reproducibility gate")
set(T81_LLAMA_REPRO_POLICY "" CACHE FILEPATH "Policy path for llama.cpp reproducibility gate")
set(T81_LLAMA_REPRO_PROMPT "" CACHE STRING "Prompt for llama.cpp reproducibility gate")
