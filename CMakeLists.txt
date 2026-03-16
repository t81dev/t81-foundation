cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
project(t81 LANGUAGES C CXX)

# ---------- Version ----------
set(T81_VERSION_MAJOR 1)
set(T81_VERSION_MINOR 3)
set(T81_VERSION_PATCH 2)
set(T81_VERSION "${T81_VERSION_MAJOR}.${T81_VERSION_MINOR}.${T81_VERSION_PATCH}")

# Generate version header
configure_file(
  include/t81/config.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/t81/config.hpp
  @ONLY
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(T81_BUILD_EXAMPLES "Build T81 C++ examples" ON)
option(T81_BUILD_TESTS    "Build T81 C++ unit tests" ON)
option(T81_BUILD_BENCHMARKS "Build T81 benchmark suite" ON)
option(T81_BUILD_FUZZ_TESTS "Build frontend fuzz target" ON)
option(T81_RUN_FRONTEND_FUZZ_IN_CTEST "Run frontend fuzz target via ctest matrix" OFF)
option(T81_ENABLE_LIBFUZZER "Link fuzz_parser/fuzz_vm with libFuzzer driver (-fsanitize=fuzzer); requires Clang with libFuzzer support" OFF)
option(T81_ENABLE_ASAN "Enable AddressSanitizer for supported compilers" OFF)
option(T81_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for supported compilers" OFF)
option(T81_USE_CXX23 "Build with C++23 language mode (fallback: C++20)" ON)
option(T81_TRITWISE_PROFILE "Enable profiling for tritwise operations" OFF)
option(T81_STRICT_DETERMINISTIC_FLOAT "Use deterministic float math paths where available" ON)
option(T81_ENABLE_LLAMA_CPP "Enable experimental llama.cpp adapter (non-DCP)" OFF)
option(T81_BUILD_DIAGNOSTICS "Build off-core floating-point diagnostic tools (NOT part of the deterministic core)" OFF)
option(T81_BUILD_TUI "Build TUI frontends: t81 studio (human) and t81 agent (AI-native) — RFC-0033" ON)
option(T81_EXPORT_LLAMA_ADAPTER
  "Export/install t81_llama_adapter (currently unsupported with vendored llama.cpp)"
  OFF)
set(T81_LLAMA_REPRO_MODEL "" CACHE FILEPATH "Model path for llama.cpp reproducibility gate")
set(T81_LLAMA_REPRO_POLICY "" CACHE FILEPATH "Policy path for llama.cpp reproducibility gate")
set(T81_LLAMA_REPRO_PROMPT "" CACHE STRING "Prompt for llama.cpp reproducibility gate")

# =============================================================================
# Detect Homebrew/non-system LLVM libc++ directory
# When using Homebrew LLVM on macOS, libc++ lives in <compiler-root>/lib/c++
# rather than the system library path.  Static archives compiled with LLVM 21+
# reference std::__1::__hash_memory as an out-of-line symbol, so executables
# that link those archives must have the LLVM lib/c++ dir on their search path.
# =============================================================================
if(APPLE)
  get_filename_component(_T81_CXX_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(_T81_CXX_ROOT    "${_T81_CXX_BIN_DIR}"  DIRECTORY)
  if(EXISTS "${_T81_CXX_ROOT}/lib/c++/libc++.dylib")
    set(T81_LLVM_LIBCXX_DIR "${_T81_CXX_ROOT}/lib/c++")
    message(STATUS "[t81] Detected LLVM libc++ at ${T81_LLVM_LIBCXX_DIR}")
    # Apply globally: every executable in this build needs the LLVM lib/c++ dir
    # on its linker search path so std::__1::__hash_memory resolves correctly.
    add_link_options("-L${T81_LLVM_LIBCXX_DIR}")
  endif()
endif()

if(T81_TRITWISE_PROFILE)
  add_compile_definitions(T81_TRITWISE_PROFILE)
endif()

if(T81_STRICT_DETERMINISTIC_FLOAT)
  add_compile_definitions(T81_DETERMINISTIC)
endif()

if(T81_USE_CXX23)
  set(T81_CXX_STD_FEATURE cxx_std_23)
else()
  set(T81_CXX_STD_FEATURE cxx_std_20)
endif()

if(T81_ENABLE_LLAMA_CPP)
  set(T81_LLAMA_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp")
  if(NOT EXISTS "${T81_LLAMA_CPP_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
      "T81_ENABLE_LLAMA_CPP=ON but llama.cpp was not found at ${T81_LLAMA_CPP_DIR}. "
      "Clone https://github.com/ggml-org/llama.cpp.git into third_party/llama.cpp.")
  endif()

  # Keep the embedded dependency minimal and deterministic: library-only build.
  set(LLAMA_BUILD_COMMON OFF CACHE BOOL "llama.cpp common helpers" FORCE)
  set(LLAMA_BUILD_TESTS OFF CACHE BOOL "llama.cpp tests" FORCE)
  set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "llama.cpp tools" FORCE)
  set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "llama.cpp examples" FORCE)
  set(LLAMA_BUILD_SERVER OFF CACHE BOOL "llama.cpp server" FORCE)
  add_subdirectory("${T81_LLAMA_CPP_DIR}" "${CMAKE_BINARY_DIR}/third_party/llama.cpp" EXCLUDE_FROM_ALL)
  message(STATUS "T81_ENABLE_LLAMA_CPP=ON (using ${T81_LLAMA_CPP_DIR})")
endif()

# =============================================================================
# FTXUI — TUI frontends (RFC-0033)
# =============================================================================
if(T81_BUILD_TUI)
  include(FetchContent)
  FetchContent_Declare(
    ftxui
    URL      https://github.com/ArthurSonzogni/FTXUI/archive/refs/tags/v5.0.0.tar.gz
  )
  set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(FTXUI_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
  set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(ftxui)
  message(STATUS "T81_BUILD_TUI=ON (FTXUI v5.0.0)")
endif()

# CPU feature gating used for SIMD source/flags.
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" T81_SYSTEM_PROCESSOR_LOWER)
set(T81_IS_X86_64 OFF)
if(T81_SYSTEM_PROCESSOR_LOWER STREQUAL "x86_64" OR T81_SYSTEM_PROCESSOR_LOWER STREQUAL "amd64")
  set(T81_IS_X86_64 ON)
endif()

# Global compiler flags (ANSI warnings for GCC/Clang; use /W4 for MSVC)
if(MSVC)
    add_compile_options(/W4 /O2 /DNDEBUG /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -DNDEBUG)
endif()

if(MSVC)
    add_compile_options(/wd4244 /wd4307 /wd4018 /wd4101 /wd4189 /wd4996 /wd4456)
endif()

# warn-strict mode: suppress -Wextra subwarnings that are not Windows CI issues.
# These must come AFTER -Wall/-Wextra above so they take effect.
option(T81_WARN_STRICT "Strict warning scan mode (warn-strict preset)" OFF)
if(T81_WARN_STRICT AND NOT MSVC)
    add_compile_options(-Wno-unused-parameter -Wno-missing-field-initializers)
endif()

if(NOT MSVC)
  set(T81_SANITIZER_FLAGS "")
  if(T81_ENABLE_ASAN)
    list(APPEND T81_SANITIZER_FLAGS -fsanitize=address -fno-omit-frame-pointer)
    add_compile_definitions(T81_ASAN_ENABLED)
  endif()
  if(T81_ENABLE_UBSAN)
    list(APPEND T81_SANITIZER_FLAGS -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
  endif()
  if(T81_SANITIZER_FLAGS)
    add_compile_options(${T81_SANITIZER_FLAGS})
    add_link_options(${T81_SANITIZER_FLAGS})
    message(STATUS "Enabled sanitizers: ${T81_SANITIZER_FLAGS}")
  endif()
endif()

# =============================================================================
# Core library
# =============================================================================
add_library(t81_core STATIC
  core/types/bigint.cpp
  core/types/fraction.cpp
  core/isa/encoding.cpp
  src/canonfs/in_memory_driver.cpp
  src/canonfs/persistent_driver.cpp
  core/determinism/canon_hash81.cpp
  core/math/t81_soft_math/t81_soft_math.cpp
  experimental/hanoi/error.cpp
  experimental/hanoi/in_memory_kernel.cpp
  experimental/tiers/cog/promotion.cpp
  experimental/tiers/cog/tier1/symbolic.cpp
  experimental/tiers/cog/tier2/reflective.cpp
  experimental/tiers/cog/tier3/recursive.cpp
  experimental/tiers/cog/tier4/tier4_loop.cpp
  experimental/distributed/distributed.cpp
  experimental/tiers/cog/tier5/infinite.cpp
  src/codec/base243.cpp
  src/codec/base81.cpp
  src/codec/base81_balanced.cpp
  src/codec/base81_packed.cpp
  src/codec/metadata.cpp
  src/codec/trit_packing.cpp
  src/crypto/sha3.cpp
  src/setun/bridge.cpp
)
add_library(t81_types ALIAS t81_core)
target_compile_features(t81_core PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(t81_tracing OBJECT
  runtime/tracing/canonhash81.cpp
)
target_compile_features(t81_tracing PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_tracing PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(t81_core PRIVATE $<TARGET_OBJECTS:t81_tracing>)

if(T81_IS_X86_64)
  set(T81_AVX2_SOURCE src/simd/base81_digits_avx2.cpp)
  target_sources(t81_core PRIVATE ${T81_AVX2_SOURCE})
  if(NOT MSVC)
    set_source_files_properties(${T81_AVX2_SOURCE} PROPERTIES COMPILE_OPTIONS "-mavx2;-mfma")
  endif()
endif()

# =============================================================================
# Promoted AI subsystem libraries (RFC-0032)
# =============================================================================

# t81_math_quantization — deterministic ternary codec (RFC-0032 Phase 1, C-01).
# Integer-only; no float, no timing, no external deps. Safe for deterministic core.
add_library(t81_math_quantization STATIC
  core/math/quantization/ternary_codec.cpp
)
target_compile_features(t81_math_quantization PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_math_quantization PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(t81_math_quantization PUBLIC t81_core)

# =============================================================================
# Other libraries
# =============================================================================
add_library(t81_axion STATIC
  kernel/axion/axion_api.cpp
  kernel/axion/engine.cpp
  kernel/axion/ethics.cpp
  kernel/axion/nondeterminism_detector.cpp
  kernel/axion/policy_engine.cpp
  kernel/axion/policy_serialization.cpp
  kernel/axion/canonfs_hook.cpp
  # RFC-0032 Phase 3 — promoted AI governance components
  kernel/axion/ai_hooks.cpp
  kernel/axion/ai_model_loader.cpp
  src/axion/policy_validator.cpp
)
target_link_libraries(t81_axion PUBLIC t81_core)

# =============================================================================
# FFI Library - RFC-00B8 Governed Foreign Function Interface
# =============================================================================
add_library(t81_ffi STATIC
  core/ffi/ffi_dispatcher.cpp
)
target_include_directories(t81_ffi PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(t81_ffi PUBLIC t81_core)

add_library(t81_io STATIC src/io/tensor_loader.cpp)
target_link_libraries(t81_io PUBLIC t81_core)
target_include_directories(t81_io PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(t81_c_api STATIC src/c_api/t81_c_api.cpp)
target_link_libraries(t81_c_api PUBLIC t81_core)
target_include_directories(t81_c_api
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(t81_tool_model STATIC
  tooling/model/weights.cpp
  tooling/model/weights_t81w_emitter.cpp
)
target_link_libraries(t81_tool_model PUBLIC t81_core)
target_include_directories(t81_tool_model PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(T81_ENABLE_LLAMA_CPP)
  add_library(t81_llama_adapter STATIC
    tooling/model/llama_cpp_adapter.cpp
  )
  target_compile_features(t81_llama_adapter PUBLIC ${T81_CXX_STD_FEATURE})
  target_include_directories(t81_llama_adapter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_link_libraries(t81_llama_adapter PUBLIC t81_core t81_axion llama)
  target_compile_definitions(t81_llama_adapter PUBLIC T81_HAS_LLAMA_CPP=1)
endif()

add_library(t81_lang_frontend STATIC
  lang/frontend/builtin_registry.cpp
  lang/frontend/ir_generator.cpp
  lang/frontend/lexer.cpp
  lang/frontend/parser.cpp
  lang/frontend/semantic_analyzer.cpp
  lang/frontend/symbol_table.cpp
)
target_link_libraries(t81_lang_frontend PUBLIC t81_core)
target_include_directories(t81_lang_frontend PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(t81_isa STATIC
  core/isa/pretty_printer.cpp
  core/isa/binary_emitter.cpp
  core/isa/binary_io.cpp
  core/isa/base81_view.cpp
)
target_link_libraries(t81_isa PUBLIC t81_core)
target_include_directories(t81_isa PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(t81_vm STATIC
  core/vm/vm.cpp
  core/vm/tier_limits.cpp
  core/vm/value_ops.cpp
  core/vm/memory_segments.cpp
  core/vm/policy_trace_bridge.cpp
  core/vm/runtime_state_helpers.cpp
  core/vm/gc_helpers.cpp
  core/vm/tensor_helpers.cpp
  core/vm/ffi_dispatcher.cpp
  # RFC-0032 Phase 4 — promoted AI VM backend adapter (C-02)
  core/vm/ai_backend/backend_adapter.cpp
  core/ffi/ffi_dispatcher.cpp
)
target_include_directories(t81_vm PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
add_library(t81_jit OBJECT
  runtime/jit/jit_compiler.cpp
  runtime/jit/jit_trace_cache.cpp   # RFC-0028 §4: CanonFS-backed trace cache
)
target_compile_features(t81_jit PUBLIC ${T81_CXX_STD_FEATURE})
target_include_directories(t81_jit PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(t81_vm PRIVATE $<TARGET_OBJECTS:t81_jit>)
target_link_libraries(t81_vm PUBLIC t81_core t81_isa t81_axion t81_ffi)

# =============================================================================
# LLVM IR Backend (optional)
# Enable with: cmake -DT81_ENABLE_LLVM=ON
# Requires LLVM 18+ installed (e.g. brew install llvm).
# When enabled, `t81 llvm compile` lowers TISC bytecode to LLVM IR / bitcode.
# =============================================================================
option(T81_ENABLE_LLVM "Enable LLVM IR backend" OFF)

if(T81_ENABLE_LLVM)
  find_package(LLVM CONFIG
    HINTS /opt/homebrew/opt/llvm/lib/cmake/llvm
          /usr/local/opt/llvm/lib/cmake/llvm
          /usr/lib/llvm-18/lib/cmake/llvm)
  if(LLVM_FOUND)
    message(STATUS "[t81] LLVM ${LLVM_PACKAGE_VERSION} — building LLVM IR backend")
    include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
    llvm_map_components_to_libnames(t81_llvm_libs
      core support irreader bitwriter analysis passes)
    add_library(t81_llvm STATIC
      src/llvm/tisc_to_llvm.cpp)
    target_include_directories(t81_llvm PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
      ${LLVM_INCLUDE_DIRS})
    target_link_libraries(t81_llvm PUBLIC t81_core ${t81_llvm_libs})
    target_compile_definitions(t81_llvm PUBLIC T81_HAS_LLVM=1)
    target_compile_features(t81_llvm PUBLIC ${T81_CXX_STD_FEATURE})
    set(T81_HAS_LLVM_BACKEND ON CACHE INTERNAL "LLVM backend compiled")
  else()
    message(STATUS "[t81] LLVM not found — t81_llvm remains a stub")
    add_library(t81_llvm INTERFACE)
    target_include_directories(t81_llvm INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_llvm INTERFACE t81_core)
  endif()
else()
  add_library(t81_llvm INTERFACE)
  target_include_directories(t81_llvm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_llvm INTERFACE t81_core)
endif()

# =============================================================================
# MLIR Frontend (optional)
# Enable with: cmake -DT81_ENABLE_MLIR=ON  (also requires T81_ENABLE_LLVM=ON)
# Requires LLVM/MLIR 18+ installed (e.g. brew install llvm).
# When enabled, `t81 mlir compile` lowers TISC to MLIR text; `t81 mlir lower`
# lowers MLIR to LLVM IR.  Float ops use standard math.* dialect by default
# (non-DCP); --mode=dcp emits func.call @t81_dmath_* for bit-exact float math.
# =============================================================================
option(T81_ENABLE_MLIR "Enable MLIR frontend (requires T81_ENABLE_LLVM=ON)" OFF)

if(T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND LLVM_FOUND)
  find_package(MLIR CONFIG
    HINTS /opt/homebrew/opt/llvm/lib/cmake/mlir
          /usr/local/opt/llvm/lib/cmake/mlir
          /usr/lib/llvm-18/lib/cmake/mlir)
  if(MLIR_FOUND)
    message(STATUS "[t81] MLIR ${MLIR_PACKAGE_VERSION} — building MLIR frontend")
    include_directories(SYSTEM ${MLIR_INCLUDE_DIRS})
    add_definitions(${MLIR_DEFINITIONS})
    add_library(t81_dmath_runtime STATIC
      src/mlir/dmath_c_wrappers.cpp)
    target_include_directories(t81_dmath_runtime PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_dmath_runtime PUBLIC t81_core)
    target_compile_features(t81_dmath_runtime PUBLIC ${T81_CXX_STD_FEATURE})
    add_library(t81_mlir STATIC
      src/mlir/t81_dialect.cpp
      src/mlir/tisc_to_mlir.cpp
      src/mlir/mlir_to_llvm.cpp)
    target_include_directories(t81_mlir PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
      ${MLIR_INCLUDE_DIRS}
      ${LLVM_INCLUDE_DIRS})
    target_link_libraries(t81_mlir PUBLIC
      t81_core
      t81_dmath_runtime
      MLIRAnalysis
      MLIRArithDialect
      MLIRArithToLLVM
      MLIRBuiltinToLLVMIRTranslation
      MLIRControlFlowDialect
      MLIRControlFlowToLLVM
      MLIRFuncDialect
      MLIRFuncToLLVM
      MLIRLLVMDialect
      MLIRLLVMToLLVMIRTranslation
      MLIRLLVMIRTransforms
      MLIRMathDialect
      MLIRMathToLLVM
      MLIRMemRefDialect
      MLIRMemRefToLLVM
      MLIRPass
      MLIRTargetLLVMIRExport
      MLIRTransforms
      ${t81_llvm_libs})
    target_compile_definitions(t81_mlir PUBLIC T81_HAS_MLIR=1 T81_HAS_LLVM=1)
    target_compile_features(t81_mlir PUBLIC ${T81_CXX_STD_FEATURE})
    set(T81_HAS_MLIR_BACKEND ON CACHE INTERNAL "MLIR backend compiled")
  else()
    message(STATUS "[t81] MLIR not found — t81_mlir remains a stub")
    add_library(t81_mlir INTERFACE)
    target_include_directories(t81_mlir INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_mlir INTERFACE t81_core)
  endif()
else()
  add_library(t81_mlir INTERFACE)
  target_include_directories(t81_mlir INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_mlir INTERFACE t81_core)
endif()

# =============================================================================
# Experimental C Frontend (optional)
# Enable with: cmake -DT81_ENABLE_C_FRONTEND=ON
# Requires MLIR/LLVM plus libclang.
# =============================================================================
option(T81_ENABLE_C_FRONTEND "Enable experimental C-subset frontend (requires MLIR/LLVM/libclang)" OFF)

if(T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND LLVM_FOUND AND MLIR_FOUND)
  find_path(T81_CLANG_C_INCLUDE_DIR
    NAMES clang-c/Index.h
    HINTS /opt/homebrew/opt/llvm/include
          /usr/local/opt/llvm/include
          /usr/lib/llvm-18/include)
  find_library(T81_CLANG_C_LIBRARY
    NAMES clang libclang
    HINTS /opt/homebrew/opt/llvm/lib
          /usr/local/opt/llvm/lib
          /usr/lib/llvm-18/lib)
  if(T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY)
    message(STATUS "[t81] libclang — building experimental C frontend")
    add_library(t81_c_frontend STATIC
      src/c_frontend/compile.cpp)
    target_include_directories(t81_c_frontend PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
      ${T81_CLANG_C_INCLUDE_DIR})
    target_link_libraries(t81_c_frontend PUBLIC
      t81_core
      t81_mlir
      ${T81_CLANG_C_LIBRARY})
    target_compile_definitions(t81_c_frontend PUBLIC T81_HAS_C_FRONTEND=1 T81_HAS_MLIR=1 T81_HAS_LLVM=1)
    target_compile_features(t81_c_frontend PUBLIC ${T81_CXX_STD_FEATURE})
  else()
    message(STATUS "[t81] libclang not found — t81_c_frontend remains a stub")
    add_library(t81_c_frontend INTERFACE)
    target_include_directories(t81_c_frontend INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_c_frontend INTERFACE t81_core)
  endif()
else()
  add_library(t81_c_frontend INTERFACE)
  target_include_directories(t81_c_frontend INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_c_frontend INTERFACE t81_core)
endif()

# =============================================================================
# Experimental Rust Frontend (optional)
# Enable with: cmake -DT81_ENABLE_RUST_FRONTEND=ON
# Requires MLIR/LLVM plus a Rust toolchain on PATH.
# =============================================================================
option(T81_ENABLE_RUST_FRONTEND "Enable experimental Rust-subset frontend scaffold (requires MLIR/LLVM/rustc)" OFF)

if(T81_ENABLE_RUST_FRONTEND AND T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND LLVM_FOUND AND MLIR_FOUND AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY)
  find_program(T81_RUSTC_EXECUTABLE NAMES rustc)
  if(T81_RUSTC_EXECUTABLE)
    message(STATUS "[t81] rustc — building experimental Rust frontend subset")
    add_library(t81_rust_frontend STATIC
      src/rust_frontend/compile.cpp)
    target_include_directories(t81_rust_frontend PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_rust_frontend PUBLIC
      t81_core
      t81_c_frontend
      t81_mlir)
    target_compile_definitions(t81_rust_frontend PUBLIC T81_HAS_RUST_FRONTEND=1 T81_HAS_MLIR=1 T81_HAS_LLVM=1)
    target_compile_features(t81_rust_frontend PUBLIC ${T81_CXX_STD_FEATURE})
  else()
    message(STATUS "[t81] rustc not found — t81_rust_frontend remains a stub")
    add_library(t81_rust_frontend INTERFACE)
    target_include_directories(t81_rust_frontend INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_rust_frontend INTERFACE t81_core)
  endif()
else()
  add_library(t81_rust_frontend INTERFACE)
  target_include_directories(t81_rust_frontend INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_rust_frontend INTERFACE t81_core)
endif()

# =============================================================================
# Experimental Python Frontend (optional)
# Enable with: cmake -DT81_ENABLE_PYTHON_FRONTEND=ON
# Requires Python 3 plus the experimental C frontend adapter.
# =============================================================================
option(T81_ENABLE_PYTHON_FRONTEND "Enable experimental Python-subset frontend (requires Python3 + C frontend + MLIR/LLVM)" OFF)

if(T81_ENABLE_PYTHON_FRONTEND AND T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND LLVM_FOUND AND MLIR_FOUND AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  if(Python3_EXECUTABLE)
    message(STATUS "[t81] python3 — building experimental Python frontend subset")
    add_library(t81_python_frontend STATIC
      src/python_frontend/compile.cpp)
    target_include_directories(t81_python_frontend PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_python_frontend PUBLIC
      t81_core
      t81_c_frontend
      t81_mlir)
    target_compile_definitions(t81_python_frontend PUBLIC
      T81_HAS_PYTHON_FRONTEND=1
      T81_HAS_C_FRONTEND=1
      T81_HAS_MLIR=1
      T81_HAS_LLVM=1
      T81_PYTHON3_EXECUTABLE="${Python3_EXECUTABLE}"
      T81_PYTHON_FRONTEND_SCRIPT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/scripts/python_frontend/normalize_subset.py")
    target_compile_features(t81_python_frontend PUBLIC ${T81_CXX_STD_FEATURE})
  else()
    message(STATUS "[t81] python3 not found — t81_python_frontend remains a stub")
    add_library(t81_python_frontend INTERFACE)
    target_include_directories(t81_python_frontend INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(t81_python_frontend INTERFACE t81_core)
  endif()
else()
  add_library(t81_python_frontend INTERFACE)
  target_include_directories(t81_python_frontend INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(t81_python_frontend INTERFACE t81_core)
endif()

# =============================================================================
# TernOS Phase 1 — HAL experimental scaffold (RFC-00B0)
# Enable with -DT81_ENABLE_TERNARYOS=ON
# =============================================================================
option(T81_ENABLE_TERNARYOS "Enable experimental TernOS HAL scaffold (non-DCP)" OFF)

if(T81_ENABLE_TERNARYOS)
  message(STATUS "[t81] T81_ENABLE_TERNARYOS=ON — building experimental TernOS HAL")

  add_library(t81_ternaryos_hal STATIC
    experimental/ternaryos/hal/hal_main.cpp
    experimental/ternaryos/hal/hal_c_abi.cpp
    experimental/ternaryos/hal/interrupt_table.cpp
    experimental/ternaryos/hal/hosted_stub.cpp
    experimental/ternaryos/hal/virtualbox_platform.cpp
    experimental/ternaryos/hal/virtualbox_guest_devices.cpp
    experimental/ternaryos/hal/qemu_platform.cpp
    experimental/ternaryos/hal/qemu_kernel_entry.cpp
    experimental/ternaryos/kernel/kernel_runtime.cpp
    experimental/ternaryos/kernel/kernel_actions.cpp
    experimental/ternaryos/kernel/kernel_abi.cpp
    experimental/ternaryos/kernel/kernel_abi_wire.cpp
    experimental/ternaryos/kernel/kernel_executable.cpp
    experimental/ternaryos/kernel/kernel_faults.cpp
    experimental/ternaryos/kernel/kernel_interrupts.cpp
    experimental/ternaryos/kernel/kernel_interrupt_policy.cpp
    experimental/ternaryos/kernel/kernel_loader.cpp
    experimental/ternaryos/kernel/kernel_lifecycle.cpp
    experimental/ternaryos/kernel/kernel_trap_shim.cpp
    experimental/ternaryos/hal/aarch64_trap_entry.cpp
    experimental/ternaryos/kernel/kernel_pager.cpp
    experimental/ternaryos/kernel/kernel_queries.cpp
    experimental/ternaryos/kernel/kernel_runtime_utils.cpp
    experimental/ternaryos/kernel/kernel_views.cpp
    experimental/ternaryos/dev/hosted_block_dev.cpp
    experimental/ternaryos/dev/virtualbox_ahci_dev.cpp
    experimental/ternaryos/dev/virtualbox_e1000_dev.cpp
    experimental/ternaryos/dev/virtualbox_vmsvga_dev.cpp
    experimental/ternaryos/dev/canon_store.cpp
    experimental/ternaryos/dev/framebuffer.cpp
    experimental/ternaryos/dev/ttf.cpp
    experimental/ternaryos/dev/pl011_uart.cpp
    experimental/ternaryos/dev/gicv3.cpp
    experimental/ternaryos/mmu/ternary_page_alloc.cpp
    experimental/ternaryos/mmu/page_table.cpp
    experimental/ternaryos/sched/context_switch.cpp
    # Phase 3 — scheduling & IPC
    experimental/ternaryos/sched/run_queue.cpp
    experimental/ternaryos/sched/scheduler.cpp
    experimental/ternaryos/ipc/canon_message.cpp)

  target_include_directories(t81_ternaryos_hal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

  target_link_libraries(t81_ternaryos_hal PUBLIC t81_core t81_axion)
  target_compile_features(t81_ternaryos_hal PUBLIC ${T81_CXX_STD_FEATURE})

  # DPE kernel wiring — compiled into t81_ternaryos_hal only when both flags are ON.
  if(T81_ENABLE_DPE)
    target_sources(t81_ternaryos_hal PRIVATE
      experimental/ternaryos/kernel/kernel_epoch.cpp)
    target_include_directories(t81_ternaryos_hal PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(t81_ternaryos_hal PUBLIC t81_dpe)
    target_compile_definitions(t81_ternaryos_hal PUBLIC T81_ENABLE_DPE=1)
  endif()

  add_library(t81_ternaryos_shell STATIC
    experimental/ternaryos/shell/shell_session.cpp)
  target_include_directories(t81_ternaryos_shell PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  target_link_libraries(t81_ternaryos_shell PUBLIC t81_ternaryos_hal)
  target_compile_features(t81_ternaryos_shell PUBLIC ${T81_CXX_STD_FEATURE})

  add_executable(t81_ternaryos_demo
    experimental/ternaryos/apps/demo.cpp)
  target_link_libraries(t81_ternaryos_demo PRIVATE t81_ternaryos_hal)

  add_executable(t81_ternaryos_shell_demo
    experimental/ternaryos/apps/shell_demo.cpp)
  target_link_libraries(t81_ternaryos_shell_demo PRIVATE t81_ternaryos_shell)

  add_executable(t81_ternaryos_shell_startup_snapshot
    experimental/ternaryos/shell/shell_startup_snapshot.cpp)
  target_link_libraries(t81_ternaryos_shell_startup_snapshot PRIVATE t81_ternaryos_shell)

  add_executable(t81_ternaryos_phase4_startup_snapshot
    experimental/ternaryos/dev/phase4_startup_snapshot.cpp)
  target_link_libraries(t81_ternaryos_phase4_startup_snapshot PRIVATE t81_ternaryos_hal)

  add_executable(t81_ternaryos_qemu_slice6_snapshot
    experimental/ternaryos/dev/qemu_slice6_startup_snapshot.cpp)
  target_link_libraries(t81_ternaryos_qemu_slice6_snapshot PRIVATE t81_ternaryos_hal)

  if(T81_BUILD_TUI)
    add_executable(t81_ternaryos_shell_tui
      experimental/ternaryos/apps/shell_tui.cpp)
    target_link_libraries(t81_ternaryos_shell_tui PRIVATE
      t81_ternaryos_shell
      ftxui::screen
      ftxui::dom
      ftxui::component)
    target_compile_features(t81_ternaryos_shell_tui PRIVATE ${T81_CXX_STD_FEATURE})
  endif()

  set(T81_TERNARYOS_GUEST_ARTIFACT_DEPS t81_ternaryos_demo)
  set(T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_demo)
  find_program(T81_LLVM_CLANG NAMES clang PATHS /opt/homebrew/opt/llvm/bin NO_DEFAULT_PATH)
  find_program(T81_LLD_LINK NAMES lld-link PATHS /opt/homebrew/bin /opt/homebrew/opt/lld/bin NO_DEFAULT_PATH)
  if(T81_LLVM_CLANG)
    set(T81_TERNARYOS_EFI_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox/BOOTX64.obj)
    set(T81_TERNARYOS_X86_64_EFI_SHIM_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox/BOOTX64_shim.obj)
    set(T81_TERNARYOS_X86_64_EFI ${CMAKE_BINARY_DIR}/ternaryos/virtualbox/BOOTX64.EFI)
    set(T81_TERNARYOS_ARMV8_EFI_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64.obj)
    set(T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64_CTRL.obj)
    set(T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64_shim.obj)
    set(T81_TERNARYOS_ARMV8_EFI ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64.EFI)
    set(T81_TERNARYOS_ARMV8_EFI_CTRL ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/BOOTAA64_CTRL.EFI)
    set(T81_TERNARYOS_ARMV8_STARTUP_SHELL_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-shell-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_shell.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_SESSION_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-session-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_session.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_HISTORY_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-history-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_history.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_STORE_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-store-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_store.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_REF_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-ref-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_REF_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_ref.h)
    set(T81_TERNARYOS_ARMV8_STARTUP_REPORT_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-report-generated.txt)
    set(T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_startup_report.h)
    set(T81_TERNARYOS_ARMV8_PHASE4_STARTUP_TXT ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/startup-phase4-generated.txt)
    set(T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR ${CMAKE_BINARY_DIR}/ternaryos/generated/axion_phase4_startup.h)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_EFI_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
      COMMAND ${T81_LLVM_CLANG}
              --target=x86_64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_efi_stub.c
              -I${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos
              -o ${T81_TERNARYOS_EFI_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_efi_stub.c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/hal_c_abi.h)
    add_custom_target(t81_ternaryos_virtualbox_efi_obj
      DEPENDS ${T81_TERNARYOS_EFI_OBJ})
    list(APPEND T81_TERNARYOS_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_efi_obj)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
      COMMAND ${T81_LLVM_CLANG}
              --target=x86_64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_x86_64_efi_shim.c
              -I${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos
              -o ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_x86_64_efi_shim.c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/hal_c_abi.h)
    add_custom_target(t81_ternaryos_virtualbox_x86_64_efi_shim_obj
      DEPENDS ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ})
    list(APPEND T81_TERNARYOS_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_x86_64_efi_shim_obj)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_STORE_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_REF_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_REF_HDR}
             ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_TXT}
             ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR}
             ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_TXT}
             ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/generated
      COMMAND $<TARGET_FILE:t81_ternaryos_shell_startup_snapshot>
              ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR}
      COMMAND $<TARGET_FILE:t81_ternaryos_phase4_startup_snapshot>
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_TXT}
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR}
      DEPENDS t81_ternaryos_shell_startup_snapshot
              t81_ternaryos_phase4_startup_snapshot)
    add_custom_target(t81_ternaryos_shell_startup_snapshot_artifact
      DEPENDS ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_TXT}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR}
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_TXT}
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR})
    list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_shell_startup_snapshot_artifact)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_ARMV8_EFI_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_armv8_efi_stub.c
              -I${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos
              -I${CMAKE_BINARY_DIR}/ternaryos/generated
              -o ${T81_TERNARYOS_ARMV8_EFI_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_armv8_efi_stub.c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/hal_c_abi.h
              ${T81_TERNARYOS_ARMV8_STARTUP_SHELL_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_SESSION_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_HISTORY_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_STORE_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REF_HDR}
              ${T81_TERNARYOS_ARMV8_STARTUP_REPORT_HDR}
              ${T81_TERNARYOS_ARMV8_PHASE4_STARTUP_HDR})
    add_custom_target(t81_ternaryos_virtualbox_armv8_efi_obj
      DEPENDS ${T81_TERNARYOS_ARMV8_EFI_OBJ})
    list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi_obj)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_armv8_efi_control.c
              -I${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos
              -o ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_armv8_efi_control.c)
    add_custom_target(t81_ternaryos_virtualbox_armv8_efi_control_obj
      DEPENDS ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ})
    list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi_control_obj)

    add_custom_command(
      OUTPUT ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_armv8_efi_shim.c
              -I${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos
              -o ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/virtualbox_armv8_efi_shim.c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/hal_c_abi.h)
    add_custom_target(t81_ternaryos_virtualbox_armv8_efi_shim_obj
      DEPENDS ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ})
    list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi_shim_obj)

    # ── Slice 8: AArch64 exception vector table assembly ──────────────────────
    # Compile aarch64_exception_vectors.S for the AArch64 PE/COFF target so
    # the vector table binary is available for inclusion in EFI images.
    # The resulting object is not linked into any EFI binary in this slice;
    # it proves the assembly builds cleanly and can be inspected with objdump.
    set(T81_TERNARYOS_AARCH64_VECTORS_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/aarch64/aarch64_exception_vectors.obj)
    add_custom_command(
      OUTPUT ${T81_TERNARYOS_AARCH64_VECTORS_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/aarch64
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/aarch64_exception_vectors.S
              -o ${T81_TERNARYOS_AARCH64_VECTORS_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/aarch64_exception_vectors.S
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/aarch64_trap_entry.hpp)
    add_custom_target(t81_ternaryos_aarch64_exception_vectors_obj
      DEPENDS ${T81_TERNARYOS_AARCH64_VECTORS_OBJ})

    # ── Slice 6: QEMU AArch64 EDK2 guest image ────────────────────────────────
    set(T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64.obj)
    set(T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64_bare.obj)
    set(T81_TERNARYOS_QEMU_SLICE6_EFI
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/BOOTAA64.EFI)
    set(T81_TERNARYOS_QEMU_SLICE6_STARTUP_TXT
        ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/slice6-startup-generated.txt)
    set(T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR
        ${CMAKE_BINARY_DIR}/ternaryos/generated/qemu_slice6_startup.h)

    add_custom_command(
      OUTPUT  ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_TXT}
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/generated
      COMMAND $<TARGET_FILE:t81_ternaryos_qemu_slice6_snapshot>
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_TXT}
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR}
      DEPENDS t81_ternaryos_qemu_slice6_snapshot)
    add_custom_target(t81_ternaryos_qemu_slice6_snapshot_data
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_TXT}
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR})

    add_custom_command(
      OUTPUT  ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fshort-wchar
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/qemu_armv8_efi_stub.c
              -I${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos
              -I${CMAKE_BINARY_DIR}/ternaryos/generated
              -o ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/qemu_armv8_efi_stub.c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/hal_c_abi.h
              ${T81_TERNARYOS_QEMU_SLICE6_STARTUP_HDR})
    add_custom_target(t81_ternaryos_qemu_slice6_efi_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ})

    # Bare-metal kernel entry (post-ExitBootServices PL011/GIC/timer probe).
    add_custom_command(
      OUTPUT  ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
      COMMAND ${T81_LLVM_CLANG}
              --target=aarch64-pc-windows-msvc
              -ffreestanding
              -fno-stack-protector
              -nostdlib
              -c
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/qemu_slice6_bare_kernel.c
              -I${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos
              -o ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ}
      DEPENDS
              ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/hal/qemu_slice6_bare_kernel.c)
    add_custom_target(t81_ternaryos_qemu_slice6_bare_obj
      DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ})

    if(T81_LLD_LINK)
      add_custom_command(
        OUTPUT ${T81_TERNARYOS_X86_64_EFI}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
        COMMAND ${T81_LLD_LINK}
                /subsystem:efi_application
                /entry:efi_main
                /machine:x64
                /nodefaultlib
                /out:${T81_TERNARYOS_X86_64_EFI}
                ${T81_TERNARYOS_EFI_OBJ}
                ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ}
        DEPENDS ${T81_TERNARYOS_EFI_OBJ}
                ${T81_TERNARYOS_X86_64_EFI_SHIM_OBJ})
      add_custom_target(t81_ternaryos_virtualbox_x86_64_efi
        DEPENDS ${T81_TERNARYOS_X86_64_EFI})
      list(APPEND T81_TERNARYOS_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_x86_64_efi)

      add_custom_command(
        OUTPUT ${T81_TERNARYOS_ARMV8_EFI}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
        COMMAND ${T81_LLD_LINK}
                /subsystem:efi_application
                /entry:efi_main
                /machine:arm64
                /nodefaultlib
                /out:${T81_TERNARYOS_ARMV8_EFI}
                ${T81_TERNARYOS_ARMV8_EFI_OBJ}
                ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ}
        DEPENDS ${T81_TERNARYOS_ARMV8_EFI_OBJ}
                ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ})
      add_custom_target(t81_ternaryos_virtualbox_armv8_efi
        DEPENDS ${T81_TERNARYOS_ARMV8_EFI})
      list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi)

      add_custom_command(
        OUTPUT ${T81_TERNARYOS_ARMV8_EFI_CTRL}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
        COMMAND ${T81_LLD_LINK}
                /subsystem:efi_application
                /entry:efi_main
                /machine:arm64
                /nodefaultlib
                /out:${T81_TERNARYOS_ARMV8_EFI_CTRL}
                ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ}
        DEPENDS ${T81_TERNARYOS_ARMV8_EFI_CTRL_OBJ})
      add_custom_target(t81_ternaryos_virtualbox_armv8_efi_control
        DEPENDS ${T81_TERNARYOS_ARMV8_EFI_CTRL})
      list(APPEND T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS t81_ternaryos_virtualbox_armv8_efi_control)

      # Slice 6: link QEMU slice6 BOOTAA64.EFI
      #   stub (EFI entry + ExitBootServices) + bare kernel (PL011/GIC/timer) + shim
      add_custom_command(
        OUTPUT ${T81_TERNARYOS_QEMU_SLICE6_EFI}
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
        COMMAND ${T81_LLD_LINK}
                /subsystem:efi_application
                /entry:efi_main
                /machine:arm64
                /nodefaultlib
                /out:${T81_TERNARYOS_QEMU_SLICE6_EFI}
                ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ}
                ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ}
        DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_EFI_OBJ}
                ${T81_TERNARYOS_QEMU_SLICE6_BARE_OBJ}
                ${T81_TERNARYOS_ARMV8_EFI_SHIM_OBJ})
      add_custom_target(t81_ternaryos_qemu_slice6_efi
        DEPENDS ${T81_TERNARYOS_QEMU_SLICE6_EFI})
    endif()
  endif()

  add_custom_target(t81_ternaryos_virtualbox_guest_artifact
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/build_virtualbox_guest_artifact.sh
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox
            x86_64
    DEPENDS ${T81_TERNARYOS_GUEST_ARTIFACT_DEPS}
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/handoff
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/package_virtualbox_x86_64_handoff.sh
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/ternaryos/handoff
    DEPENDS t81_ternaryos_virtualbox_guest_artifact
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/docs/virtualbox_x86_64_handoff.md
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff_bundle_smoke
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/validate_packaged_virtualbox_x86_64_handoff_bundle.sh
            ${CMAKE_BINARY_DIR}/ternaryos/handoff/ternos_virtualbox_x86_64_handoff
    DEPENDS t81_ternaryos_virtualbox_x86_64_handoff
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff_bundle_negative_smoke
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/expect_packaged_virtualbox_x86_64_handoff_bundle_failure.sh
            ${CMAKE_BINARY_DIR}/ternaryos/handoff/ternos_virtualbox_x86_64_handoff
    DEPENDS t81_ternaryos_virtualbox_x86_64_handoff
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_armv8_dev_artifact
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/build_virtualbox_guest_artifact.sh
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
            armv8
    DEPENDS ${T81_TERNARYOS_ARMV8_GUEST_ARTIFACT_DEPS}
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_host_check
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/check_virtualbox_host.sh
            --soft
            x86_64
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_armv8_host_check
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/check_virtualbox_host.sh
            --soft
            ARMv8
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_armv8_efi_link_check
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/check_efi_link_toolchain.sh
            armv8
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/efi-link-status.txt
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_armv8_boot_probe
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/probe_virtualbox_armv8_vm.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/ternos_virtualbox_armv8_dev_guest.vdi
            5
    DEPENDS t81_ternaryos_virtualbox_armv8_dev_artifact
            t81_ternaryos_virtualbox_armv8_host_check
            t81_ternaryos_virtualbox_armv8_efi_link_check
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_qemu_armv8_efi_control_probe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_control
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/run_qemu_armv8_efi_control.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/ternos_virtualbox_armv8_dev_guest.img
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_control
    DEPENDS t81_ternaryos_virtualbox_armv8_dev_artifact
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_qemu_armv8_guest_probe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/run_qemu_armv8_guest_probe.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_armv8/ternos_virtualbox_armv8_dev_guest.img
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest
            8
    DEPENDS t81_ternaryos_virtualbox_armv8_dev_artifact
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_qemu_x86_64_guest_probe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_x86_64_guest
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/run_qemu_x86_64_guest_probe.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox/ternos_virtualbox_guest.img
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_x86_64_guest
    DEPENDS t81_ternaryos_virtualbox_guest_artifact
    USES_TERMINAL)

  # ── Slice 6: QEMU AArch64 EDK2 disk image + probe ─────────────────────────
  add_custom_target(t81_ternaryos_qemu_slice6_dev_artifact
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/build_qemu_slice6_artifact.sh
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6
    DEPENDS t81_ternaryos_qemu_slice6_efi
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_qemu_armv8_slice6_probe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_slice6_probe
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/run_qemu_armv8_slice6_probe.sh
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_slice6/qemu_slice6_guest.img
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_slice6_probe
            8
    DEPENDS t81_ternaryos_qemu_slice6_dev_artifact
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_qemu_armv8_guest_probe_blocked_fixture
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest_blocked_fixture
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/dev/qemu_armv8_guest_probe_blocked_fixture
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest_blocked_fixture
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/validate_qemu_armv8_guest_reports.sh
            ${CMAKE_BINARY_DIR}/ternaryos/qemu_armv8_guest_blocked_fixture
            blocked
            qemu-armv8-guest-probe-fixture
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff_fixture
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/dev/virtualbox_x86_64_handoff_validation_fixture
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/validate_virtualbox_x86_64_handoff.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture/expected-boot-report.txt
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture/expected-startup-status.txt
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_fixture/recovered-artifacts
    USES_TERMINAL)

  add_custom_target(t81_ternaryos_virtualbox_x86_64_handoff_negative_fixture
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/dev/virtualbox_x86_64_handoff_validation_negative_fixture
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture
    COMMAND /bin/zsh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/expect_virtualbox_x86_64_handoff_failure.sh
            ${CMAKE_CURRENT_SOURCE_DIR}/experimental/ternaryos/scripts/validate_virtualbox_x86_64_handoff.sh
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture/expected-boot-report.txt
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture/expected-startup-status.txt
            ${CMAKE_BINARY_DIR}/ternaryos/virtualbox_x86_64_handoff_negative_fixture/recovered-artifacts
    USES_TERMINAL)

  if(T81_BUILD_TESTS)
    add_executable(t81_ternaryos_hal_boot_test
      experimental/ternaryos/tests/hal_boot_test.cpp)
    target_link_libraries(t81_ternaryos_hal_boot_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_hal_boot_test COMMAND t81_ternaryos_hal_boot_test)

    add_executable(t81_ternaryos_page_alloc_test
      experimental/ternaryos/tests/ternary_page_alloc_test.cpp)
    target_link_libraries(t81_ternaryos_page_alloc_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_page_alloc_test COMMAND t81_ternaryos_page_alloc_test)

    add_executable(t81_ternaryos_context_switch_test
      experimental/ternaryos/tests/context_switch_test.cpp)
    target_link_libraries(t81_ternaryos_context_switch_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_context_switch_test COMMAND t81_ternaryos_context_switch_test)

    add_executable(t81_ternaryos_mmu_test
      experimental/ternaryos/tests/mmu_test.cpp)
    target_link_libraries(t81_ternaryos_mmu_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_mmu_test COMMAND t81_ternaryos_mmu_test)

    # Phase 3 tests
    add_executable(t81_ternaryos_scheduler_test
      experimental/ternaryos/tests/scheduler_test.cpp)
    target_link_libraries(t81_ternaryos_scheduler_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_scheduler_test COMMAND t81_ternaryos_scheduler_test)

    add_executable(t81_ternaryos_ipc_test
      experimental/ternaryos/tests/ipc_test.cpp)
    target_link_libraries(t81_ternaryos_ipc_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_ipc_test COMMAND t81_ternaryos_ipc_test)

    add_executable(t81_ternaryos_device_driver_test
      experimental/ternaryos/tests/device_driver_test.cpp)
    target_link_libraries(t81_ternaryos_device_driver_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_device_driver_test COMMAND t81_ternaryos_device_driver_test)

    add_executable(t81_ternaryos_shell_session_test
      experimental/ternaryos/tests/shell_session_test.cpp)
    target_link_libraries(t81_ternaryos_shell_session_test PRIVATE t81_ternaryos_shell)
    add_test(NAME t81_ternaryos_shell_session_test COMMAND t81_ternaryos_shell_session_test)

    if(T81_ENABLE_DPE)
      add_executable(t81_ternaryos_epoch_submission_test
        experimental/ternaryos/tests/epoch_submission_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_submission_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_submission_test COMMAND t81_ternaryos_epoch_submission_test)

      add_executable(t81_ternaryos_epoch_policy_test
        experimental/ternaryos/tests/epoch_policy_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_policy_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_policy_test COMMAND t81_ternaryos_epoch_policy_test)

      # RFC-DPE-0003 §10 / RFC-DPE-0006 §4 — SubmitEpoch KernelCallKind ABI tests [DPE-07]
      add_executable(t81_ternaryos_epoch_syscall_test
        experimental/ternaryos/tests/epoch_syscall_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_syscall_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_syscall_test COMMAND t81_ternaryos_epoch_syscall_test)

      # RFC-DPE-0007 — Epoch Execution Timeout tests [DPE-08]
      add_executable(t81_ternaryos_epoch_timeout_test
        experimental/ternaryos/tests/epoch_timeout_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_timeout_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_timeout_test COMMAND t81_ternaryos_epoch_timeout_test)

      # RFC-DPE-0008 — Epoch Audit Events tests [DPE-09]
      add_executable(t81_ternaryos_epoch_audit_test
        experimental/ternaryos/tests/epoch_audit_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_audit_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_audit_test COMMAND t81_ternaryos_epoch_audit_test)

      # RFC-DPE-0009 — Epoch History Ring tests [DPE-10]
      add_executable(t81_ternaryos_epoch_history_test
        experimental/ternaryos/tests/epoch_history_test.cpp)
      target_link_libraries(t81_ternaryos_epoch_history_test PRIVATE t81_ternaryos_hal)
      add_test(NAME t81_ternaryos_epoch_history_test COMMAND t81_ternaryos_epoch_history_test)
    endif()

    # RFC-00B6 §5.3.6 — ClaimDevice/ReleaseDevice/QueryDevice KernelCallKind ABI tests [AC-22d]
    add_executable(t81_ternaryos_device_arbitration_syscall_test
      experimental/ternaryos/tests/device_arbitration_syscall_test.cpp)
    target_link_libraries(t81_ternaryos_device_arbitration_syscall_test PRIVATE t81_ternaryos_hal)
    add_test(NAME t81_ternaryos_device_arbitration_syscall_test
             COMMAND t81_ternaryos_device_arbitration_syscall_test)
  endif()
