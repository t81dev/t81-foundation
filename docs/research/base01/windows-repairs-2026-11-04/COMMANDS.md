# Recorded correctness commands

Source: `e09540caeb826df7eaeb9e876be5beb745784921`.
Exact argument vectors and working directories follow. Full output, compiler/link commands, caches and controller sources are archived. Build-tool timing text is incidental, not a performance sample.

## base01-ubuntu-24.04-Debug

```json
{
  "label": "source-head",
  "argv": [
    "git",
    "rev-parse",
    "HEAD"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "compiler",
  "argv": [
    "g++-14",
    "--version"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "cmake-version",
  "argv": [
    "cmake",
    "--version"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "ninja-version",
  "argv": [
    "ninja",
    "--version"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "configure",
  "argv": [
    "cmake",
    "-S",
    "/home/runner/work/t81-foundation/t81-foundation/source",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_C_COMPILER=gcc-14",
    "-DCMAKE_CXX_COMPILER=g++-14",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DT81_BUILD_BENCHMARKS=OFF",
    "-DT81_BUILD_TUI=OFF",
    "-DT81_BUILD_EXAMPLES=OFF",
    "-DT81_BUILD_FUZZ_TESTS=OFF",
    "-DT81_ENABLE_TERNARYOS=OFF",
    "-DT81_ENABLE_DPE=OFF",
    "-DT81_ENABLE_LLAMA_CPP=OFF",
    "-DT81_BUILD_DIAGNOSTICS=OFF"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "build-core",
  "argv": [
    "cmake",
    "--build",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "t81_sha3_test",
    "test_deterministic_float_boundaries",
    "test_map_determinism",
    "t81_determinism_containers_test",
    "t81_determinism_codec_test",
    "t81_determinism_primitives_test",
    "t81_determinism_float_test",
    "t81_determinism_math_test",
    "t81_determinism_vector_test",
    "t81_determinism_new_types_test",
    "test_t81float_soft_math",
    "t81_ternary_arith_test",
    "t81_canonfs_driver_test",
    "canonfs_read_verify_test",
    "canonfs_read_verify_env_contract_test",
    "canonfs_integrity_matrix_test",
    "canonfs_identity_contract_test",
    "t81_release_sha3_guard",
    "t81_release_float_guard",
    "t81_release_map_guard"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_sha3_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_sha3_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_deterministic_float_boundaries",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_deterministic_float_boundaries"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_map_determinism",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_map_determinism"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_containers_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_containers_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_codec_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_codec_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_primitives_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_primitives_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_float_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_float_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_math_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_math_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_vector_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_vector_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_new_types_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_new_types_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_t81float_soft_math",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_t81float_soft_math"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_ternary_arith_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_ternary_arith_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_canonfs_driver_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_canonfs_driver_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_read_verify_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_env_contract_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_read_verify_env_contract_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_integrity_matrix_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_integrity_matrix_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_identity_contract_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_identity_contract_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "guard-sha3",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "sha3",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_sha3_guard"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "guard-float",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "float",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_float_guard"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "guard-map",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "map",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_map_guard"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "mutants-configure",
  "argv": [
    "cmake",
    "-S",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutants",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_CXX_COMPILER=g++-14",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DPR_SOURCE=/home/runner/work/t81-foundation/t81-foundation/source",
    "-DCORE_LIBRARY=/home/runner/work/t81-foundation/t81-foundation/evidence/build/libt81_core.a"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "mutants-build",
  "argv": [
    "cmake",
    "--build",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build",
    "--verbose",
    "--parallel",
    "2"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "mutant-rejection-sha3",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "sha3",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_sha3"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 1
}
```

```json
{
  "label": "mutant-rejection-float",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "float",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_float"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 1
}
```

```json
{
  "label": "mutant-rejection-map",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "map",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_map"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 1
}
```

