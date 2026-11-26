cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
project(t81 LANGUAGES C CXX ASM)

# ---------- Version ----------
set(T81_VERSION_MAJOR 2)
set(T81_VERSION_MINOR 2)
set(T81_VERSION_PATCH 0)
set(T81_VERSION "${T81_VERSION_MAJOR}.${T81_VERSION_MINOR}.${T81_VERSION_PATCH}")

# Generate version header
configure_file(
  include/t81/config.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/t81/config.hpp
  @ONLY
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Options.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Platform.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Compiler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Int128.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Sanitizers.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake)
add_subdirectory(src)

# Opt-in research instrumentation; no production decoder or policy changes.
option(T81_BUILD_INVESTIGATION08_PROBE "Build Investigation 08 differential tensor probe" OFF)
if(T81_BUILD_INVESTIGATION08_PROBE)
  add_executable(t81_investigation08_probe experiments/integrated_system_advantage/parity_probe.cpp)
  target_include_directories(t81_investigation08_probe PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(t81_investigation08_probe PRIVATE t81_decode t81_tool_cli t81_vm t81_core)
endif()

option(T81_BUILD_ARCHITECTURE_CHALLENGE "Build isolated inventory architecture comparison" OFF)
if(T81_BUILD_ARCHITECTURE_CHALLENGE)
  add_executable(t81_architecture_arm_a experiments/architecture_challenge/arm_a.cpp)
  target_link_libraries(t81_architecture_arm_a PRIVATE t81_vm t81_axion t81_core)
endif()

# Opt-in research experiment harness; no production arithmetic or decoder changes.
option(T81_BUILD_EXPERIMENTS "Build isolated research experiments" OFF)

if(T81_BUILD_PROVENANCE_EXPERIMENT)
  if(NOT T81_STRICT_DETERMINISTIC_FLOAT)
    message(FATAL_ERROR "Provenance experiment requires T81_STRICT_DETERMINISTIC_FLOAT=ON")
  endif()
  add_executable(t81_provenance_experiment experiments/deterministic_provenance/runner.cpp)
  target_link_libraries(t81_provenance_experiment PRIVATE t81_vm t81_axion t81_core)
  if(T81_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME deterministic_provenance_experiment
      COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/experiments/deterministic_provenance/test_experiment.py
        --runner $<TARGET_FILE:t81_provenance_experiment>)
    set_tests_properties(deterministic_provenance_experiment PROPERTIES TIMEOUT 120)
    add_test(NAME float_conversion_exact_reference
      COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/experiments/float_conversion_audit/test_oracle.py)
    set_tests_properties(float_conversion_exact_reference PROPERTIES TIMEOUT 60)
  endif()
endif()

if(T81_BUILD_OBJECT_INTERFACE_EXPERIMENT)
  add_executable(t81_object_comparison examples/research/object-interface/runner.cpp)
  target_link_libraries(t81_object_comparison PRIVATE t81_vm t81_isa t81_tool_model t81_axion)
  if(T81_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME object_interface_comparison
      COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/examples/research/object-interface/compare.py
        --runner $<TARGET_FILE:t81_object_comparison>)
    set_tests_properties(object_interface_comparison PROPERTIES TIMEOUT 60)
  endif()
endif()

if(T81_BUILD_DESCRIPTOR_EXPERIMENT)
  add_executable(t81_descriptor_comparison examples/research/validated-descriptors/runner.cpp)
  target_link_libraries(t81_descriptor_comparison PRIVATE t81_vm t81_isa t81_tool_model t81_axion)
  if(NOT T81_BUILD_OBJECT_INTERFACE_EXPERIMENT)
    message(FATAL_ERROR "Descriptor comparison requires T81_BUILD_OBJECT_INTERFACE_EXPERIMENT=ON for arm A")
  endif()
  if(T81_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME validated_descriptor_comparison
      COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/examples/research/validated-descriptors/compare.py
        --runner $<TARGET_FILE:t81_descriptor_comparison>
        --baseline-runner $<TARGET_FILE:t81_object_comparison>)
    set_tests_properties(validated_descriptor_comparison PROPERTIES TIMEOUT 60)
  endif()
endif()

# =============================================================================
# TernaryOS — HAL, shell, and user environment (RFC-00B0 through RFC-00C6)
# Stable; disable with -DT81_ENABLE_TERNARYOS=OFF
# =============================================================================
if(T81_ENABLE_TERNARYOS)
  add_subdirectory(ternaryos)
endif()

if(T81_BUILD_TESTS OR T81_BUILD_EXAMPLES)
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
endif()

if(T81_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# =============================================================================
# Tests (modularized into tests/CMakeLists.txt)
# =============================================================================
if(T81_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# =============================================================================
# Python Bindings
# =============================================================================
if(NOT T81_BUILD_PYTHON_BINDINGS)
  # Auto-detect when pybind11 is already installed in the active interpreter
  execute_process(
      COMMAND python3 -c "import pybind11; print(pybind11.get_cmake_dir())"
      OUTPUT_VARIABLE pybind11_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
  )
  if(pybind11_DIR)
    set(T81_BUILD_PYTHON_BINDINGS ON)
  endif()
endif()

if(T81_BUILD_PYTHON_BINDINGS)
  set(PYBIND11_FINDPYTHON ON)
  find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
  find_package(pybind11 REQUIRED)
  pybind11_add_module(_t81 lang/python/t81_python.cpp)
  target_link_libraries(_t81 PRIVATE t81_core t81_axion t81_lang_frontend t81_isa t81_vm)
  # Install the extension into the t81 package directory so scikit-build-core
  # picks it up alongside ffi/python/t81/__init__.py.
  install(TARGETS _t81 DESTINATION t81 COMPONENT python)
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
  t81_math_quantization
  t81_vm
  t81_experimental
  t81_llvm
  t81_mlir
  t81_tool_cli
  t81_decode
  t81_provider
  t81_crypto
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
install(DIRECTORY include/t81
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PATTERN "config.hpp.in" EXCLUDE
  PATTERN "config.hpp.backup" EXCLUDE
)

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


  # RFC-0032 diagnostics (float-domain; gated; NOT part of the deterministic core)
  if(T81_BUILD_DIAGNOSTICS)
    add_executable(t81_codec_metrics_tool tools/diagnostics/ternary_codec_metrics.cpp)
    target_link_libraries(t81_codec_metrics_tool PRIVATE t81_math_quantization)
    target_compile_definitions(t81_codec_metrics_tool PRIVATE T81_BUILD_DIAGNOSTICS=1)
  endif()

# =============================================================================
# In-browser Playground (Emscripten / Wasm)
# =============================================================================
if(T81_BUILD_PLAYGROUND)
  if(NOT EMSCRIPTEN)
    message(WARNING "T81_BUILD_PLAYGROUND requires Emscripten. "
                    "Re-configure with the Emscripten CMake toolchain file.")
  else()
    add_subdirectory(tools/playground)
  endif()
endif()