endif()

# =============================================================================
# Experimental DPE — Deterministic Parallel Execution (RFC-DPE-0002+)
# Enable with -DT81_ENABLE_DPE=ON
# =============================================================================
option(T81_ENABLE_DPE "Enable experimental DPE task graph primitives (RFC-DPE-0002)" OFF)

if(T81_ENABLE_DPE)
  message(STATUS "[t81] T81_ENABLE_DPE=ON — building experimental DPE task graph")

  add_library(t81_dpe STATIC
    experimental/dpe/task_graph.cpp
    experimental/dpe/task_runner.cpp
    experimental/dpe/epoch_commit.cpp
    experimental/dpe/thread_pool.cpp
  )
  target_include_directories(t81_dpe PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
  target_link_libraries(t81_dpe PUBLIC t81_core t81_vm)
  target_compile_features(t81_dpe PUBLIC ${T81_CXX_STD_FEATURE})

  if(T81_BUILD_TESTS)
    add_executable(t81_dpe_task_graph_test
      experimental/dpe/tests/task_graph_test.cpp)
    target_link_libraries(t81_dpe_task_graph_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_task_graph_test COMMAND t81_dpe_task_graph_test)

    add_executable(t81_dpe_task_runner_test
      experimental/dpe/tests/task_runner_test.cpp)
    target_link_libraries(t81_dpe_task_runner_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_task_runner_test COMMAND t81_dpe_task_runner_test)

    add_executable(t81_dpe_epoch_commit_test
      experimental/dpe/tests/epoch_commit_test.cpp)
    target_link_libraries(t81_dpe_epoch_commit_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_epoch_commit_test COMMAND t81_dpe_epoch_commit_test)

    add_executable(t81_dpe_epoch_dag_test
      experimental/dpe/tests/epoch_dag_test.cpp)
    target_link_libraries(t81_dpe_epoch_dag_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_epoch_dag_test COMMAND t81_dpe_epoch_dag_test)

    add_executable(t81_dpe_epoch_parallel_test
      experimental/dpe/tests/epoch_parallel_test.cpp)
    target_link_libraries(t81_dpe_epoch_parallel_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_epoch_parallel_test COMMAND t81_dpe_epoch_parallel_test)

    add_executable(t81_dpe_thread_pool_test
      experimental/dpe/tests/thread_pool_test.cpp)
    target_link_libraries(t81_dpe_thread_pool_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_thread_pool_test COMMAND t81_dpe_thread_pool_test)

    add_executable(t81_dpe_float_reduction_test
      experimental/dpe/tests/dpe_float_reduction_test.cpp)
    target_link_libraries(t81_dpe_float_reduction_test PRIVATE t81_dpe)
    add_test(NAME t81_dpe_float_reduction_test COMMAND t81_dpe_float_reduction_test)
  endif()
endif()

# =============================================================================
# CLI & Examples
# =============================================================================
add_library(t81_tool_cli STATIC
  tooling/cli/driver.cpp
  tooling/cli/debugger.cpp
  tooling/cli/canonize_tensor.cpp
)
target_link_libraries(t81_tool_cli PUBLIC t81_lang_frontend t81_isa t81_vm t81_tool_model)
target_link_libraries(t81_tool_cli PUBLIC t81_llvm t81_mlir t81_c_frontend t81_rust_frontend t81_python_frontend)
target_include_directories(t81_tool_cli PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(t81_tool_cli PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# TUI library — built only when T81_BUILD_TUI=ON
if(T81_BUILD_TUI)
  add_library(t81_tui STATIC
    tooling/tui/common.cpp
    tooling/tui/studio.cpp
    tooling/tui/agent.cpp
  )
  target_link_libraries(t81_tui PUBLIC t81_tool_cli ftxui::screen ftxui::dom ftxui::component)
  target_include_directories(t81_tui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_include_directories(t81_tui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_features(t81_tui PUBLIC ${T81_CXX_STD_FEATURE})

  if(T81_BUILD_TESTS)
    add_executable(tui_snapshot_test tests/cpp/tui_snapshot_test.cpp)
    target_link_libraries(tui_snapshot_test PRIVATE t81_tui ftxui::screen ftxui::dom)
    target_include_directories(tui_snapshot_test PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp
    )
    target_compile_features(tui_snapshot_test PRIVATE ${T81_CXX_STD_FEATURE})
    add_test(NAME tui_snapshot_test COMMAND tui_snapshot_test)
  endif()
endif()

add_executable(t81 tooling/cli/main.cpp)
target_link_libraries(t81 PRIVATE t81_lang_frontend t81_isa t81_vm t81_tool_cli)
target_include_directories(t81 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)
if(T81_ENABLE_LLAMA_CPP)
  target_link_libraries(t81 PRIVATE t81_llama_adapter)
  target_compile_definitions(t81 PRIVATE T81_HAS_LLAMA_CPP=1)
endif()
if(T81_BUILD_TUI)
  target_link_libraries(t81 PRIVATE t81_tui)
  target_compile_definitions(t81 PRIVATE T81_HAS_TUI=1)
endif()

if(T81_BUILD_EXAMPLES)
add_executable(t81_demo examples/demo.cpp)
target_link_libraries(t81_demo PRIVATE t81_core)

add_executable(t81_tensor_ops examples/tensor_ops.cpp)
target_link_libraries(t81_tensor_ops PRIVATE t81_core)

add_executable(t81_ir_roundtrip examples/ir_roundtrip.cpp)
target_link_libraries(t81_ir_roundtrip PRIVATE t81_core)

add_executable(axion_demo examples/axion_demo.cpp)
target_link_libraries(axion_demo PRIVATE t81_core t81_axion)

add_executable(axion_guard_trace examples/axion_guard_trace.cpp)
target_link_libraries(axion_guard_trace PRIVATE t81_tool_cli t81_vm)

add_executable(axion_policy_trace examples/axion_policy_trace.cpp)
target_link_libraries(axion_policy_trace PRIVATE t81_tool_cli t81_vm)

add_executable(axion_policy_runner examples/axion_policy_runner.cpp)
target_link_libraries(axion_policy_runner PRIVATE t81_tool_cli t81_vm)

add_executable(llama32_demo examples/llama32_demo.cpp)
target_link_libraries(llama32_demo PRIVATE t81_tool_cli t81_vm)

if(T81_ENABLE_LLAMA_CPP)
add_executable(llama_cpp_governed_demo examples/llama_cpp_governed_demo.cpp)
target_link_libraries(llama_cpp_governed_demo PRIVATE t81_llama_adapter)
endif()

add_executable(canonfs_trace_demo examples/canonfs_trace_demo.cpp)
target_link_libraries(canonfs_trace_demo PRIVATE t81_core t81_axion)

add_executable(complex_demo examples/complex_demo.cpp)
target_link_libraries(complex_demo PRIVATE t81_core asio)
if(WIN32)
  target_link_libraries(complex_demo PRIVATE ws2_32 mswsock)
endif()

endif()

# RFC-0032 Phase 5 (C-07) — promoted AI CLI tool (no sleep_for / no llama.cpp)
add_executable(t81_ai_cli tooling/cli/ai/t81_ai_cli.cpp)
target_link_libraries(t81_ai_cli PRIVATE t81_vm t81_axion t81_math_quantization t81_core)

# =============================================================================
# All tests — exactly as you wrote them
# =============================================================================
if(T81_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
  )
  FetchContent_MakeAvailable(asio)
  # Asio does not provide a CMake target; expose a local interface target.
  if(NOT TARGET asio)
    add_library(asio INTERFACE)
    target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
    target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
  endif()

  add_executable(t81_bigint_test tests/cpp/bigint_roundtrip.cpp)
  target_link_libraries(t81_bigint_test PRIVATE t81_core)

  add_executable(t81_fraction_test tests/cpp/fraction_roundtrip.cpp)
  target_link_libraries(t81_fraction_test PRIVATE t81_core)

  add_executable(t81_bigint_properties_test tests/cpp/bigint_properties_test.cpp)
  target_link_libraries(t81_bigint_properties_test PRIVATE t81_core)

  add_executable(t81_prob_properties_test tests/cpp/prob_properties_test.cpp)
  target_link_libraries(t81_prob_properties_test PRIVATE t81_core)


  add_executable(t81_t81int_properties_test tests/cpp/t81int_properties_test.cpp)
  target_link_libraries(t81_t81int_properties_test PRIVATE t81_core)

  add_executable(t81_bigint_division_edge_properties_test tests/cpp/bigint_division_edge_properties_test.cpp)
  target_link_libraries(t81_bigint_division_edge_properties_test PRIVATE t81_core)

  add_executable(t81_bigint_division_semantics_test tests/cpp/bigint_division_semantics_test.cpp)
  target_link_libraries(t81_bigint_division_semantics_test PRIVATE t81_core)

  add_executable(t81_bigint_gcd_divmod_property_test tests/cpp/bigint_gcd_divmod_property_test.cpp)
  target_link_libraries(t81_bigint_gcd_divmod_property_test PRIVATE t81_core)

  add_executable(t81_bigint_mod_test tests/cpp/test_T81BigInt_mod.cpp)
  target_link_libraries(t81_bigint_mod_test PRIVATE t81_core)

  # Consolidated modular inverse test (ExtGCD + Stein, shared fixtures)
  add_executable(t81_bigint_modular_inverse_test tests/cpp/bigint_modular_inverse_test.cpp)
  target_link_libraries(t81_bigint_modular_inverse_test PRIVATE t81_core)

  add_executable(t81_bigint_sqrt_test tests/cpp/test_T81BigInt_sqrt.cpp)
  target_link_libraries(t81_bigint_sqrt_test PRIVATE t81_core)

  add_executable(fuzz_bigint_div tests/cpp/fuzz_bigint_div.cpp)
  target_link_libraries(fuzz_bigint_div PRIVATE t81_core)

  add_executable(fuzz_bigint_libfuzzer tests/cpp/fuzz_bigint_libfuzzer.cpp)
  target_link_libraries(fuzz_bigint_libfuzzer PRIVATE t81_core)

  add_executable(t81_fraction_properties_test tests/cpp/fraction_properties_test.cpp)
  target_link_libraries(t81_fraction_properties_test PRIVATE t81_core)

  add_executable(t81_core_numeric_compat_test tests/cpp/core_numeric_compat_test.cpp)
  target_link_libraries(t81_core_numeric_compat_test PRIVATE t81_core)

  add_executable(t81_core_bigint_compat_properties_test tests/cpp/core_bigint_compat_properties_test.cpp)
  target_link_libraries(t81_core_bigint_compat_properties_test PRIVATE t81_core)

  add_executable(t81_core_fraction_compat_properties_test tests/cpp/core_fraction_compat_properties_test.cpp)
  target_link_libraries(t81_core_fraction_compat_properties_test PRIVATE t81_core)

  add_executable(t81_v1_canonical_numeric_contract_test tests/cpp/v1_canonical_numeric_contract_test.cpp)
  target_link_libraries(t81_v1_canonical_numeric_contract_test PRIVATE t81_core)

  add_executable(t81_tensor_transpose_test tests/cpp/tensor_transpose_test.cpp)
  target_link_libraries(t81_tensor_transpose_test PRIVATE t81_core)

  add_executable(t81_core_improvements_test tests/cpp/test_core_improvements.cpp)
  target_link_libraries(t81_core_improvements_test PRIVATE t81_core)

  add_executable(t81_core_features_test tests/cpp/test_core_features.cpp)
  target_link_libraries(t81_core_features_test PRIVATE t81_core)

  add_executable(t81_tensor_slice_test tests/cpp/tensor_slice_test.cpp)
  target_link_libraries(t81_tensor_slice_test PRIVATE t81_core)

  add_executable(t81_tensor_reshape_test tests/cpp/tensor_reshape_test.cpp)
  target_link_libraries(t81_tensor_reshape_test PRIVATE t81_core)

  add_executable(t81_tensor_loader_test tests/cpp/tensor_loader_test.cpp)
  target_link_libraries(t81_tensor_loader_test PRIVATE t81_io)

  add_executable(t81_weights_quantize_test tests/cpp/weights_quantize_test.cpp)
  target_link_libraries(t81_weights_quantize_test PRIVATE t81_core t81_tool_model)

  add_executable(t81_canonfs_io_test tests/cpp/canonfs_io_test.cpp)
  target_link_libraries(t81_canonfs_io_test PRIVATE t81_core)

  add_executable(canonfs_axion_trace_test tests/cpp/canonfs_axion_trace_test.cpp)
  target_link_libraries(canonfs_axion_trace_test PRIVATE t81_core t81_axion)

  add_executable(canonfs_persistent_driver_test tests/cpp/canonfs_persistent_driver_test.cpp)
  target_link_libraries(canonfs_persistent_driver_test PRIVATE t81_core t81_axion)

  add_executable(canonfs_read_verify_test tests/cpp/canonfs_read_verify_test.cpp)
  target_link_libraries(canonfs_read_verify_test PRIVATE t81_core t81_axion)

  add_executable(canonfs_read_verify_env_contract_test
                 tests/cpp/canonfs_read_verify_env_contract_test.cpp)
  target_link_libraries(canonfs_read_verify_env_contract_test PRIVATE t81_core t81_axion)

  add_executable(canonfs_integrity_matrix_test tests/cpp/canonfs_integrity_matrix_test.cpp)
  target_link_libraries(canonfs_integrity_matrix_test PRIVATE t81_core t81_axion)

  add_executable(t81_ir_encoding_test tests/cpp/ir_encoding_test.cpp)
  target_link_libraries(t81_ir_encoding_test PRIVATE t81_core)

  add_executable(t81_isa_encoding_test tests/cpp/tisc_encoding_test.cpp)
  target_link_libraries(t81_isa_encoding_test PRIVATE t81_core)

  add_executable(t81_vm_tensor_test tests/cpp/vm_tensor_test.cpp)
  target_link_libraries(t81_vm_tensor_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_tloadhash_conformance_test tests/cpp/vm_tloadhash_conformance_test.cpp)
  target_link_libraries(t81_vm_tloadhash_conformance_test PRIVATE t81_vm t81_core)


  add_executable(t81_vm_tloadhash_canonical_fixed_test
                 tests/cpp/vm_tloadhash_canonical_fixed_test.cpp)
  target_link_libraries(t81_vm_tloadhash_canonical_fixed_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_canonfs_root_env_contract_test
                 tests/cpp/vm_canonfs_root_env_contract_test.cpp)
  target_link_libraries(t81_vm_canonfs_root_env_contract_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_tloadhash_decodefault_determinism_matrix_test
                 tests/cpp/vm_tloadhash_decodefault_determinism_matrix_test.cpp)
  target_link_libraries(t81_vm_tloadhash_decodefault_determinism_matrix_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_ai_phase1_attention_conformance_test
                 tests/cpp/vm_ai_phase1_attention_conformance_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_attention_conformance_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_ai_phase1_embed_conformance_test
                 tests/cpp/vm_ai_phase1_embed_conformance_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_embed_conformance_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_ai_phase1_qmatmul_conformance_test
                 tests/cpp/vm_ai_phase1_qmatmul_conformance_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_qmatmul_conformance_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_ai_phase1_shared_helper_parity_test
                 tests/cpp/vm_ai_phase1_shared_helper_parity_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_shared_helper_parity_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_ai_phase1_wload_conformance_test
                 tests/cpp/vm_ai_phase1_wload_conformance_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_wload_conformance_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_ai_phase1_gather_conformance_test
                 tests/cpp/vm_ai_phase1_gather_conformance_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_gather_conformance_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_ai_phase1_scatter_conformance_test
                 tests/cpp/vm_ai_phase1_scatter_conformance_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_scatter_conformance_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_ai_phase1_wload_canonfs_audit_test
                 tests/cpp/vm_ai_phase1_wload_canonfs_audit_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_wload_canonfs_audit_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_ai_phase1_scatter_aliasing_test
                 tests/cpp/vm_ai_phase1_scatter_aliasing_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_scatter_aliasing_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_ai_phase1_gather_axis1_test
                 tests/cpp/vm_ai_phase1_gather_axis1_test.cpp)
  target_link_libraries(t81_vm_ai_phase1_gather_axis1_test PRIVATE t81_vm t81_core)
  add_executable(t81_vm_rfc0034_ternary_native_test
                 tests/cpp/vm_rfc0034_ternary_native_test.cpp)
  target_link_libraries(t81_vm_rfc0034_ternary_native_test PRIVATE t81_vm t81_core)
  add_executable(t81_lang_ai_m6_annotation_lowering_test
                 tests/cpp/lang_ai_m6_annotation_lowering_test.cpp)
  target_link_libraries(t81_lang_ai_m6_annotation_lowering_test
                        PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(t81_vm_tensor_shape_faults_test tests/cpp/vm_tensor_shape_faults_test.cpp)
  target_link_libraries(t81_vm_tensor_shape_faults_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_tisc_v04_extensions_test tests/cpp/vm_tisc_v04_extensions_test.cpp)
  target_link_libraries(t81_vm_tisc_v04_extensions_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_tensor_get_set_conformance_test
                 tests/cpp/vm_tensor_get_set_conformance_test.cpp)
  target_link_libraries(t81_vm_tensor_get_set_conformance_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_tensor_helper_predicates_test
                 tests/cpp/vm_tensor_helper_predicates_test.cpp)
  target_link_libraries(t81_vm_tensor_helper_predicates_test PRIVATE t81_vm t81_core)
  target_include_directories(t81_vm_tensor_helper_predicates_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  add_executable(t81_vm_tensor_provenance_trace_test
                 tests/cpp/vm_tensor_provenance_trace_test.cpp)
  target_link_libraries(t81_vm_tensor_provenance_trace_test PRIVATE t81_vm t81_core)
  target_include_directories(t81_vm_tensor_provenance_trace_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  add_executable(t81_vm_predispatch_policy_deny_logging_test
                 tests/cpp/vm_predispatch_policy_deny_logging_test.cpp)
  target_link_libraries(t81_vm_predispatch_policy_deny_logging_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_fault_test tests/cpp/vm_fault_test.cpp)
  target_link_libraries(t81_vm_fault_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_deterministic_fault_test tests/cpp/test_vm_deterministic_fault.cpp)
  target_link_libraries(t81_vm_deterministic_fault_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_fault_family_determinism_matrix_test
                 tests/cpp/vm_fault_family_determinism_matrix_test.cpp)
  target_link_libraries(t81_vm_fault_family_determinism_matrix_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_new_opcodes_test tests/cpp/vm_new_opcodes_test.cpp)
  target_link_libraries(t81_vm_new_opcodes_test PRIVATE t81_vm t81_core)

  add_executable(t81_test_axion_opcodes tests/cpp/test_axion_opcodes.cpp)
  target_link_libraries(t81_test_axion_opcodes PRIVATE t81_vm t81_core)
  add_test(NAME t81_test_axion_opcodes COMMAND t81_test_axion_opcodes)

  add_executable(t81_test_axion_m6_canonical_reason tests/cpp/test_axion_m6_canonical_reason.cpp)
  target_link_libraries(t81_test_axion_m6_canonical_reason PRIVATE t81_core)
  add_test(NAME t81_test_axion_m6_canonical_reason COMMAND t81_test_axion_m6_canonical_reason)

  add_executable(t81_vm_neural_opcodes_test tests/cpp/test_vm_neural_opcodes.cpp)
  target_link_libraries(t81_vm_neural_opcodes_test PRIVATE t81_vm t81_core)
  add_test(NAME t81_vm_neural_opcodes_test COMMAND t81_vm_neural_opcodes_test)

  add_executable(test_bitwise tests/cpp/test_bitwise.cpp)
  target_link_libraries(test_bitwise PRIVATE t81_vm t81_core)
  add_test(NAME test_bitwise COMMAND test_bitwise)

  add_executable(t81_vm_memory_test tests/cpp/vm_memory_test.cpp)
  target_link_libraries(t81_vm_memory_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_resource_monitoring_test tests/cpp/test_resource_monitoring.cpp)
  target_link_libraries(t81_vm_resource_monitoring_test PRIVATE t81_vm t81_core)
  add_test(NAME t81_vm_resource_monitoring_test COMMAND t81_vm_resource_monitoring_test)

  add_executable(t81_axion_log_determinism_test tests/cpp/axion_log_determinism_test.cpp)
  target_link_libraries(t81_axion_log_determinism_test PRIVATE t81_vm t81_core)

  add_executable(t81_reflection_non_interference_test tests/cpp/reflection_non_interference_test.cpp)
  target_link_libraries(t81_reflection_non_interference_test PRIVATE t81_vm t81_core)

  add_executable(t81_weights_load_test tests/cpp/weights_load_test.cpp)
  target_link_libraries(t81_weights_load_test PRIVATE t81_vm t81_core t81_tool_model)

  add_executable(t81_hash_stub_test tests/cpp/hash_stub_test.cpp)
  target_link_libraries(t81_hash_stub_test PRIVATE t81_core)

  add_executable(t81_sha3_test tests/cpp/test_sha3.cpp)
  target_link_libraries(t81_sha3_test PRIVATE t81_core)

  add_executable(t81_axion_stub_test tests/cpp/axion_stub_test.cpp)
  target_link_libraries(t81_axion_stub_test PRIVATE t81_core t81_axion)

  add_executable(t81_check_tier_test tests/cpp/test_check_tier.cpp)
  target_link_libraries(t81_check_tier_test PRIVATE t81_core t81_axion)

  add_executable(t81_codec_base243_test tests/cpp/codec_base243_test.cpp)
  target_link_libraries(t81_codec_base243_test PRIVATE t81_core)
  add_executable(t81_codec_base81_test tests/cpp/codec_base81_test.cpp)
  target_link_libraries(t81_codec_base81_test PRIVATE t81_core)

  add_executable(t81_trit_packing_test tests/cpp/trit_packing_test.cpp)
  target_link_libraries(t81_trit_packing_test PRIVATE t81_core)

  add_executable(t81_packed_trit_vector_test tests/cpp/test_packed_trit_vector.cpp)
  target_link_libraries(t81_packed_trit_vector_test PRIVATE t81_core)
  if(T81_IS_X86_64 AND NOT MSVC)
    target_compile_options(t81_packed_trit_vector_test PRIVATE -mavx2 -mfma)
  endif()

  add_executable(t81_tritwise_backend_equivalence_test tests/cpp/test_tritwise_backend_equivalence.cpp)
  target_link_libraries(t81_tritwise_backend_equivalence_test PRIVATE t81_core)
  if(T81_IS_X86_64 AND NOT MSVC)
    target_compile_options(t81_tritwise_backend_equivalence_test PRIVATE -mavx2 -mfma)
  endif()
  add_test(NAME t81_tritwise_backend_equivalence_test COMMAND t81_tritwise_backend_equivalence_test)

  add_executable(t81_phase2c_truth_table_test tests/cpp/test_phase2c_truth_table.cpp)
  target_link_libraries(t81_phase2c_truth_table_test PRIVATE t81_core)

  add_executable(t81_packed_trit_vector_stress_test tests/cpp/test_packed_trit_vector_stress.cpp)
  target_link_libraries(t81_packed_trit_vector_stress_test PRIVATE t81_core)

  add_executable(t81_base81_balanced_test tests/cpp/test_base81_balanced.cpp)
  target_link_libraries(t81_base81_balanced_test PRIVATE t81_core)

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    add_executable(t81_base81_simd_test tests/cpp/test_base81_simd.cpp)
    target_link_libraries(t81_base81_simd_test PRIVATE t81_core)
  endif()

  add_executable(t81_base81_packed_test tests/cpp/test_base81_packed.cpp)
  target_link_libraries(t81_base81_packed_test PRIVATE t81_core)

  add_executable(t81_metadata_test tests/cpp/test_metadata.cpp)
  target_link_libraries(t81_metadata_test PRIVATE t81_core)

  add_executable(t81_isa_base81_view_test tests/cpp/test_tisc_base81_view.cpp)
  target_link_libraries(t81_isa_base81_view_test PRIVATE t81_isa)

  add_executable(tisc_base81 tools/tisc_base81.cpp)
  target_link_libraries(tisc_base81 PRIVATE t81_isa)

  add_executable(setun_bridge_test tests/cpp/setun_bridge_test.cpp)
  target_link_libraries(setun_bridge_test PRIVATE t81_core)

  add_executable(tisc_opcode_matrix_test tests/cpp/tisc_opcode_matrix_test.cpp)
  target_link_libraries(tisc_opcode_matrix_test PRIVATE t81_isa)

  add_executable(t81lang_conformance_baseline_test tests/cpp/t81lang_conformance_baseline_test.cpp)
add_executable(t81lang_surface_gate_test tests/cpp/t81lang_surface_gate_test.cpp)
target_link_libraries(t81lang_surface_gate_test PRIVATE t81_lang_frontend t81_core)
add_test(NAME t81lang_surface_gate_test COMMAND t81lang_surface_gate_test)

  target_link_libraries(t81lang_conformance_baseline_test PRIVATE t81_lang_frontend)

  add_executable(t81lang_conformance_edge_semantics_test
                 tests/cpp/t81lang_conformance_edge_semantics_test.cpp)
  target_link_libraries(t81lang_conformance_edge_semantics_test PRIVATE t81_lang_frontend)
  add_test(NAME t81lang_conformance_edge_semantics_test
           COMMAND t81lang_conformance_edge_semantics_test)

  add_executable(t81_numeric_format_test tests/cpp/t81_numeric_format_test.cpp)
  target_link_libraries(t81_numeric_format_test PRIVATE t81_core)

  add_executable(axion_recursion_guardrails_test tests/cpp/axion_recursion_guardrails_test.cpp)
  target_link_libraries(axion_recursion_guardrails_test PRIVATE t81_vm t81_core)

  add_executable(t81_tensor_shape_test tests/cpp/tensor_shape_test.cpp)
  target_link_libraries(t81_tensor_shape_test PRIVATE t81_core)

  add_executable(t81_tensor_serialization_canonical_fixed_test
                 tests/cpp/tensor_serialization_canonical_fixed_test.cpp)
  target_link_libraries(t81_tensor_serialization_canonical_fixed_test PRIVATE t81_core)

  add_executable(t81_tensor_native_decode_test tests/cpp/tensor_native_decode_test.cpp)
  target_link_libraries(t81_tensor_native_decode_test PRIVATE t81_core)

  add_executable(t81_ternary_arith_test tests/cpp/ternary_arith_test.cpp)
  target_link_libraries(t81_ternary_arith_test PRIVATE t81_core)

  add_executable(t81_tensor_matmul_test tests/cpp/tensor_matmul_test.cpp)
  target_link_libraries(t81_tensor_matmul_test PRIVATE t81_core)

  add_executable(t81_tensor_conv_test tests/cpp/test_T81Tensor_conv.cpp)
  target_link_libraries(t81_tensor_conv_test PRIVATE t81_core)

  add_executable(t81_tensor_nn_test tests/cpp/test_T81Tensor_nn.cpp)
  target_link_libraries(t81_tensor_nn_test PRIVATE t81_core)

  add_executable(t81_tensor_reduce_test tests/cpp/tensor_reduce_test.cpp)
  target_link_libraries(t81_tensor_reduce_test PRIVATE t81_core)

  add_executable(t81_tensor_broadcast_test tests/cpp/tensor_broadcast_test.cpp)
  target_link_libraries(t81_tensor_broadcast_test PRIVATE t81_core)

  add_executable(t81_entropy_test tests/cpp/entropy_test.cpp)
  target_link_libraries(t81_entropy_test PRIVATE t81_core)

  add_executable(t81_ethics_test tests/cpp/test_ethics.cpp)
  target_link_libraries(t81_ethics_test PRIVATE t81_core t81_axion)
  add_test(NAME t81_ethics_test COMMAND t81_ethics_test)

  add_executable(t81_ethics_invariants_test tests/cpp/test_ethics_invariants.cpp)
  target_link_libraries(t81_ethics_invariants_test PRIVATE t81_core t81_axion)
  add_test(NAME t81_ethics_invariants_test COMMAND t81_ethics_invariants_test)

  add_executable(test_resource_monitoring tests/cpp/test_resource_monitoring.cpp)
  target_link_libraries(test_resource_monitoring PRIVATE t81_core t81_isa t81_vm)
  add_test(NAME test_resource_monitoring COMMAND test_resource_monitoring)

  add_executable(test_tier3_opcodes tests/cpp/test_tier3_opcodes.cpp)
  target_link_libraries(test_tier3_opcodes PRIVATE t81_core t81_isa t81_vm)
  add_test(NAME test_tier3_opcodes COMMAND test_tier3_opcodes)

  add_executable(t81_expected_test tests/cpp/expected_test.cpp)
  target_link_libraries(t81_expected_test PRIVATE t81_core)

  add_executable(t81_tensor_elementwise_test tests/cpp/tensor_elementwise_test.cpp)
  target_link_libraries(t81_tensor_elementwise_test PRIVATE t81_core)

  add_executable(t81_tensor_ternary_test tests/cpp/tensor_ternary_test.cpp)
  target_link_libraries(t81_tensor_ternary_test PRIVATE t81_core)

  add_executable(t81_t729_tensor_test tests/cpp/test_t729_tensor.cpp)
  target_link_libraries(t81_t729_tensor_test PRIVATE t81_core)

  add_executable(t81_symbolic_algebra_test tests/cpp/test_symbolic_algebra.cpp)
  target_link_libraries(t81_symbolic_algebra_test PRIVATE t81_core)

  add_executable(t81_semantic_graph_test tests/cpp/test_semantic_graph.cpp)
  target_link_libraries(t81_semantic_graph_test PRIVATE t81_core)

  add_executable(t81_neural_primitives_test tests/cpp/test_neural_primitives.cpp)
  target_link_libraries(t81_neural_primitives_test PRIVATE t81_core)

  add_executable(t81_frontend_neural_test tests/cpp/test_frontend_neural.cpp)
  target_link_libraries(t81_frontend_neural_test PRIVATE t81_lang_frontend t81_core)

  add_executable(t81_c_api_bigint_test tests/cpp/c_api_bigint_test.c)
  target_link_libraries(t81_c_api_bigint_test PRIVATE t81_c_api)
  target_include_directories(t81_c_api_bigint_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  add_executable(t81_tensor_unary_test tests/cpp/tensor_unary_test.cpp)
  target_link_libraries(t81_tensor_unary_test PRIVATE t81_core)

  add_executable(t81_hanoi_integration_test tests/cpp/hanoi_integration_test.cpp)
  target_link_libraries(t81_hanoi_integration_test PRIVATE t81_core t81_axion)

  add_executable(t81_vm_load_store_test tests/cpp/vm_load_store_test.cpp)
  target_link_libraries(t81_vm_load_store_test PRIVATE t81_vm t81_core)

  
  add_executable(t81_vm_illegal_test tests/cpp/vm_illegal_test.cpp)
  target_link_libraries(t81_vm_illegal_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_bounds_test tests/cpp/vm_bounds_test.cpp)
  target_link_libraries(t81_vm_bounds_test PRIVATE t81_vm t81_core)

  add_executable(vm_bounds_trace_test tests/cpp/vm_bounds_trace_test.cpp)
  target_link_libraries(vm_bounds_trace_test PRIVATE t81_vm t81_core)
  add_test(NAME vm_bounds_trace_test COMMAND vm_bounds_trace_test)

  add_executable(t81_canonfs_driver_test tests/cpp/canonfs_driver_test.cpp)
  target_link_libraries(t81_canonfs_driver_test PRIVATE t81_core)

  add_executable(t81_canonfs_rs_repair_test tests/cpp/canonfs_rs_repair_test.cpp)
  target_link_libraries(t81_canonfs_rs_repair_test PRIVATE t81_core)

  # RFC-0000 §1: CanonHash-81 reference vector tests.
  add_executable(t81_canonhash81_reference_vectors_test
                 tests/cpp/canonhash81_reference_vectors_test.cpp)
  target_link_libraries(t81_canonhash81_reference_vectors_test PRIVATE t81_core)

  add_executable(t81_vm_trace_test tests/cpp/vm_trace_test.cpp)
  target_link_libraries(t81_vm_trace_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_determinism_property_test tests/cpp/vm_determinism_property_test.cpp)
  target_link_libraries(t81_vm_determinism_property_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_state_transition_invariants_test
                 tests/cpp/vm_state_transition_invariants_test.cpp)
  target_link_libraries(t81_vm_state_transition_invariants_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_state_transition_conformance_matrix_test
                 tests/cpp/vm_state_transition_conformance_matrix_test.cpp)
  target_link_libraries(t81_vm_state_transition_conformance_matrix_test PRIVATE t81_vm t81_core)

  # vm_workload_determinism_test absorbed into vm_workload_determinism_tiers_test as "workload" tier

  add_executable(t81_vm_workload_determinism_tiers_test
                 tests/cpp/vm_workload_determinism_tiers_test.cpp)
  target_link_libraries(t81_vm_workload_determinism_tiers_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_mixed_workload_conformance_matrix_test
                 tests/cpp/vm_mixed_workload_conformance_matrix_test.cpp)
  target_link_libraries(t81_vm_mixed_workload_conformance_matrix_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_policy_parse_fail_closed_test
                 tests/cpp/vm_policy_parse_fail_closed_test.cpp)
  target_link_libraries(t81_vm_policy_parse_fail_closed_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_axreport_policy_deny_fail_closed_test
                 tests/cpp/vm_axreport_policy_deny_fail_closed_test.cpp)
  target_link_libraries(t81_vm_axreport_policy_deny_fail_closed_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_system_registers_deterministic_test
                 tests/cpp/vm_system_registers_deterministic_test.cpp)
  target_link_libraries(t81_vm_system_registers_deterministic_test PRIVATE t81_vm t81_core)

  # Consolidated fail-closed stub harness (merges async/network + privileged opcode tests)
  add_executable(t81_vm_stubbed_opcode_fail_closed_test
                 tests/cpp/vm_stubbed_opcode_fail_closed_test.cpp)
  target_link_libraries(t81_vm_stubbed_opcode_fail_closed_test PRIVATE t81_vm t81_core)

  add_executable(vm_extended_ops_test tests/cpp/vm_extended_ops_test.cpp)
  target_link_libraries(vm_extended_ops_test PRIVATE t81_vm t81_core)

  add_executable(llama_kernels_test tests/cpp/llama_kernels_test.cpp)
  target_link_libraries(llama_kernels_test PRIVATE t81_core)

  add_executable(t81_vm_jump_flags_test tests/cpp/vm_jump_flags_test.cpp)
  target_link_libraries(t81_vm_jump_flags_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_float_fraction_ops_test tests/cpp/vm_float_fraction_ops_test.cpp)
  target_link_libraries(t81_vm_float_fraction_ops_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_literal_pool_extension_test tests/cpp/test_vm_literal_pool_extension.cpp)
  target_link_libraries(t81_vm_literal_pool_extension_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_neg_jumps_test tests/cpp/vm_neg_jumps_test.cpp)
  target_link_libraries(t81_vm_neg_jumps_test PRIVATE t81_vm t81_core)

  add_executable(t81_vm_print_test tests/cpp/vm_print_test.cpp)
  target_link_libraries(t81_vm_print_test PRIVATE t81_vm t81_core)

  add_executable(t81_frontend_lexer_test tests/cpp/frontend_lexer_test.cpp)
  target_link_libraries(t81_frontend_lexer_test PRIVATE t81_lang_frontend)

  add_executable(t81_frontend_parser_test tests/cpp/frontend_parser_test.cpp)
  target_link_libraries(t81_frontend_parser_test PRIVATE t81_lang_frontend)

  add_executable(t81_parser_regression_audit_test tests/cpp/test_parser_regression_audit.cpp)
  target_link_libraries(t81_parser_regression_audit_test PRIVATE t81_lang_frontend)
  add_test(NAME t81_parser_regression_audit_test COMMAND t81_parser_regression_audit_test)

  add_executable(t81_spec_compliance_test tests/cpp/spec_compliance_test.cpp)
  target_link_libraries(t81_spec_compliance_test PRIVATE t81_lang_frontend)

  add_executable(tisc_semantic_equivalence_test tests/cpp/tisc_semantic_equivalence_test.cpp)
  target_link_libraries(tisc_semantic_equivalence_test PRIVATE t81_vm t81_core)

  add_executable(tisc_opcode_family_semantics_test tests/cpp/tisc_opcode_family_semantics_test.cpp)
  target_link_libraries(tisc_opcode_family_semantics_test PRIVATE t81_vm t81_core)

  add_executable(t81_frontend_expression_features_test tests/cpp/frontend_expression_features_test.cpp)
  target_link_libraries(t81_frontend_expression_features_test PRIVATE t81_lang_frontend t81_isa)

  add_executable(t81_frontend_parser_generics_test tests/cpp/frontend_parser_generics_test.cpp)
  target_link_libraries(t81_frontend_parser_generics_test PRIVATE t81_lang_frontend)

  add_executable(t81_frontend_parser_appendix_coverage_test
                 tests/cpp/frontend_parser_appendix_coverage_test.cpp)
  target_link_libraries(t81_frontend_parser_appendix_coverage_test PRIVATE t81_lang_frontend)
  add_test(NAME t81_frontend_parser_appendix_coverage_test
           COMMAND t81_frontend_parser_appendix_coverage_test)

  add_executable(t81_frontend_parser_recovery_test tests/cpp/frontend_parser_recovery_test.cpp)
  target_link_libraries(t81_frontend_parser_recovery_test PRIVATE t81_lang_frontend)

  add_executable(t81_frontend_parser_legacy_rejection_test tests/cpp/frontend_parser_legacy_rejection_test.cpp)
  target_link_libraries(t81_frontend_parser_legacy_rejection_test PRIVATE t81_lang_frontend)

  add_executable(t81_frontend_ir_generator_test tests/cpp/frontend_ir_generator_test.cpp)
  target_link_libraries(t81_frontend_ir_generator_test PRIVATE t81_lang_frontend t81_isa)

  add_executable(t81_frontend_logical_lowering_test tests/cpp/test_frontend_logical_lowering.cpp)
  target_link_libraries(t81_frontend_logical_lowering_test PRIVATE t81_lang_frontend t81_isa)

  add_executable(t81_ir_snapshot_audit_test tests/cpp/test_ir_snapshot_audit.cpp)
  target_link_libraries(t81_ir_snapshot_audit_test PRIVATE t81_lang_frontend t81_isa)
  add_test(NAME t81_ir_snapshot_audit_test COMMAND t81_ir_snapshot_audit_test)

  add_executable(t81_semantic_analyzer_option_result_test tests/cpp/semantic_analyzer_option_result_test.cpp)
  target_link_libraries(t81_semantic_analyzer_option_result_test PRIVATE t81_lang_frontend)

  add_executable(test_std_option_result tests/cpp/test_std_option_result.cpp)
  target_link_libraries(test_std_option_result PRIVATE t81_lang_frontend t81_isa)

  add_executable(t81_semantic_analyzer_match_test tests/cpp/semantic_analyzer_match_test.cpp)
  target_link_libraries(t81_semantic_analyzer_match_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_generic_test tests/cpp/semantic_analyzer_generic_test.cpp)
  target_link_libraries(t81_semantic_analyzer_generic_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_indexing_test tests/cpp/semantic_analyzer_indexing_test.cpp)
  target_link_libraries(t81_semantic_analyzer_indexing_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_vector_literal_test tests/cpp/semantic_analyzer_vector_literal_test.cpp)
  target_link_libraries(t81_semantic_analyzer_vector_literal_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_record_enum_test tests/cpp/semantic_analyzer_record_enum_test.cpp)
  target_link_libraries(t81_semantic_analyzer_record_enum_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_equality_test tests/cpp/semantic_analyzer_equality_test.cpp)
  target_link_libraries(t81_semantic_analyzer_equality_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_numeric_test tests/cpp/semantic_analyzer_numeric_test.cpp)
  target_link_libraries(t81_semantic_analyzer_numeric_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_stage3_rules_test
                 tests/cpp/semantic_analyzer_stage3_rules_test.cpp)
  target_link_libraries(t81_semantic_analyzer_stage3_rules_test PRIVATE t81_lang_frontend)
  add_test(NAME t81_semantic_analyzer_stage3_rules_test
           COMMAND t81_semantic_analyzer_stage3_rules_test)

  add_executable(t81_semantic_analyzer_diagnostic_location_test tests/cpp/semantic_analyzer_diagnostic_location_test.cpp)
  target_link_libraries(t81_semantic_analyzer_diagnostic_location_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_cascade_suppression_test tests/cpp/semantic_analyzer_cascade_suppression_test.cpp)
  target_link_libraries(t81_semantic_analyzer_cascade_suppression_test PRIVATE t81_lang_frontend)

  add_executable(t81_semantic_analyzer_diagnostic_precision_test tests/cpp/semantic_analyzer_diagnostic_precision_test.cpp)
  target_link_libraries(t81_semantic_analyzer_diagnostic_precision_test PRIVATE t81_lang_frontend)

  add_executable(t81_isa_binary_emitter_test tests/cpp/tisc_binary_emitter_test.cpp)
  target_link_libraries(t81_isa_binary_emitter_test PRIVATE t81_isa)

  add_executable(t81_isa_type_alias_io_test tests/cpp/tisc_type_alias_io_test.cpp)
  target_link_libraries(t81_isa_type_alias_io_test PRIVATE t81_isa)

  add_executable(t81_isa_binary_io_determinism_test tests/cpp/tisc_binary_io_determinism_test.cpp)
  target_link_libraries(t81_isa_binary_io_determinism_test PRIVATE t81_isa t81_core)
  add_executable(t81_isa_binary_metadata_roundtrip_property_test tests/cpp/tisc_binary_metadata_roundtrip_property_test.cpp)
  target_link_libraries(t81_isa_binary_metadata_roundtrip_property_test PRIVATE t81_isa t81_core)

  add_executable(t81_semantic_analyzer_loop_test tests/cpp/semantic_analyzer_loop_test.cpp)
  target_link_libraries(t81_semantic_analyzer_loop_test PRIVATE t81_lang_frontend)

  add_executable(ir_inspector tools/ir_inspector.cpp)
  target_link_libraries(ir_inspector PRIVATE t81_lang_frontend)

  add_executable(e2e_let_statement_test tests/cpp/e2e_let_statement_test.cpp)
  target_link_libraries(e2e_let_statement_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_arithmetic_test tests/cpp/e2e_arithmetic_test.cpp)
  target_link_libraries(e2e_arithmetic_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_if_statement_test tests/cpp/e2e_if_statement_test.cpp)
  target_link_libraries(e2e_if_statement_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_option_result_test tests/cpp/e2e_option_result_test.cpp)
  target_link_libraries(e2e_option_result_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_option_type_test tests/cpp/e2e_option_type_test.cpp)
  target_link_libraries(e2e_option_type_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_option_result_function_test tests/cpp/e2e_option_result_function_test.cpp)
  target_link_libraries(e2e_option_result_function_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(t81_cli_check_test tests/cpp/cli_check_test.cpp)
  target_link_libraries(t81_cli_check_test PRIVATE t81_tool_cli)

  add_executable(security_package_init_test tests/cpp/security_package_init_test.cpp)
  target_link_libraries(security_package_init_test PRIVATE t81_tool_cli)

  add_executable(t81_cli_pipeline_test tests/cpp/cli_pipeline_test.cpp)
  target_link_libraries(t81_cli_pipeline_test PRIVATE t81_tool_cli)

  add_executable(t81_cli_disasm_test tests/cpp/cli_disasm_test.cpp)
  target_link_libraries(t81_cli_disasm_test PRIVATE t81_tool_cli)

  add_executable(t81_cli_debugger_test tests/cpp/cli_debugger_test.cpp)
  target_link_libraries(t81_cli_debugger_test PRIVATE t81_tool_cli t81_isa)

  add_executable(t81_cli_option_result_test tests/cpp/cli_option_result_test.cpp)
  target_link_libraries(t81_cli_option_result_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(t81_cli_diagnostic_context_test tests/cpp/cli_diagnostic_context_test.cpp)
  target_link_libraries(t81_cli_diagnostic_context_test PRIVATE t81_tool_cli t81_lang_frontend t81_isa t81_vm)

  add_executable(t81_cli_trace_export_test tests/cpp/cli_trace_export_test.cpp)
  target_link_libraries(t81_cli_trace_export_test PRIVATE t81_tool_cli)

  # Skip on Windows because it relies on POSIX fork() and <sys/wait.h> which are unsupported.
  if(NOT WIN32)
    add_executable(t81_cli_contract_test tests/cpp/cli_contract_test.cpp)
    target_compile_features(t81_cli_contract_test PRIVATE ${T81_CXX_STD_FEATURE})
  endif()

  # Skip on Windows because it relies on POSIX fork() and <sys/wait.h> which are unsupported.
  if(NOT WIN32)
    add_executable(t81_cli_stress_test tests/cpp/cli_stress_test.cpp)
    target_compile_features(t81_cli_stress_test PRIVATE ${T81_CXX_STD_FEATURE})
  endif()

  if(T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY)
    add_executable(t81_c_frontend_mlir_smoke_test tests/cpp/c_frontend_mlir_smoke_test.cpp)
    target_link_libraries(t81_c_frontend_mlir_smoke_test PRIVATE t81_c_frontend)
  endif()
  if(T81_ENABLE_RUST_FRONTEND AND T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY AND T81_RUSTC_EXECUTABLE)
    add_executable(t81_rust_frontend_mlir_smoke_test tests/cpp/rust_frontend_mlir_smoke_test.cpp)
    target_link_libraries(t81_rust_frontend_mlir_smoke_test PRIVATE t81_rust_frontend)
  endif()
  if(T81_ENABLE_PYTHON_FRONTEND AND T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY AND Python3_EXECUTABLE)
    add_executable(t81_python_frontend_mlir_smoke_test tests/cpp/python_frontend_mlir_smoke_test.cpp)
    target_link_libraries(t81_python_frontend_mlir_smoke_test PRIVATE t81_python_frontend)
  endif()

  add_executable(t81_cli_structural_types_test tests/cpp/cli_structural_types_test.cpp)
  target_link_libraries(t81_cli_structural_types_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(t81_cli_repl_test tests/cpp/cli_repl_test.cpp)
  target_link_libraries(t81_cli_repl_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(cli_canonize_tensor_test tests/cpp/cli_canonize_tensor_test.cpp)
  target_link_libraries(cli_canonize_tensor_test PRIVATE t81_tool_cli t81_core)

  add_executable(cli_repro_hash_test tests/cpp/cli_repro_hash_test.cpp)
  target_compile_features(cli_repro_hash_test PRIVATE ${T81_CXX_STD_FEATURE})

  # Parameterized stdlib fixture harness (consolidates 10 per-module tests)
  add_executable(cli_stdlib_fixtures_test tests/cpp/cli_stdlib_fixtures_test.cpp)
  target_link_libraries(cli_stdlib_fixtures_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(cli_std_collections_determinism_test tests/cpp/cli_std_collections_determinism_test.cpp)
  target_link_libraries(cli_std_collections_determinism_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(t81_cli_record_enum_test tests/cpp/cli_record_enum_test.cpp)
  target_link_libraries(t81_cli_record_enum_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(t81_tier4_test tests/cpp/tier4_test.cpp)
  target_link_libraries(t81_tier4_test PRIVATE t81_core t81_axion)

  add_executable(t81_tier5_test tests/cpp/tier5_test.cpp)
  target_link_libraries(t81_tier5_test PRIVATE t81_core)

  add_executable(t81_infinite_opcodes_test tests/cpp/test_infinite_opcodes.cpp)
  target_link_libraries(t81_infinite_opcodes_test PRIVATE t81_vm t81_isa t81_core)

  add_executable(t81_tier4_vm_test tests/cpp/tier4_vm_test.cpp)
  target_link_libraries(t81_tier4_vm_test PRIVATE t81_vm t81_core)

  add_executable(t81_tier4_distributed_test tests/cpp/test_tier4_distributed.cpp)
  target_link_libraries(t81_tier4_distributed_test PRIVATE t81_vm t81_core)

  add_executable(tiers_structure_test tests/cpp/tiers_structure_test.cpp)
  target_link_libraries(tiers_structure_test PRIVATE t81_core)

  add_executable(vm_reflection_tier2_test tests/cpp/vm_reflection_tier2_test.cpp)
  target_link_libraries(vm_reflection_tier2_test PRIVATE t81_vm t81_isa t81_core)
  add_test(NAME vm_reflection_tier2_test COMMAND vm_reflection_tier2_test)
  add_executable(reflective_evidence_replay_test tests/cpp/reflective_evidence_replay_test.cpp)
  target_link_libraries(reflective_evidence_replay_test PRIVATE t81_core)
  add_test(NAME reflective_evidence_replay_test COMMAND reflective_evidence_replay_test)

  add_executable(t81_jit_test tests/cpp/jit_test.cpp)
  target_link_libraries(t81_jit_test PRIVATE t81_vm t81_core)

  add_executable(jit_trace_equivalence_test tests/cpp/jit_trace_equivalence_test.cpp)
  target_link_libraries(jit_trace_equivalence_test PRIVATE t81_vm)

  add_executable(jit_repro_oracle_test tests/cpp/jit_repro_oracle_test.cpp)
  target_link_libraries(jit_repro_oracle_test PRIVATE t81_vm t81_core)

  # RFC-0028 §4: CanonFS-backed trace cache acceptance tests.
  add_executable(jit_canonfs_cache_test tests/cpp/jit_canonfs_cache_test.cpp)
  target_link_libraries(jit_canonfs_cache_test PRIVATE t81_vm t81_core)

  add_executable(vm_gc_safepoint_test tests/cpp/vm_gc_safepoint_test.cpp)
  target_link_libraries(vm_gc_safepoint_test PRIVATE t81_vm t81_core)

  # RFC-0015 — First-Class Agent Constructs
  add_executable(agent_constructs_test tests/cpp/agent_constructs_test.cpp)
  target_link_libraries(agent_constructs_test PRIVATE t81_vm t81_lang_frontend t81_core)

  # RFC-0036 — Governed Foreign Function Interface (T81Lang `foreign {}` syntax)
  add_executable(t81lang_foreign_ffi_test tests/cpp/t81lang_foreign_ffi_test.cpp)
  target_link_libraries(t81lang_foreign_ffi_test PRIVATE t81_vm t81_lang_frontend t81_core)

  # RFC-0037 — TNN stdlib (std.tnn.* builtins → RFC-0034 TISC opcodes)
  add_executable(tnn_stdlib_test tests/cpp/tnn_stdlib_test.cpp)
  target_link_libraries(tnn_stdlib_test PRIVATE t81_vm t81_lang_frontend t81_core)

  # RFC-0038 — Ternary Lattice Cryptography (POLYMUL / POLYMOD)
  add_executable(lattice_crypto_test tests/cpp/lattice_crypto_test.cpp)
  target_link_libraries(lattice_crypto_test PRIVATE t81_vm t81_lang_frontend t81_core)

  add_executable(jit_tensor_trace_equivalence_test tests/cpp/jit_tensor_trace_equivalence_test.cpp)
  target_link_libraries(jit_tensor_trace_equivalence_test PRIVATE t81_vm)

  add_executable(vm_jit_per_instruction_policy_test tests/cpp/vm_jit_per_instruction_policy_test.cpp)
  target_link_libraries(vm_jit_per_instruction_policy_test PRIVATE t81_vm t81_core)

  add_executable(axion_loop_metadata_test tests/cpp/axion_loop_metadata_test.cpp)
  target_link_libraries(axion_loop_metadata_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(axion_match_metadata_test tests/cpp/axion_match_metadata_test.cpp)
  target_link_libraries(axion_match_metadata_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(axion_enum_guard_test tests/cpp/axion_enum_guard_test.cpp)
  target_link_libraries(axion_enum_guard_test PRIVATE t81_tool_cli t81_isa t81_vm)

add_executable(axion_policy_match_guard_test tests/cpp/axion_policy_match_guard_test.cpp)
target_link_libraries(axion_policy_match_guard_test PRIVATE t81_tool_cli t81_isa t81_vm)

add_executable(axion_policy_gc_trace_test tests/cpp/axion_policy_gc_trace_test.cpp)
target_link_libraries(axion_policy_gc_trace_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(axion_policy_bytecode_test tests/cpp/axion_policy_bytecode_test.cpp)
  target_link_libraries(axion_policy_bytecode_test PRIVATE t81_core t81_axion)

add_executable(axion_policy_segment_event_test tests/cpp/axion_policy_segment_event_test.cpp)
target_link_libraries(axion_policy_segment_event_test PRIVATE t81_isa t81_vm)

  add_executable(axion_policy_invariants_test tests/cpp/axion_policy_invariants_test.cpp)
  target_link_libraries(axion_policy_invariants_test PRIVATE t81_vm t81_core)

  add_executable(axion_policy_allow_deny_determinism_test
                 tests/cpp/axion_policy_allow_deny_determinism_test.cpp)
  target_link_libraries(axion_policy_allow_deny_determinism_test PRIVATE t81_vm t81_core)

  # RFC-0032 Phase 3 gate tests — AIHookEngine + AiModelLoader
  add_executable(axion_ai_hooks_test tests/cpp/axion_ai_hooks_test.cpp)
  target_link_libraries(axion_ai_hooks_test PRIVATE t81_axion t81_core)
  add_test(NAME axion_ai_hooks_test COMMAND axion_ai_hooks_test)

  # RFC-0032 Phase 4 gate tests — T81VmBackend (C-02)
  add_executable(backend_adapter_test tests/cpp/backend_adapter_test.cpp)
  target_link_libraries(backend_adapter_test PRIVATE t81_vm t81_axion t81_core)
  add_test(NAME backend_adapter_test COMMAND backend_adapter_test)

  # RFC-0032 Phase 5 gate tests — EvidenceCollector (C-06)
  add_executable(t81_determinism_evidence_test
                 tests/determinism/evidence_collector.cpp)
  target_link_libraries(t81_determinism_evidence_test PRIVATE t81_axion t81_core)
  add_test(NAME t81_determinism_evidence_test COMMAND t81_determinism_evidence_test)

  add_executable(axion_policy_conformance_matrix_test
                 tests/cpp/axion_policy_conformance_matrix_test.cpp)
  target_link_libraries(axion_policy_conformance_matrix_test PRIVATE t81_vm t81_core)

  add_executable(axion_instruction_counter_test tests/cpp/axion_instruction_counter_test.cpp)
  target_link_libraries(axion_instruction_counter_test PRIVATE t81_core t81_vm)

  add_executable(e2e_match_expression_test tests/cpp/e2e_match_expression_test.cpp)
  target_link_libraries(e2e_match_expression_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_loop_statement_test tests/cpp/e2e_loop_statement_test.cpp)
  target_link_libraries(e2e_loop_statement_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_advanced_features_test tests/cpp/e2e_advanced_features_test.cpp)
  target_link_libraries(e2e_advanced_features_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_print_runtime_test tests/cpp/e2e_print_runtime_test.cpp)
  target_link_libraries(e2e_print_runtime_test PRIVATE t81_lang_frontend t81_isa t81_vm)

  add_executable(e2e_compile_determinism_test tests/cpp/e2e_compile_determinism_test.cpp)
  target_link_libraries(e2e_compile_determinism_test PRIVATE t81_tool_cli t81_isa t81_vm t81_core)

  add_executable(e2e_match_metadata_determinism_test tests/cpp/e2e_match_metadata_determinism_test.cpp)
  target_link_libraries(e2e_match_metadata_determinism_test PRIVATE t81_tool_cli t81_vm)

  add_executable(e2e_ast_ir_canonical_determinism_test tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp)
  target_link_libraries(e2e_ast_ir_canonical_determinism_test PRIVATE t81_lang_frontend t81_core)

  add_executable(e2e_enum_metadata_determinism_test tests/cpp/e2e_enum_metadata_determinism_test.cpp)
  target_link_libraries(e2e_enum_metadata_determinism_test PRIVATE t81_tool_cli t81_isa)

  add_executable(cli_e2e_test tests/cpp/cli_e2e_test.cpp)
  target_link_libraries(cli_e2e_test PRIVATE t81_tool_cli)

  if(T81_BUILD_FUZZ_TESTS)
    add_executable(t81_frontend_fuzz_test tests/cpp/frontend_fuzz_test.cpp)
    target_link_libraries(t81_frontend_fuzz_test PRIVATE t81_lang_frontend)

    add_executable(fuzz_parser tests/fuzz/fuzz_parser.cpp)
    target_link_libraries(fuzz_parser PRIVATE t81_lang_frontend t81_core)

    add_executable(fuzz_vm tests/fuzz/fuzz_vm.cpp)
    target_link_libraries(fuzz_vm PRIVATE t81_vm t81_core t81_isa)

    if(T81_ENABLE_LIBFUZZER)
      foreach(_fuzz_target fuzz_parser fuzz_vm)
        target_compile_options(${_fuzz_target} PRIVATE -fsanitize=fuzzer -DT81_FUZZ_TARGET)
        target_link_options(${_fuzz_target} PRIVATE -fsanitize=fuzzer)
      endforeach()
    endif()

  endif()

  add_executable(t81_t81int_test tests/cpp/test_t81int.cpp)
  target_link_libraries(t81_t81int_test PRIVATE t81_core)

  add_executable(t81_t81int_overflow_test tests/cpp/test_t81int_overflow.cpp)
  target_link_libraries(t81_t81int_overflow_test PRIVATE t81_core)

  add_executable(t81_t81int_generic_div_test tests/cpp/test_t81int_generic_div.cpp)
  target_link_libraries(t81_t81int_generic_div_test PRIVATE t81_core)

  add_executable(t81_int_sign_trit_test tests/cpp/test_t81int_sign_trit.cpp)
  target_link_libraries(t81_int_sign_trit_test PRIVATE t81_core)

  add_executable(t81_division_test tests/cpp/test_division.cpp)
  target_link_libraries(t81_division_test PRIVATE t81_core)

  add_executable(t81_float_test tests/cpp/test_T81Float.cpp)
  target_link_libraries(t81_float_test PRIVATE t81_core)

  add_executable(t81_float_deterministic_test tests/cpp/test_T81Float.cpp)
  target_compile_definitions(t81_float_deterministic_test PRIVATE T81_DETERMINISTIC)
  target_link_libraries(t81_float_deterministic_test PRIVATE t81_core)
  add_test(NAME t81_float_deterministic_test COMMAND t81_float_deterministic_test)

  add_executable(t81_bigint_float_conversion_test tests/cpp/test_T81BigInt_Float_conversion.cpp)
  target_link_libraries(t81_bigint_float_conversion_test PRIVATE t81_core)
  add_test(NAME t81_bigint_float_conversion_test COMMAND t81_bigint_float_conversion_test)

  add_executable(t81_float_properties_test tests/cpp/float_properties_test.cpp)
  target_link_libraries(t81_float_properties_test PRIVATE t81_core)

  add_executable(t81_t81int_to_binary_test tests/cpp/test_t81int_to_binary.cpp)
  target_link_libraries(t81_t81int_to_binary_test PRIVATE t81_core)

  add_executable(t81_complex_test tests/cpp/test_T81Complex.cpp)
  target_link_libraries(t81_complex_test PRIVATE t81_core)

  add_executable(t81_prob_test tests/cpp/test_T81Prob.cpp)
  target_link_libraries(t81_prob_test PRIVATE t81_core)

  add_executable(t81_symbol_test tests/cpp/test_T81Symbol.cpp)
  target_link_libraries(t81_symbol_test PRIVATE t81_core)

  add_executable(t81_string_test tests/cpp/test_T81String.cpp)
  target_link_libraries(t81_string_test PRIVATE t81_core)

  add_executable(t81_qutrit_test tests/cpp/test_T81Qutrit.cpp)
  target_link_libraries(t81_qutrit_test PRIVATE t81_core)

  add_executable(t81_fixed_test tests/cpp/test_T81Fixed.cpp)
  target_link_libraries(t81_fixed_test PRIVATE t81_core)

  add_executable(t81_uint_test tests/cpp/test_T81Uint.cpp)
  target_link_libraries(t81_uint_test PRIVATE t81_core)

  add_executable(t81_list_test tests/cpp/test_T81List.cpp)
  target_link_libraries(t81_list_test PRIVATE t81_core)

  add_executable(t81_map_test tests/cpp/test_T81Map.cpp)
  target_link_libraries(t81_map_test PRIVATE t81_core)

  add_executable(t81_set_test tests/cpp/test_T81Set.cpp)
  target_link_libraries(t81_set_test PRIVATE t81_core)

  add_executable(t81_vector_test tests/cpp/test_T81Vector.cpp)
  target_link_libraries(t81_vector_test PRIVATE t81_core)

  add_executable(t81_matrix_test tests/cpp/test_T81Matrix.cpp)
  target_link_libraries(t81_matrix_test PRIVATE t81_core)

  add_executable(t81_matrix_singular_test tests/cpp/test_T81Matrix_singular.cpp)
  target_link_libraries(t81_matrix_singular_test PRIVATE t81_core)

  add_executable(debug_matrix_crash tests/cpp/debug_matrix_crash.cpp)
  target_link_libraries(debug_matrix_crash PRIVATE t81_core)
  add_test(NAME debug_matrix_crash COMMAND debug_matrix_crash)

  add_executable(t81_maybe_test tests/cpp/test_T81Maybe.cpp)
  target_link_libraries(t81_maybe_test PRIVATE t81_core)

  add_executable(t81_result_test tests/cpp/test_T81Result.cpp)
  target_link_libraries(t81_result_test PRIVATE t81_core)

  add_executable(t81_simd_add_helpers_test tests/cpp/test_t81_simd_add_helpers.cpp)
  target_link_libraries(t81_simd_add_helpers_test PRIVATE t81_core)

  add_executable(t81_quaternion_test tests/cpp/test_T81Quaternion.cpp)
  target_link_libraries(t81_quaternion_test PRIVATE t81_core)

  add_executable(t81_time_test tests/cpp/test_T81Time.cpp)
  target_link_libraries(t81_time_test PRIVATE t81_core)

  add_executable(t81_bytes_test tests/cpp/test_T81Bytes.cpp)
  target_link_libraries(t81_bytes_test PRIVATE t81_core)

  add_executable(t81_tree_test tests/cpp/test_T81Tree.cpp)
  target_link_libraries(t81_tree_test PRIVATE t81_core)

  add_executable(t81_native_conversion_test tests/cpp/test_t81_native_conversion.cpp)
  target_link_libraries(t81_native_conversion_test PRIVATE t81_core)

  add_executable(t81_native_property_test tests/cpp/t81_native_property_test.cpp)
  target_link_libraries(t81_native_property_test PRIVATE t81_core)

  add_executable(t81_limb_bohemian_add_test tests/cpp/test_t81_limb_bohemian_add.cpp)
  target_link_libraries(t81_limb_bohemian_add_test PRIVATE t81_core)

  add_executable(t81_limb_bohemian_mul_test tests/cpp/test_t81_limb_bohemian_mul.cpp)
  target_link_libraries(t81_limb_bohemian_mul_test PRIVATE t81_core)

  add_executable(t81_limb54_add_test tests/cpp/test_t81_limb54_add.cpp)
  target_link_libraries(t81_limb54_add_test PRIVATE t81_core)

  add_executable(t81_limb54_booth_mul_test tests/cpp/test_t81_limb54_booth_mul.cpp)
  target_link_libraries(t81_limb54_booth_mul_test PRIVATE t81_core)

  add_executable(t81_limb_booth_mul_test tests/cpp/test_t81_limb_booth_mul.cpp)
  target_link_libraries(t81_limb_booth_mul_test PRIVATE t81_core)

  add_executable(t81_limb_mul_wide_high_test tests/cpp/test_t81_limb_mul_wide_high.cpp)
  target_link_libraries(t81_limb_mul_wide_high_test PRIVATE t81_core)

  add_executable(t81_limb_add_test tests/cpp/test_t81_limb_add.cpp)
  target_link_libraries(t81_limb_add_test PRIVATE t81_core)

  add_executable(t81_native_arith_test tests/cpp/test_t81_native_arith.cpp)
  target_link_libraries(t81_native_arith_test PRIVATE t81_core)

  add_executable(t81_bigint_v1_baseline_test tests/cpp/test_bigint_v1_baseline.cpp)
  target_link_libraries(t81_bigint_v1_baseline_test PRIVATE t81_core)

  add_executable(t81_bigint_v1_perf tests/cpp/test_bigint_v1_perf.cpp)
  target_link_libraries(t81_bigint_v1_perf PRIVATE t81_core)

  add_executable(test_bigint_robust_features tests/cpp/test_bigint_robust_features.cpp)
  target_link_libraries(test_bigint_robust_features PRIVATE t81_core)

  add_executable(test_bigint_multilimb_ext tests/cpp/test_bigint_multilimb_ext.cpp)
  target_link_libraries(test_bigint_multilimb_ext PRIVATE t81_core)

  add_executable(test_T81Tensor_transpose tests/cpp/test_T81Tensor_transpose.cpp)
  target_link_libraries(test_T81Tensor_transpose PRIVATE t81_core)

  add_executable(performance_advancements_test tests/cpp/performance_advancements_test.cpp)
  target_link_libraries(performance_advancements_test PRIVATE t81_core t81_axion)

  add_executable(t81_distributed_tensor_test tests/cpp/distributed_tensor_test.cpp)
  target_link_libraries(t81_distributed_tensor_test PRIVATE t81_core)

  add_executable(tensor_backend_parity_test tests/cpp/tensor_backend_parity_test.cpp)
  target_link_libraries(tensor_backend_parity_test PRIVATE t81_core)

  add_executable(bigint_perf_guardrail_test tests/cpp/bigint_perf_guardrail_test.cpp)
  target_link_libraries(bigint_perf_guardrail_test PRIVATE t81_core)

  add_executable(t81_bigint_allocation_pathology_test tests/cpp/test_bigint_allocation_pathology.cpp)
  target_link_libraries(t81_bigint_allocation_pathology_test PRIVATE t81_core)

  add_executable(t81_bigint_allocation_guardrail_test tests/cpp/bigint_allocation_guardrail_test.cpp)
  target_link_libraries(t81_bigint_allocation_guardrail_test PRIVATE t81_core)

  add_executable(t81_vm_tier_promotion_test tests/cpp/vm_tier_promotion_test.cpp)
  target_link_libraries(t81_vm_tier_promotion_test PRIVATE t81_vm t81_core t81_isa)

  add_executable(t81_code_protection_test tests/cpp/test_code_protection.cpp)
  target_link_libraries(t81_code_protection_test PRIVATE t81_vm t81_core)

  add_executable(axion_heap_compaction_trace_test tests/cpp/axion_heap_compaction_trace_test.cpp)
  target_link_libraries(axion_heap_compaction_trace_test PRIVATE t81_tool_cli t81_isa t81_vm)

  add_executable(axion_nested_guard_test tests/cpp/axion_nested_guard_test.cpp)
  target_link_libraries(axion_nested_guard_test PRIVATE t81_tool_cli t81_vm)

  add_executable(e2e_axion_trace_test tests/cpp/e2e_axion_trace_test.cpp)
  target_link_libraries(e2e_axion_trace_test PRIVATE t81_tool_cli t81_vm)

  if(T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND MLIR_FOUND)
    add_executable(t81_mlir_t81_dialect_memory_ops_test tests/cpp/mlir_t81_dialect_memory_ops_test.cpp)
    target_link_libraries(t81_mlir_t81_dialect_memory_ops_test PRIVATE t81_mlir t81_core t81_isa)
  endif()

  enable_testing()
  set(T81_TEST_TARGETS
    t81_float_test
    t81_float_properties_test
    t81_t81int_to_binary_test
    t81_division_test
    t81_t81int_test
    t81_t81int_overflow_test
    t81_t81int_generic_div_test
    t81_int_sign_trit_test
    t81_bigint_test
    t81_fraction_test
    t81_bigint_properties_test
    t81_prob_properties_test
    t81_t81int_properties_test
    t81_bigint_division_edge_properties_test
    t81_bigint_division_semantics_test
    t81_bigint_gcd_divmod_property_test
    t81_bigint_modular_inverse_test
    fuzz_bigint_libfuzzer
    t81_fraction_properties_test
    t81_core_numeric_compat_test
    t81_core_bigint_compat_properties_test
    t81_core_fraction_compat_properties_test
    t81_v1_canonical_numeric_contract_test
    t81_complex_test
    t81_prob_test
    t81_symbol_test
    t81_string_test
    t81_qutrit_test
    t81_fixed_test
    t81_uint_test
    t81_list_test
    t81_map_test
    t81_set_test
    t81_vector_test
    t81_matrix_test
    t81_matrix_singular_test
    t81_maybe_test
    t81_result_test
    t81_quaternion_test
    t81_time_test
    t81_bytes_test
    t81_tree_test
    t81_tensor_transpose_test
    t81_core_improvements_test
    t81_core_features_test
    t81_tensor_slice_test
    t81_tensor_reshape_test
    t81_tensor_loader_test
    t81_canonfs_io_test
    canonfs_read_verify_test
    canonfs_read_verify_env_contract_test
    canonfs_integrity_matrix_test
    t81_vm_canonfs_root_env_contract_test
    t81_ir_encoding_test
    t81_isa_encoding_test
    t81_hash_stub_test
    t81_sha3_test
    t81_axion_stub_test
    t81_codec_base243_test
    t81_codec_base81_test
    t81_trit_packing_test
    t81_packed_trit_vector_test
    t81_phase2c_truth_table_test
    t81_packed_trit_vector_stress_test
    t81_base81_balanced_test
    t81_base81_packed_test
    t81_metadata_test
    t81_isa_base81_view_test
    setun_bridge_test
    tisc_opcode_matrix_test
    t81lang_conformance_baseline_test
    t81_numeric_format_test
    axion_recursion_guardrails_test
    t81_tensor_shape_test
    t81_tensor_serialization_canonical_fixed_test
    t81_tensor_native_decode_test
    t81_ternary_arith_test
    t81_tensor_matmul_test
    t81_tensor_reduce_test
    t81_tensor_broadcast_test
    t81_entropy_test
    t81_expected_test
    t81_tensor_elementwise_test
    t81_tensor_ternary_test
    t81_t729_tensor_test
    t81_symbolic_algebra_test
    t81_semantic_graph_test
    t81_neural_primitives_test
    t81_frontend_neural_test
    t81_c_api_bigint_test
    t81_tensor_unary_test
    t81_hanoi_integration_test
    t81_vm_load_store_test
    t81_vm_illegal_test
    t81_vm_bounds_test
    t81_vm_tensor_test
    t81_vm_tloadhash_conformance_test
    t81_vm_tloadhash_canonical_fixed_test
    t81_vm_tloadhash_decodefault_determinism_matrix_test
    t81_vm_ai_phase1_attention_conformance_test
    t81_vm_ai_phase1_embed_conformance_test
    t81_vm_ai_phase1_qmatmul_conformance_test
    t81_vm_ai_phase1_shared_helper_parity_test
    t81_vm_ai_phase1_wload_conformance_test
    t81_vm_ai_phase1_gather_conformance_test
    t81_vm_ai_phase1_scatter_conformance_test
    t81_vm_ai_phase1_wload_canonfs_audit_test
    t81_vm_ai_phase1_scatter_aliasing_test
    t81_vm_ai_phase1_gather_axis1_test
    t81_lang_ai_m6_annotation_lowering_test
    t81_vm_tensor_shape_faults_test
    t81_vm_tisc_v04_extensions_test
    t81_vm_tensor_get_set_conformance_test
    t81_vm_tensor_helper_predicates_test
    t81_vm_tensor_provenance_trace_test
    t81_vm_predispatch_policy_deny_logging_test
    t81_vm_fault_test
    t81_vm_deterministic_fault_test
    t81_vm_fault_family_determinism_matrix_test
    t81_vm_memory_test
    t81_axion_log_determinism_test
    t81_reflection_non_interference_test
    t81_canonfs_driver_test
    t81_canonfs_rs_repair_test
    t81_canonhash81_reference_vectors_test
    t81_weights_load_test
    t81_vm_trace_test
    t81_vm_determinism_property_test
    t81_vm_state_transition_invariants_test
    t81_vm_state_transition_conformance_matrix_test
    t81_vm_workload_determinism_tiers_test
    t81_vm_mixed_workload_conformance_matrix_test
    t81_vm_policy_parse_fail_closed_test
    t81_vm_axreport_policy_deny_fail_closed_test
    t81_vm_system_registers_deterministic_test
    t81_vm_stubbed_opcode_fail_closed_test
    vm_extended_ops_test
    llama_kernels_test
    t81_vm_jump_flags_test
    t81_vm_float_fraction_ops_test
    t81_vm_literal_pool_extension_test
    t81_vm_neg_jumps_test
    t81_vm_print_test
    t81_limb_bohemian_add_test
    t81_limb_bohemian_mul_test
    t81_limb_booth_mul_test
    t81_limb_mul_wide_high_test
    t81_limb_add_test
    t81_native_arith_test
    t81_native_conversion_test
    t81_simd_add_helpers_test
    t81_native_property_test
    t81_bigint_v1_baseline_test
    t81_limb54_add_test
    t81_limb54_booth_mul_test
    t81_frontend_lexer_test
    t81_frontend_parser_test
    t81_spec_compliance_test
    tisc_semantic_equivalence_test
    tisc_opcode_family_semantics_test
    t81_frontend_expression_features_test
    t81_frontend_parser_generics_test
    t81_frontend_parser_recovery_test
    t81_frontend_parser_legacy_rejection_test
    t81_frontend_ir_generator_test
    t81_frontend_logical_lowering_test
    t81_semantic_analyzer_option_result_test
    test_std_option_result
    t81_semantic_analyzer_match_test
    t81_semantic_analyzer_generic_test
    t81_semantic_analyzer_indexing_test
    t81_semantic_analyzer_vector_literal_test
    t81_semantic_analyzer_equality_test
    t81_semantic_analyzer_loop_test
    t81_semantic_analyzer_numeric_test
    t81_semantic_analyzer_diagnostic_location_test
    t81_semantic_analyzer_cascade_suppression_test
    t81_semantic_analyzer_diagnostic_precision_test
    t81_isa_binary_emitter_test
    t81_isa_type_alias_io_test
    t81_isa_binary_io_determinism_test
    t81_isa_binary_metadata_roundtrip_property_test
    e2e_option_result_test
    e2e_option_result_function_test
    e2e_option_type_test
    e2e_match_expression_test
    e2e_loop_statement_test
    e2e_advanced_features_test
    e2e_print_runtime_test
    e2e_compile_determinism_test
    e2e_match_metadata_determinism_test
    e2e_ast_ir_canonical_determinism_test
    e2e_enum_metadata_determinism_test
    cli_e2e_test
    t81_cli_check_test
    t81_cli_pipeline_test
    t81_cli_disasm_test
    t81_cli_debugger_test
    t81_cli_option_result_test
    t81_cli_diagnostic_context_test
    t81_cli_trace_export_test
    cli_canonize_tensor_test
    axion_heap_compaction_trace_test
    axion_policy_gc_trace_test
    axion_policy_match_guard_test
    axion_policy_segment_event_test
    axion_policy_invariants_test
    axion_policy_allow_deny_determinism_test
    axion_policy_conformance_matrix_test
    axion_loop_metadata_test
    axion_match_metadata_test
    axion_enum_guard_test
    axion_nested_guard_test
    axion_instruction_counter_test
    canonfs_axion_trace_test
    t81_tier4_test
    t81_tier5_test
    t81_infinite_opcodes_test
    t81_tier4_vm_test
    t81_tier4_distributed_test
    t81_jit_test
    jit_trace_equivalence_test
    jit_repro_oracle_test
    jit_canonfs_cache_test
    vm_gc_safepoint_test
    agent_constructs_test
    t81lang_foreign_ffi_test
    tnn_stdlib_test
    lattice_crypto_test
    jit_tensor_trace_equivalence_test
    vm_jit_per_instruction_policy_test
    e2e_axion_trace_test
    axion_policy_bytecode_test
    test_bigint_multilimb_ext
    test_bigint_robust_features
    t81_bigint_v1_perf
    bigint_perf_guardrail_test
    t81_bigint_allocation_pathology_test
    t81_bigint_allocation_guardrail_test
    t81_vm_tier_promotion_test
    test_T81Tensor_transpose
    performance_advancements_test
    t81_distributed_tensor_test
    tensor_backend_parity_test
    t81_vm_rfc0034_ternary_native_test
  )
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    list(APPEND T81_TEST_TARGETS t81_base81_simd_test)
  endif()
  if(T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND MLIR_FOUND)
    list(APPEND T81_TEST_TARGETS t81_mlir_t81_dialect_memory_ops_test)
  endif()
  if(T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY)
    list(APPEND T81_TEST_TARGETS t81_c_frontend_mlir_smoke_test)
  endif()
  if(T81_ENABLE_RUST_FRONTEND AND T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY AND T81_RUSTC_EXECUTABLE)
    list(APPEND T81_TEST_TARGETS t81_rust_frontend_mlir_smoke_test)
  endif()
  if(T81_ENABLE_PYTHON_FRONTEND AND T81_ENABLE_C_FRONTEND AND T81_ENABLE_MLIR AND T81_ENABLE_LLVM AND T81_CLANG_C_INCLUDE_DIR AND T81_CLANG_C_LIBRARY AND Python3_EXECUTABLE)
    list(APPEND T81_TEST_TARGETS t81_python_frontend_mlir_smoke_test)
  endif()
  if(T81_BUILD_FUZZ_TESTS AND T81_RUN_FRONTEND_FUZZ_IN_CTEST)
    list(APPEND T81_TEST_TARGETS t81_frontend_fuzz_test)
  endif()
  foreach(tgt IN LISTS T81_TEST_TARGETS)
    add_test(NAME ${tgt} COMMAND $<TARGET_FILE:${tgt}>)
  endforeach()
  add_test(NAME t81_cli_repro_hash_test COMMAND $<TARGET_FILE:cli_repro_hash_test> $<TARGET_FILE:t81>)
  if(NOT WIN32)
    add_test(NAME t81_cli_contract_test_runner COMMAND $<TARGET_FILE:t81_cli_contract_test> $<TARGET_FILE:t81>)
  endif()
  if(NOT WIN32)
    add_test(NAME t81_cli_stress_test_runner COMMAND $<TARGET_FILE:t81_cli_stress_test> $<TARGET_FILE:t81>)
    set_tests_properties(t81_cli_stress_test_runner PROPERTIES TIMEOUT 300)
  endif()
  add_test(NAME t81_cli_stdlib_fixtures_test COMMAND $<TARGET_FILE:cli_stdlib_fixtures_test>)
  add_test(NAME cli_std_collections_determinism_test COMMAND $<TARGET_FILE:cli_std_collections_determinism_test>)
  add_test(
    NAME t81_legacy_core_numeric_include_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_legacy_core_numeric_includes_test.py
  )
  set_tests_properties(t81_legacy_core_numeric_include_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_test(
    NAME t81_legacy_core_numeric_type_usage_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_legacy_core_numeric_type_usage_test.py
  )
  set_tests_properties(t81_legacy_core_numeric_type_usage_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_test(
    NAME t81_legacy_v1_numeric_include_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_legacy_v1_numeric_includes_test.py
  )
  set_tests_properties(t81_legacy_v1_numeric_include_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_test(
    NAME t81_core_numeric_wrapper_thinness_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_core_numeric_wrapper_thinness_test.py
  )
  set_tests_properties(t81_core_numeric_wrapper_thinness_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_test(
    NAME t81_v1_canonical_numeric_alias_usage_policy_test
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/ci/check_v1_canonical_numeric_alias_usage_test.py
  )
  set_tests_properties(t81_v1_canonical_numeric_alias_usage_policy_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif()

# =============================================================================
# Python Bindings
# =============================================================================
execute_process(
    COMMAND python3 -c "import pybind11; print(pybind11.get_cmake_dir())"
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(pybind11_DIR)
  set(PYBIND11_FINDPYTHON ON)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_package(pybind11 REQUIRED)
  pybind11_add_module(t81_python src/python/t81_python.cpp)
  target_link_libraries(t81_python PRIVATE t81_core t81_lang_frontend t81_isa t81_vm)
endif()

# =============================================================================
# Benchmarks
# =============================================================================
if(T81_BUILD_BENCHMARKS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
  else()
    message(WARNING "T81_BUILD_BENCHMARKS=ON but benchmarks/ is unavailable in this checkout; skipping benchmark targets.")
  endif()
endif()

# =============================================================================
# Doxygen
# =============================================================================
find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
endif()

# =============================================================================
# Spec Conformance Suite (RFC-0027 SE-M1)
# =============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spec/conformance/CMakeLists.txt")
  add_subdirectory(spec/conformance)
else()
  message(WARNING "spec/conformance is unavailable in this checkout; skipping spec conformance targets.")
endif()

if(T81_BUILD_TESTS)
  add_executable(t81_graph_test tests/cpp/test_T81Graph.cpp)
  target_link_libraries(t81_graph_test PRIVATE t81_core)
  add_test(NAME t81_graph_test COMMAND t81_graph_test)

  add_executable(t81_graph_components_test tests/cpp/test_T81Graph_components.cpp)
  target_link_libraries(t81_graph_components_test PRIVATE t81_core)
  add_test(NAME t81_graph_components_test COMMAND t81_graph_components_test)

  add_executable(t81_graph_shortest_path_test tests/cpp/test_T81Graph_shortest_path.cpp)
  target_link_libraries(t81_graph_shortest_path_test PRIVATE t81_core)
  add_test(NAME t81_graph_shortest_path_test COMMAND t81_graph_shortest_path_test)

  add_executable(t81_graph_large_test tests/cpp/test_T81Graph_large.cpp)
  target_link_libraries(t81_graph_large_test PRIVATE t81_core)
  add_test(NAME t81_graph_large_test COMMAND t81_graph_large_test)

  add_executable(t81_map_regression_test tests/cpp/test_T81Map_regression.cpp)
  target_link_libraries(t81_map_regression_test PRIVATE t81_core)
  add_test(NAME t81_map_regression_test COMMAND t81_map_regression_test)

  add_executable(t81_polynomial_test tests/cpp/test_T81Polynomial.cpp)
  target_link_libraries(t81_polynomial_test PRIVATE t81_core)
  add_test(NAME t81_polynomial_test COMMAND t81_polynomial_test)

  add_executable(t81_agent_test tests/cpp/test_T81Agent.cpp)
  target_link_libraries(t81_agent_test PRIVATE t81_core)
  add_test(NAME t81_agent_test COMMAND t81_agent_test)

  add_executable(t81_agent_neural_test tests/cpp/test_T81Agent_neural.cpp)
  target_link_libraries(t81_agent_neural_test PRIVATE t81_core)
  add_test(NAME t81_agent_neural_test COMMAND t81_agent_neural_test)

  add_executable(t81_stream_test tests/cpp/test_T81Stream.cpp)
  target_link_libraries(t81_stream_test PRIVATE t81_core)
  add_test(NAME t81_stream_test COMMAND t81_stream_test)

  add_executable(t81_promise_test tests/cpp/test_T81Promise.cpp)
  target_link_libraries(t81_promise_test PRIVATE t81_core)
  add_test(NAME t81_promise_test COMMAND t81_promise_test)

  add_executable(t81_thread_test tests/cpp/test_T81Thread.cpp)
  target_link_libraries(t81_thread_test PRIVATE t81_core)
  add_test(NAME t81_thread_test COMMAND t81_thread_test)

  add_executable(t81_network_test tests/cpp/test_T81Network.cpp)
  target_link_libraries(t81_network_test PRIVATE t81_core asio)
  if(WIN32)
    target_link_libraries(t81_network_test PRIVATE ws2_32 mswsock)
  endif()
  add_test(NAME t81_network_test COMMAND t81_network_test)

  add_executable(t81_discovery_test tests/cpp/test_T81Discovery.cpp)
  target_link_libraries(t81_discovery_test PRIVATE t81_core asio)
  if(WIN32)
    target_link_libraries(t81_discovery_test PRIVATE ws2_32 mswsock)
  endif()
  add_test(NAME t81_discovery_test COMMAND t81_discovery_test)

  add_executable(t81_category_test tests/cpp/test_T81Category.cpp)
  target_link_libraries(t81_category_test PRIVATE t81_core)
  add_test(NAME t81_category_test COMMAND t81_category_test)

  add_executable(t81_proof_test tests/cpp/test_T81Proof.cpp)
  target_link_libraries(t81_proof_test PRIVATE t81_core)
  add_test(NAME t81_proof_test COMMAND t81_proof_test)

  add_executable(t81_tensor_slice_static_test tests/cpp/test_T81Tensor_slice_static.cpp)
  target_link_libraries(t81_tensor_slice_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_slice_static_test COMMAND t81_tensor_slice_static_test)

  add_executable(t81_tensor_concat_static_test tests/cpp/test_T81Tensor_concat_static.cpp)
  target_link_libraries(t81_tensor_concat_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_concat_static_test COMMAND t81_tensor_concat_static_test)

  add_executable(t81_tensor_permute_static_test tests/cpp/test_T81Tensor_permute_static.cpp)
  target_link_libraries(t81_tensor_permute_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_permute_static_test COMMAND t81_tensor_permute_static_test)

  add_executable(t81_tensor_pad_static_test tests/cpp/test_T81Tensor_pad_static.cpp)
  target_link_libraries(t81_tensor_pad_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_pad_static_test COMMAND t81_tensor_pad_static_test)

  add_executable(t81_tensor_activations_static_test tests/cpp/test_T81Tensor_activations_static.cpp)
  target_link_libraries(t81_tensor_activations_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_activations_static_test COMMAND t81_tensor_activations_static_test)

  add_executable(t81_tensor_softmax_static_test tests/cpp/test_T81Tensor_softmax_static.cpp)
  target_link_libraries(t81_tensor_softmax_static_test PRIVATE t81_core)
  add_test(NAME t81_tensor_softmax_static_test COMMAND t81_tensor_softmax_static_test)

  add_executable(t81_graph_pagerank_convergence_test tests/cpp/test_T81Graph_pagerank_convergence.cpp)
  target_link_libraries(t81_graph_pagerank_convergence_test PRIVATE t81_core)
  add_test(NAME t81_graph_pagerank_convergence_test COMMAND t81_graph_pagerank_convergence_test)

  add_executable(t81_graph_bfs_test tests/cpp/test_T81Graph_bfs.cpp)
  target_link_libraries(t81_graph_bfs_test PRIVATE t81_core)
  add_test(NAME t81_graph_bfs_test COMMAND t81_graph_bfs_test)

  add_executable(t81_float_rounding_test tests/cpp/test_T81Float_rounding.cpp)
  target_link_libraries(t81_float_rounding_test PRIVATE t81_core)
  add_test(NAME t81_float_rounding_test COMMAND t81_float_rounding_test)

  add_executable(t81_float_clamp_test tests/cpp/test_T81Float_clamp.cpp)
  target_link_libraries(t81_float_clamp_test PRIVATE t81_core)
  add_test(NAME t81_float_clamp_test COMMAND t81_float_clamp_test)

  add_executable(t81_int_pow_test tests/cpp/test_T81Int_pow.cpp)
  target_link_libraries(t81_int_pow_test PRIVATE t81_core)
  add_test(NAME t81_int_pow_test COMMAND t81_int_pow_test)

  add_executable(t81_int_gcd_lcm_test tests/cpp/test_T81Int_gcd_lcm.cpp)
  target_link_libraries(t81_int_gcd_lcm_test PRIVATE t81_core)
  add_test(NAME t81_int_gcd_lcm_test COMMAND t81_int_gcd_lcm_test)

  add_executable(t81_int_shift_test tests/cpp/test_t81int_shift.cpp)
  target_link_libraries(t81_int_shift_test PRIVATE t81_core)
  add_test(NAME t81_int_shift_test COMMAND t81_int_shift_test)

  add_executable(t81_std_test tests/cpp/test_t81_std.cpp)
  target_link_libraries(t81_std_test PRIVATE t81_core)
  add_test(NAME t81_std_test COMMAND t81_std_test)

  add_executable(t81_symbolic_vm_test tests/cpp/symbolic_vm_test.cpp)
  target_link_libraries(t81_symbolic_vm_test PRIVATE t81_core t81_isa t81_vm)
  add_test(NAME t81_symbolic_vm_test COMMAND t81_symbolic_vm_test)

  add_executable(tier1_rewrite_confluence_test tests/cpp/tier1_rewrite_confluence_test.cpp)
  target_link_libraries(tier1_rewrite_confluence_test PRIVATE t81_core)
  add_test(NAME tier1_rewrite_confluence_test COMMAND tier1_rewrite_confluence_test)

  add_executable(test_t81float_scientific tests/cpp/test_t81float_scientific.cpp)
  target_link_libraries(test_t81float_scientific PRIVATE t81_core t81_isa t81_lang_frontend)
  add_test(NAME test_t81float_scientific COMMAND test_t81float_scientific)

  add_executable(test_symbol_type tests/cpp/test_symbol_type.cpp)
  target_link_libraries(test_symbol_type PRIVATE t81_core t81_isa t81_lang_frontend)
  add_test(NAME test_symbol_type COMMAND test_symbol_type)

  add_executable(test_dynamic_vector tests/cpp/test_dynamic_vector.cpp)
  target_link_libraries(test_dynamic_vector PRIVATE t81_core t81_isa t81_lang_frontend)
  add_test(NAME test_dynamic_vector COMMAND test_dynamic_vector)

  add_executable(test_concurrency tests/cpp/test_concurrency.cpp)
  target_link_libraries(test_concurrency PRIVATE t81_core t81_isa t81_vm)
  add_test(NAME test_concurrency COMMAND test_concurrency)

  add_executable(test_collections_scaffold tests/cpp/test_collections_scaffold.cpp)
  target_link_libraries(test_collections_scaffold PRIVATE t81_core t81_isa t81_vm t81_lang_frontend t81_axion)
  add_test(NAME test_collections_scaffold COMMAND test_collections_scaffold)

  if(T81_ENABLE_LLAMA_CPP)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    if(T81_LLAMA_REPRO_MODEL AND T81_LLAMA_REPRO_POLICY AND T81_LLAMA_REPRO_PROMPT)
      add_test(
        NAME llama_cpp_repro_gate_test
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/ci/llama_cpp_repro_gate.py
          --t81-bin $<TARGET_FILE:t81>
          --model ${T81_LLAMA_REPRO_MODEL}
          --policy ${T81_LLAMA_REPRO_POLICY}
          --prompt ${T81_LLAMA_REPRO_PROMPT}
          --runs 3
          --max-tokens 64
          --seed 0
          --threads 1
          --top-k 1
          --top-p 1.0
          --temperature 0.0
          --hash-out ${CMAKE_BINARY_DIR}/llama_cpp_repro_hash.txt
      )
    else()
      message(STATUS
        "llama_cpp_repro_gate_test not added (set T81_LLAMA_REPRO_MODEL, "
        "T81_LLAMA_REPRO_POLICY, and T81_LLAMA_REPRO_PROMPT to enable).")
    endif()
  endif()
endif()

# =============================================================================
# Installation & Packaging
# =============================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install targets and export them
set(T81_INSTALL_TARGETS
  t81_core
  t81_axion
  t81_ffi
  t81_tool_model
  t81_io
  t81_c_api
  t81_lang_frontend
  t81_isa
  t81_vm
  t81_llvm
  t81_mlir
  t81_tool_cli
  t81
)
if(TARGET t81_dmath_runtime)
  list(APPEND T81_INSTALL_TARGETS t81_dmath_runtime)
endif()
if(TARGET t81_c_frontend)
  list(APPEND T81_INSTALL_TARGETS t81_c_frontend)
endif()
if(TARGET t81_rust_frontend)
  list(APPEND T81_INSTALL_TARGETS t81_rust_frontend)
endif()
if(TARGET t81_python_frontend)
  list(APPEND T81_INSTALL_TARGETS t81_python_frontend)
endif()
if(T81_ENABLE_LLAMA_CPP)
  if(T81_EXPORT_LLAMA_ADAPTER)
    message(FATAL_ERROR
      "T81_EXPORT_LLAMA_ADAPTER=ON is not supported with vendored llama.cpp. "
      "t81_llama_adapter remains internal/build-only until dependency export "
      "and package-boundary policy are formalized.")
  endif()
  message(STATUS
    "t81_llama_adapter remains internal/build-only (not exported in install targets)")
endif()

install(TARGETS ${T81_INSTALL_TARGETS}
  EXPORT T81Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY include/t81 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install export file
install(EXPORT T81Targets
  FILE T81Targets.cmake
  NAMESPACE T81::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/T81
)

# Provide a second export namespace root for external consumers using
# the package name "T81Foundation".
install(EXPORT T81Targets
  FILE T81FoundationTargets.cmake
  NAMESPACE T81::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/T81Foundation
)

# Generate config file
configure_package_config_file(
  cmake/T81Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/T81Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/T81
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/T81ConfigVersion.cmake"
  VERSION ${T81_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  cmake/T81FoundationConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/T81FoundationConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/T81Foundation
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/T81FoundationConfigVersion.cmake"
  VERSION ${T81_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/T81Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/T81ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/T81
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/T81FoundationConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/T81FoundationConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/T81Foundation
)

if(T81_BUILD_TESTS)
  add_executable(test_bigint_demotion tests/cpp/test_bigint_demotion.cpp)
  target_link_libraries(test_bigint_demotion PRIVATE t81_core)
  add_test(NAME test_bigint_demotion COMMAND test_bigint_demotion)
endif()
add_executable(t81_graph_algorithms_test tests/cpp/test_T81Graph_algorithms.cpp)
target_link_libraries(t81_graph_algorithms_test PRIVATE t81_core)
add_test(NAME t81_graph_algorithms_test COMMAND t81_graph_algorithms_test)

  # Determinism Stress Tests
  # RFC-0032 Phase 1 — codec determinism gate tests
  add_executable(t81_determinism_codec_test tests/determinism/codec/ternary_codec_test.cpp)
  target_link_libraries(t81_determinism_codec_test PRIVATE t81_math_quantization)
  add_test(NAME t81_determinism_codec_test COMMAND t81_determinism_codec_test)

  add_executable(t81_determinism_primitives_test tests/determinism/test_primitives.cpp)
  target_link_libraries(t81_determinism_primitives_test PRIVATE t81_core)
  add_test(NAME t81_determinism_primitives_test COMMAND t81_determinism_primitives_test)

  add_executable(t81_determinism_float_test tests/determinism/test_float.cpp)
  target_link_libraries(t81_determinism_float_test PRIVATE t81_core)
  add_test(NAME t81_determinism_float_test COMMAND t81_determinism_float_test)

  add_executable(t81_determinism_containers_test tests/determinism/test_containers.cpp)
  target_link_libraries(t81_determinism_containers_test PRIVATE t81_core)
  add_test(NAME t81_determinism_containers_test COMMAND t81_determinism_containers_test)

  add_executable(t81_determinism_math_test tests/determinism/test_math.cpp)
  target_link_libraries(t81_determinism_math_test PRIVATE t81_core)
  add_test(NAME t81_determinism_math_test COMMAND t81_determinism_math_test)

  add_executable(t81_determinism_vector_test tests/determinism/test_vector.cpp)
  target_link_libraries(t81_determinism_vector_test PRIVATE t81_core)
  add_test(NAME t81_determinism_vector_test COMMAND t81_determinism_vector_test)

  add_executable(t81_determinism_new_types_test tests/determinism/test_new_types.cpp)
  target_link_libraries(t81_determinism_new_types_test PRIVATE t81_core)
  add_test(NAME t81_determinism_new_types_test COMMAND t81_determinism_new_types_test)

  # RFC-0032 diagnostics (float-domain; gated; NOT part of the deterministic core)
  if(T81_BUILD_DIAGNOSTICS)
    add_executable(t81_codec_metrics_tool tools/diagnostics/ternary_codec_metrics.cpp)
    target_link_libraries(t81_codec_metrics_tool PRIVATE t81_math_quantization)
    target_compile_definitions(t81_codec_metrics_tool PRIVATE T81_BUILD_DIAGNOSTICS=1)
  endif()

  # Repository Hardening Deterministic Tests
  add_executable(test_deterministic_float_boundaries tests/cpp/test_deterministic_float_boundaries.cpp)
  target_link_libraries(test_deterministic_float_boundaries PRIVATE t81_core)
  add_test(NAME test_deterministic_float_boundaries COMMAND test_deterministic_float_boundaries)

  # RFC-0030: Deterministic Math Subsystem acceptance test
  add_executable(test_t81float_soft_math tests/cpp/test_t81float_soft_math.cpp)
  target_link_libraries(test_t81float_soft_math PRIVATE t81_core)
  add_test(NAME test_t81float_soft_math COMMAND test_t81float_soft_math)

  add_executable(test_map_determinism tests/cpp/test_map_determinism.cpp)
  target_link_libraries(test_map_determinism PRIVATE t81_core)
  add_test(NAME test_map_determinism COMMAND test_map_determinism)

  add_executable(test_t81lang_traceability_enforcement tests/cpp/test_t81lang_traceability_enforcement.cpp)
  target_link_libraries(test_t81lang_traceability_enforcement PRIVATE t81_core t81_isa t81_vm t81_tool_cli)
  add_test(NAME test_t81lang_traceability_enforcement COMMAND test_t81lang_traceability_enforcement)

  add_executable(test_axion_evidence_loop_closure_corrected tests/cpp/test_axion_evidence_loop_closure_corrected.cpp)
  target_link_libraries(test_axion_evidence_loop_closure_corrected PRIVATE t81_core t81_isa t81_vm t81_tool_cli t81_axion)
  add_test(NAME test_axion_evidence_loop_closure_corrected COMMAND test_axion_evidence_loop_closure_corrected)