```json
{
  "label": "build-vm",
  "argv": [
    "cmake",
    "--build",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "t81_vm_determinism_property_test",
    "t81_vm_deterministic_fault_test",
    "t81_vm_fault_family_determinism_matrix_test",
    "t81_vm_tloadhash_decodefault_determinism_matrix_test",
    "t81_axion_log_determinism_test",
    "t81_vm_system_registers_deterministic_test",
    "t81_isa_binary_io_determinism_test",
    "t81_vm_workload_determinism_tiers_test",
    "test_resource_monitoring",
    "t81_determinism_evidence_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_determinism_property_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_determinism_property_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_deterministic_fault_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_deterministic_fault_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_fault_family_determinism_matrix_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_fault_family_determinism_matrix_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_tloadhash_decodefault_determinism_matrix_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_tloadhash_decodefault_determinism_matrix_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_axion_log_determinism_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_axion_log_determinism_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_system_registers_deterministic_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_system_registers_deterministic_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_isa_binary_io_determinism_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_isa_binary_io_determinism_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_workload_determinism_tiers_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_workload_determinism_tiers_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_resource_monitoring",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_resource_monitoring"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_evidence_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_evidence_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "build-repair-regressions",
  "argv": [
    "cmake",
    "--build",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "canonfs_storage_format_test",
    "t81_vm_metarefine_writecode_denial_test",
    "canonfs_interchange_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "repair-canonfs_storage_format_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_storage_format_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "repair-t81_vm_metarefine_writecode_denial_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_metarefine_writecode_denial_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "repair-canonfs_interchange_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_interchange_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

## base01-windows-2022-Release

```json
{
  "label": "source-head",
  "argv": [
    "git",
    "rev-parse",
    "HEAD"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "compiler",
  "argv": [
    "cl"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "cmake-version",
  "argv": [
    "cmake",
    "--version"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "ninja-version",
  "argv": [
    "ninja",
    "--version"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "configure",
  "argv": [
    "cmake",
    "-S",
    "D:\\a\\t81-foundation\\t81-foundation\\source",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_C_COMPILER=cl",
    "-DCMAKE_CXX_COMPILER=cl",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DT81_BUILD_BENCHMARKS=OFF",
    "-DT81_BUILD_TUI=OFF",
    "-DT81_BUILD_EXAMPLES=OFF",
    "-DT81_BUILD_FUZZ_TESTS=OFF",
    "-DT81_ENABLE_TERNARYOS=OFF",
    "-DT81_ENABLE_DPE=OFF",
    "-DT81_ENABLE_LLAMA_CPP=OFF",
    "-DT81_BUILD_DIAGNOSTICS=OFF"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "msvc-debug-configuration",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_msvc_debug_configuration.py",
    "--source",
    "D:\\a\\t81-foundation\\t81-foundation\\source",
    "--cmake",
    "C:\\Program Files\\CMake\\bin\\cmake.EXE",
    "--compiler",
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Tools\\MSVC\\14.44.35207\\bin\\HostX64\\x64\\cl.EXE",
    "--generator",
    "Ninja"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "build-core",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "t81_sha3_test",
    "test_deterministic_float_boundaries",
    "test_map_determinism",
    "t81_determinism_containers_test",
    "t81_determinism_codec_test",
    "t81_determinism_primitives_test",
    "t81_determinism_float_test",
    "t81_determinism_math_test",
    "t81_determinism_vector_test",
    "t81_determinism_new_types_test",
    "test_t81float_soft_math",
    "t81_ternary_arith_test",
    "t81_canonfs_driver_test",
    "canonfs_read_verify_test",
    "canonfs_read_verify_env_contract_test",
    "canonfs_integrity_matrix_test",
    "canonfs_identity_contract_test",
    "t81_release_sha3_guard",
    "t81_release_float_guard",
    "t81_release_map_guard"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_sha3_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_sha3_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_deterministic_float_boundaries",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_deterministic_float_boundaries.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_map_determinism",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_map_determinism.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_containers_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_containers_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_codec_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_codec_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_primitives_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_primitives_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_float_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_float_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_math_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_math_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_vector_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_vector_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_new_types_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_new_types_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_t81float_soft_math",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_t81float_soft_math.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_ternary_arith_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_ternary_arith_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_canonfs_driver_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_canonfs_driver_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_read_verify_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_env_contract_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_read_verify_env_contract_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_integrity_matrix_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_integrity_matrix_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_identity_contract_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_identity_contract_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "guard-sha3",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "sha3",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_sha3_guard.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "guard-float",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "float",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_float_guard.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "guard-map",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "map",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_map_guard.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "mutants-configure",
  "argv": [
    "cmake",
    "-S",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutants",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_CXX_COMPILER=cl",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DPR_SOURCE=D:\\a\\t81-foundation\\t81-foundation\\source",
    "-DCORE_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_core.lib"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "mutants-build",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build",
    "--verbose",
    "--parallel",
    "2"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "mutant-rejection-sha3",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "sha3",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_sha3.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 1
}
```

```json
{
  "label": "mutant-rejection-float",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "float",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_float.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 1
}
```

```json
{
  "label": "mutant-rejection-map",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "map",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_map.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 1
}
```

```json
{
  "label": "build-vm",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "t81_vm_determinism_property_test",
    "t81_vm_deterministic_fault_test",
    "t81_vm_fault_family_determinism_matrix_test",
    "t81_vm_tloadhash_decodefault_determinism_matrix_test",
    "t81_axion_log_determinism_test",
    "t81_vm_system_registers_deterministic_test",
    "t81_isa_binary_io_determinism_test",
    "t81_vm_workload_determinism_tiers_test",
    "test_resource_monitoring",
    "t81_determinism_evidence_test"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_determinism_property_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_determinism_property_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_deterministic_fault_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_deterministic_fault_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_fault_family_determinism_matrix_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_fault_family_determinism_matrix_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_tloadhash_decodefault_determinism_matrix_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_tloadhash_decodefault_determinism_matrix_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_axion_log_determinism_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_axion_log_determinism_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_system_registers_deterministic_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_system_registers_deterministic_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_isa_binary_io_determinism_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_isa_binary_io_determinism_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_workload_determinism_tiers_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_workload_determinism_tiers_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_resource_monitoring",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_resource_monitoring.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_evidence_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_evidence_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "build-repair-regressions",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "canonfs_storage_format_test",
    "t81_vm_metarefine_writecode_denial_test",
    "canonfs_interchange_test"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "repair-canonfs_storage_format_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_storage_format_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "repair-t81_vm_metarefine_writecode_denial_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_metarefine_writecode_denial_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "repair-canonfs_interchange_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_interchange_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "diagnostics-configure",
  "argv": [
    "cmake",
    "-S",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostics",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_CXX_COMPILER=cl",
    "-DPR_SOURCE=D:\\a\\t81-foundation\\t81-foundation\\source",
    "-DAXION_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_axion.lib",
    "-DCORE_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_core.lib"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "diagnostics-build",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build",
    "--verbose"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "diagnostic-evidence",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build\\evidence_probe.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "diagnostic-path",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build\\path_probe.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

## base01-windows-2022-Debug

```json
{
  "label": "source-head",
  "argv": [
    "git",
    "rev-parse",
    "HEAD"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "compiler",
  "argv": [
    "cl"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "cmake-version",
  "argv": [
    "cmake",
    "--version"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "ninja-version",
  "argv": [
    "ninja",
    "--version"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "configure",
  "argv": [
    "cmake",
    "-S",
    "D:\\a\\t81-foundation\\t81-foundation\\source",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_C_COMPILER=cl",
    "-DCMAKE_CXX_COMPILER=cl",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DT81_BUILD_BENCHMARKS=OFF",
    "-DT81_BUILD_TUI=OFF",
    "-DT81_BUILD_EXAMPLES=OFF",
    "-DT81_BUILD_FUZZ_TESTS=OFF",
    "-DT81_ENABLE_TERNARYOS=OFF",
    "-DT81_ENABLE_DPE=OFF",
    "-DT81_ENABLE_LLAMA_CPP=OFF",
    "-DT81_BUILD_DIAGNOSTICS=OFF"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "msvc-debug-configuration",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_msvc_debug_configuration.py",
    "--source",
    "D:\\a\\t81-foundation\\t81-foundation\\source",
    "--cmake",
    "C:\\Program Files\\CMake\\bin\\cmake.EXE",
    "--compiler",
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Tools\\MSVC\\14.44.35207\\bin\\HostX64\\x64\\cl.EXE",
    "--generator",
    "Ninja"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "build-core",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "t81_sha3_test",
    "test_deterministic_float_boundaries",
    "test_map_determinism",
    "t81_determinism_containers_test",
    "t81_determinism_codec_test",
    "t81_determinism_primitives_test",
    "t81_determinism_float_test",
    "t81_determinism_math_test",
    "t81_determinism_vector_test",
    "t81_determinism_new_types_test",
    "test_t81float_soft_math",
    "t81_ternary_arith_test",
    "t81_canonfs_driver_test",
    "canonfs_read_verify_test",
    "canonfs_read_verify_env_contract_test",
    "canonfs_integrity_matrix_test",
    "canonfs_identity_contract_test",
    "t81_release_sha3_guard",
    "t81_release_float_guard",
    "t81_release_map_guard"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_sha3_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_sha3_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_deterministic_float_boundaries",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_deterministic_float_boundaries.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_map_determinism",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_map_determinism.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_containers_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_containers_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_codec_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_codec_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_primitives_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_primitives_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_float_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_float_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_math_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_math_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_vector_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_vector_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_new_types_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_new_types_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_t81float_soft_math",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_t81float_soft_math.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_ternary_arith_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_ternary_arith_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_canonfs_driver_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_canonfs_driver_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_read_verify_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_env_contract_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_read_verify_env_contract_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_integrity_matrix_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_integrity_matrix_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_identity_contract_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_identity_contract_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "guard-sha3",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "sha3",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_sha3_guard.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "guard-float",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "float",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_float_guard.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "guard-map",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "map",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_map_guard.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "mutants-configure",
  "argv": [
    "cmake",
    "-S",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutants",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_CXX_COMPILER=cl",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DPR_SOURCE=D:\\a\\t81-foundation\\t81-foundation\\source",
    "-DCORE_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_core.lib"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "mutants-build",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build",
    "--verbose",
    "--parallel",
    "2"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "mutant-rejection-sha3",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "sha3",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_sha3.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 1
}
```

```json
{
  "label": "mutant-rejection-float",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "float",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_float.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 1
}
```

```json
{
  "label": "mutant-rejection-map",
  "argv": [
    "C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py",
    "map",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_map.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 1
}
```

```json
{
  "label": "build-vm",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "t81_vm_determinism_property_test",
    "t81_vm_deterministic_fault_test",
    "t81_vm_fault_family_determinism_matrix_test",
    "t81_vm_tloadhash_decodefault_determinism_matrix_test",
    "t81_axion_log_determinism_test",
    "t81_vm_system_registers_deterministic_test",
    "t81_isa_binary_io_determinism_test",
    "t81_vm_workload_determinism_tiers_test",
    "test_resource_monitoring",
    "t81_determinism_evidence_test"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_determinism_property_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_determinism_property_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_deterministic_fault_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_deterministic_fault_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_fault_family_determinism_matrix_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_fault_family_determinism_matrix_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_tloadhash_decodefault_determinism_matrix_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_tloadhash_decodefault_determinism_matrix_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_axion_log_determinism_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_axion_log_determinism_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_system_registers_deterministic_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_system_registers_deterministic_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_isa_binary_io_determinism_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_isa_binary_io_determinism_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_workload_determinism_tiers_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_workload_determinism_tiers_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_resource_monitoring",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_resource_monitoring.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_evidence_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_evidence_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "build-repair-regressions",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "canonfs_storage_format_test",
    "t81_vm_metarefine_writecode_denial_test",
    "canonfs_interchange_test"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "repair-canonfs_storage_format_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_storage_format_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "repair-t81_vm_metarefine_writecode_denial_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_metarefine_writecode_denial_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "repair-canonfs_interchange_test",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_interchange_test.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "diagnostics-configure",
  "argv": [
    "cmake",
    "-S",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostics",
    "-B",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_CXX_COMPILER=cl",
    "-DPR_SOURCE=D:\\a\\t81-foundation\\t81-foundation\\source",
    "-DAXION_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_axion.lib",
    "-DCORE_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_core.lib"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "diagnostics-build",
  "argv": [
    "cmake",
    "--build",
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build",
    "--verbose"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "diagnostic-evidence",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build\\evidence_probe.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

```json
{
  "label": "diagnostic-path",
  "argv": [
    "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build\\path_probe.exe"
  ],
  "cwd": "D:\\a\\t81-foundation\\t81-foundation\\source",
  "returncode": 0
}
```

## base01-ubuntu-24.04-Release

```json
{
  "label": "source-head",
  "argv": [
    "git",
    "rev-parse",
    "HEAD"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "compiler",
  "argv": [
    "g++-14",
    "--version"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "cmake-version",
  "argv": [
    "cmake",
    "--version"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "ninja-version",
  "argv": [
    "ninja",
    "--version"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "configure",
  "argv": [
    "cmake",
    "-S",
    "/home/runner/work/t81-foundation/t81-foundation/source",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_C_COMPILER=gcc-14",
    "-DCMAKE_CXX_COMPILER=g++-14",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DT81_BUILD_BENCHMARKS=OFF",
    "-DT81_BUILD_TUI=OFF",
    "-DT81_BUILD_EXAMPLES=OFF",
    "-DT81_BUILD_FUZZ_TESTS=OFF",
    "-DT81_ENABLE_TERNARYOS=OFF",
    "-DT81_ENABLE_DPE=OFF",
    "-DT81_ENABLE_LLAMA_CPP=OFF",
    "-DT81_BUILD_DIAGNOSTICS=OFF"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "build-core",
  "argv": [
    "cmake",
    "--build",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "t81_sha3_test",
    "test_deterministic_float_boundaries",
    "test_map_determinism",
    "t81_determinism_containers_test",
    "t81_determinism_codec_test",
    "t81_determinism_primitives_test",
    "t81_determinism_float_test",
    "t81_determinism_math_test",
    "t81_determinism_vector_test",
    "t81_determinism_new_types_test",
    "test_t81float_soft_math",
    "t81_ternary_arith_test",
    "t81_canonfs_driver_test",
    "canonfs_read_verify_test",
    "canonfs_read_verify_env_contract_test",
    "canonfs_integrity_matrix_test",
    "canonfs_identity_contract_test",
    "t81_release_sha3_guard",
    "t81_release_float_guard",
    "t81_release_map_guard"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_sha3_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_sha3_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_deterministic_float_boundaries",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_deterministic_float_boundaries"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_map_determinism",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_map_determinism"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_containers_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_containers_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_codec_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_codec_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_primitives_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_primitives_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_float_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_float_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_math_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_math_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_vector_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_vector_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_new_types_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_new_types_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_t81float_soft_math",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_t81float_soft_math"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_ternary_arith_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_ternary_arith_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_canonfs_driver_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_canonfs_driver_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_read_verify_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_env_contract_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_read_verify_env_contract_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_integrity_matrix_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_integrity_matrix_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_identity_contract_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_identity_contract_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "guard-sha3",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "sha3",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_sha3_guard"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "guard-float",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "float",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_float_guard"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "guard-map",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "map",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_map_guard"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "mutants-configure",
  "argv": [
    "cmake",
    "-S",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutants",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build",
    "-G",
    "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_CXX_COMPILER=g++-14",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DPR_SOURCE=/home/runner/work/t81-foundation/t81-foundation/source",
    "-DCORE_LIBRARY=/home/runner/work/t81-foundation/t81-foundation/evidence/build/libt81_core.a"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "mutants-build",
  "argv": [
    "cmake",
    "--build",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build",
    "--verbose",
    "--parallel",
    "2"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "mutant-rejection-sha3",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "sha3",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_sha3"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 1
}
```

```json
{
  "label": "mutant-rejection-float",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "float",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_float"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 1
}
```

```json
{
  "label": "mutant-rejection-map",
  "argv": [
    "/opt/hostedtoolcache/Python/3.12.15/x64/bin/python",
    "-B",
    "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py",
    "map",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_map"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 1
}
```

```json
{
  "label": "build-vm",
  "argv": [
    "cmake",
    "--build",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "t81_vm_determinism_property_test",
    "t81_vm_deterministic_fault_test",
    "t81_vm_fault_family_determinism_matrix_test",
    "t81_vm_tloadhash_decodefault_determinism_matrix_test",
    "t81_axion_log_determinism_test",
    "t81_vm_system_registers_deterministic_test",
    "t81_isa_binary_io_determinism_test",
    "t81_vm_workload_determinism_tiers_test",
    "test_resource_monitoring",
    "t81_determinism_evidence_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_determinism_property_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_determinism_property_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_deterministic_fault_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_deterministic_fault_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_fault_family_determinism_matrix_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_fault_family_determinism_matrix_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_tloadhash_decodefault_determinism_matrix_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_tloadhash_decodefault_determinism_matrix_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_axion_log_determinism_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_axion_log_determinism_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_system_registers_deterministic_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_system_registers_deterministic_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_isa_binary_io_determinism_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_isa_binary_io_determinism_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_workload_determinism_tiers_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_workload_determinism_tiers_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-test_resource_monitoring",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_resource_monitoring"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_evidence_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_evidence_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "build-repair-regressions",
  "argv": [
    "cmake",
    "--build",
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build",
    "--verbose",
    "--parallel",
    "2",
    "--target",
    "canonfs_storage_format_test",
    "t81_vm_metarefine_writecode_denial_test",
    "canonfs_interchange_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "repair-canonfs_storage_format_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_storage_format_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "repair-t81_vm_metarefine_writecode_denial_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_metarefine_writecode_denial_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

```json
{
  "label": "repair-canonfs_interchange_test",
  "argv": [
    "/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_interchange_test"
  ],
  "cwd": "/home/runner/work/t81-foundation/t81-foundation/source",
  "returncode": 0
}
```

## macOS-Release

```json
{
  "label": "compiler",
  "argv": [
    "/usr/bin/c++",
    "--version"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "system",
  "argv": [
    "sw_vers"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "cmake-version",
  "argv": [
    "cmake",
    "--version"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "ninja-version",
  "argv": [
    "ninja",
    "--version"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "configure",
  "argv": [
    "cmake",
    "-S",
    "/private/tmp/t81-windows-repairs",
    "-B",
    "/private/tmp/t81-windows-repair-build",
    "-G",
    "Ninja",
    "-DCMAKE_CXX_COMPILER=/usr/bin/c++",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DT81_BUILD_BENCHMARKS=OFF",
    "-DT81_BUILD_TUI=OFF",
    "-DT81_BUILD_EXAMPLES=OFF",
    "-DT81_BUILD_FUZZ_TESTS=OFF",
    "-DT81_ENABLE_TERNARYOS=OFF",
    "-DT81_ENABLE_DPE=OFF",
    "-DFETCHCONTENT_SOURCE_DIR_ASIO=/private/tmp/t81-base01-execution/asio"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "build",
  "argv": [
    "cmake",
    "--build",
    "/private/tmp/t81-windows-repair-build",
    "--verbose",
    "--parallel",
    "4",
    "--target",
    "t81_sha3_test",
    "test_deterministic_float_boundaries",
    "test_map_determinism",
    "t81_determinism_containers_test",
    "t81_determinism_codec_test",
    "t81_determinism_primitives_test",
    "t81_determinism_float_test",
    "t81_determinism_math_test",
    "t81_determinism_vector_test",
    "t81_determinism_new_types_test",
    "test_t81float_soft_math",
    "t81_ternary_arith_test",
    "t81_canonfs_driver_test",
    "canonfs_read_verify_test",
    "canonfs_read_verify_env_contract_test",
    "canonfs_integrity_matrix_test",
    "canonfs_identity_contract_test",
    "t81_vm_determinism_property_test",
    "t81_vm_deterministic_fault_test",
    "t81_vm_fault_family_determinism_matrix_test",
    "t81_vm_tloadhash_decodefault_determinism_matrix_test",
    "t81_axion_log_determinism_test",
    "t81_vm_system_registers_deterministic_test",
    "t81_isa_binary_io_determinism_test",
    "t81_vm_workload_determinism_tiers_test",
    "test_resource_monitoring",
    "t81_determinism_evidence_test",
    "canonfs_storage_format_test",
    "t81_vm_metarefine_writecode_denial_test",
    "canonfs_interchange_test",
    "t81_release_sha3_guard",
    "t81_release_float_guard",
    "t81_release_map_guard",
    "t81"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_sha3_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_sha3_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-test_deterministic_float_boundaries",
  "argv": [
    "/private/tmp/t81-windows-repair-build/test_deterministic_float_boundaries"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-test_map_determinism",
  "argv": [
    "/private/tmp/t81-windows-repair-build/test_map_determinism"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_containers_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_determinism_containers_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_codec_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_determinism_codec_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_primitives_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_determinism_primitives_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_float_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_determinism_float_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_math_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_determinism_math_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_vector_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_determinism_vector_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_new_types_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_determinism_new_types_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-test_t81float_soft_math",
  "argv": [
    "/private/tmp/t81-windows-repair-build/test_t81float_soft_math"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_ternary_arith_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_ternary_arith_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_canonfs_driver_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_canonfs_driver_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/canonfs_read_verify_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_read_verify_env_contract_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/canonfs_read_verify_env_contract_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_integrity_matrix_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/canonfs_integrity_matrix_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-canonfs_identity_contract_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/canonfs_identity_contract_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_determinism_property_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_vm_determinism_property_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_deterministic_fault_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_vm_deterministic_fault_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_fault_family_determinism_matrix_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_vm_fault_family_determinism_matrix_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_tloadhash_decodefault_determinism_matrix_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_vm_tloadhash_decodefault_determinism_matrix_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_axion_log_determinism_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_axion_log_determinism_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_system_registers_deterministic_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_vm_system_registers_deterministic_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_isa_binary_io_determinism_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_isa_binary_io_determinism_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_vm_workload_determinism_tiers_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_vm_workload_determinism_tiers_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-test_resource_monitoring",
  "argv": [
    "/private/tmp/t81-windows-repair-build/test_resource_monitoring"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "test-t81_determinism_evidence_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_determinism_evidence_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "extra-canonfs_storage_format_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/canonfs_storage_format_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "extra-t81_vm_metarefine_writecode_denial_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81_vm_metarefine_writecode_denial_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "extra-canonfs_interchange_test",
  "argv": [
    "/private/tmp/t81-windows-repair-build/canonfs_interchange_test"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "guard-sha3",
  "argv": [
    "/opt/homebrew/opt/python@3.14/bin/python3.14",
    "-B",
    "/private/tmp/t81-windows-repairs/tests/python/check_release_correctness_guard.py",
    "sha3",
    "/private/tmp/t81-windows-repair-build/t81_release_sha3_guard"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "guard-float",
  "argv": [
    "/opt/homebrew/opt/python@3.14/bin/python3.14",
    "-B",
    "/private/tmp/t81-windows-repairs/tests/python/check_release_correctness_guard.py",
    "float",
    "/private/tmp/t81-windows-repair-build/t81_release_float_guard"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "guard-map",
  "argv": [
    "/opt/homebrew/opt/python@3.14/bin/python3.14",
    "-B",
    "/private/tmp/t81-windows-repairs/tests/python/check_release_correctness_guard.py",
    "map",
    "/private/tmp/t81-windows-repair-build/t81_release_map_guard"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

## macOS-snapshot

```json
{
  "label": "put",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81",
    "canonfs",
    "put-file",
    "/private/tmp/t81-windows-repair-evidence/macOS-snapshot/payload",
    "--canonfs-root",
    "/private/tmp/t81-windows-repair-evidence/macOS-snapshot/store"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "get",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81",
    "canonfs",
    "get",
    "sha3-256:48<PEaur−f>≠pHqcsSkFH<b7−Szzb3Es≥Ij-pq3fv",
    "--out",
    "/private/tmp/t81-windows-repair-evidence/macOS-snapshot/returned",
    "--canonfs-root",
    "/private/tmp/t81-windows-repair-evidence/macOS-snapshot/store",
    "--json"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```

```json
{
  "label": "snapshot",
  "argv": [
    "/private/tmp/t81-windows-repair-build/t81",
    "canonfs",
    "snapshot",
    "--canonfs-root",
    "/private/tmp/t81-windows-repair-evidence/macOS-snapshot/store",
    "--json"
  ],
  "cwd": "/private/tmp/t81-windows-repairs",
  "returncode": 0
}
```
