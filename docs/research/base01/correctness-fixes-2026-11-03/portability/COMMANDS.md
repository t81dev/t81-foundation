# Exact native correctness commands

Final source: `35f73ac9751423f9ac083c7f4b84def34d810443`. These are recorded argument vectors, not reconstructed shell invocations. Working directories, native output, compiler/link commands and configuration metadata remain in each raw artifact ZIP inside evidence.tar.gz. Historical argument vectors remain in their unchanged ZIPs.

## final / base01-ubuntu-24.04-Debug

### source-head — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["git", "rev-parse", "HEAD"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `source-head.stdout` / `.stderr`

### compiler — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["g++-14", "--version"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `compiler.stdout` / `.stderr`

### cmake-version — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "--version"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `cmake-version.stdout` / `.stderr`

### ninja-version — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["ninja", "--version"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `ninja-version.stdout` / `.stderr`

### configure — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "-S", "/home/runner/work/t81-foundation/t81-foundation/source", "-B", "/home/runner/work/t81-foundation/t81-foundation/evidence/build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_C_COMPILER=gcc-14", "-DCMAKE_CXX_COMPILER=g++-14", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DT81_BUILD_BENCHMARKS=OFF", "-DT81_BUILD_TUI=OFF", "-DT81_BUILD_EXAMPLES=OFF", "-DT81_BUILD_FUZZ_TESTS=OFF", "-DT81_ENABLE_TERNARYOS=OFF", "-DT81_ENABLE_DPE=OFF", "-DT81_ENABLE_LLAMA_CPP=OFF", "-DT81_BUILD_DIAGNOSTICS=OFF"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `configure.stdout` / `.stderr`

### build-core — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "--build", "/home/runner/work/t81-foundation/t81-foundation/evidence/build", "--verbose", "--parallel", "2", "--target", "t81_sha3_test", "test_deterministic_float_boundaries", "test_map_determinism", "t81_determinism_containers_test", "t81_determinism_codec_test", "t81_determinism_primitives_test", "t81_determinism_float_test", "t81_determinism_math_test", "t81_determinism_vector_test", "t81_determinism_new_types_test", "test_t81float_soft_math", "t81_ternary_arith_test", "t81_canonfs_driver_test", "canonfs_read_verify_test", "canonfs_read_verify_env_contract_test", "canonfs_integrity_matrix_test", "canonfs_identity_contract_test", "t81_release_sha3_guard", "t81_release_float_guard", "t81_release_map_guard"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `build-core.stdout` / `.stderr`

### test-t81_sha3_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_sha3_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_sha3_test.stdout` / `.stderr`

### test-test_deterministic_float_boundaries — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_deterministic_float_boundaries"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-test_deterministic_float_boundaries.stdout` / `.stderr`

### test-test_map_determinism — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_map_determinism"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-test_map_determinism.stdout` / `.stderr`

### test-t81_determinism_containers_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_containers_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_determinism_containers_test.stdout` / `.stderr`

### test-t81_determinism_codec_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_codec_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_determinism_codec_test.stdout` / `.stderr`

### test-t81_determinism_primitives_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_primitives_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_determinism_primitives_test.stdout` / `.stderr`

### test-t81_determinism_float_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_float_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_determinism_float_test.stdout` / `.stderr`

### test-t81_determinism_math_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_math_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_determinism_math_test.stdout` / `.stderr`

### test-t81_determinism_vector_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_vector_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_determinism_vector_test.stdout` / `.stderr`

### test-t81_determinism_new_types_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_new_types_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_determinism_new_types_test.stdout` / `.stderr`

### test-test_t81float_soft_math — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_t81float_soft_math"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-test_t81float_soft_math.stdout` / `.stderr`

### test-t81_ternary_arith_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_ternary_arith_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_ternary_arith_test.stdout` / `.stderr`

### test-t81_canonfs_driver_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_canonfs_driver_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_canonfs_driver_test.stdout` / `.stderr`

### test-canonfs_read_verify_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_read_verify_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-canonfs_read_verify_test.stdout` / `.stderr`

### test-canonfs_read_verify_env_contract_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_read_verify_env_contract_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-canonfs_read_verify_env_contract_test.stdout` / `.stderr`

### test-canonfs_integrity_matrix_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_integrity_matrix_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-canonfs_integrity_matrix_test.stdout` / `.stderr`

### test-canonfs_identity_contract_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_identity_contract_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-canonfs_identity_contract_test.stdout` / `.stderr`

### guard-sha3 — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "sha3", "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_sha3_guard"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `guard-sha3.stdout` / `.stderr`

### guard-float — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "float", "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_float_guard"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `guard-float.stdout` / `.stderr`

### guard-map — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "map", "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_map_guard"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `guard-map.stdout` / `.stderr`

### mutants-configure — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "-S", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutants", "-B", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_CXX_COMPILER=g++-14", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DPR_SOURCE=/home/runner/work/t81-foundation/t81-foundation/source", "-DCORE_LIBRARY=/home/runner/work/t81-foundation/t81-foundation/evidence/build/libt81_core.a"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `mutants-configure.stdout` / `.stderr`

### mutants-build — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "--build", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build", "--verbose", "--parallel", "2"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `mutants-build.stdout` / `.stderr`

### mutant-rejection-sha3 — return code 1

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "sha3", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_sha3"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `mutant-rejection-sha3.stdout` / `.stderr`

### mutant-rejection-float — return code 1

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "float", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_float"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `mutant-rejection-float.stdout` / `.stderr`

### mutant-rejection-map — return code 1

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "map", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_map"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `mutant-rejection-map.stdout` / `.stderr`

### build-vm — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "--build", "/home/runner/work/t81-foundation/t81-foundation/evidence/build", "--verbose", "--parallel", "2", "--target", "t81_vm_determinism_property_test", "t81_vm_deterministic_fault_test", "t81_vm_fault_family_determinism_matrix_test", "t81_vm_tloadhash_decodefault_determinism_matrix_test", "t81_axion_log_determinism_test", "t81_vm_system_registers_deterministic_test", "t81_isa_binary_io_determinism_test", "t81_vm_workload_determinism_tiers_test", "test_resource_monitoring", "t81_determinism_evidence_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `build-vm.stdout` / `.stderr`

### test-t81_vm_determinism_property_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_determinism_property_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_vm_determinism_property_test.stdout` / `.stderr`

### test-t81_vm_deterministic_fault_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_deterministic_fault_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_vm_deterministic_fault_test.stdout` / `.stderr`

### test-t81_vm_fault_family_determinism_matrix_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_fault_family_determinism_matrix_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_vm_fault_family_determinism_matrix_test.stdout` / `.stderr`

### test-t81_vm_tloadhash_decodefault_determinism_matrix_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_tloadhash_decodefault_determinism_matrix_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_vm_tloadhash_decodefault_determinism_matrix_test.stdout` / `.stderr`

### test-t81_axion_log_determinism_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_axion_log_determinism_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_axion_log_determinism_test.stdout` / `.stderr`

### test-t81_vm_system_registers_deterministic_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_system_registers_deterministic_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_vm_system_registers_deterministic_test.stdout` / `.stderr`

### test-t81_isa_binary_io_determinism_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_isa_binary_io_determinism_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_isa_binary_io_determinism_test.stdout` / `.stderr`

### test-t81_vm_workload_determinism_tiers_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_workload_determinism_tiers_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_vm_workload_determinism_tiers_test.stdout` / `.stderr`

### test-test_resource_monitoring — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_resource_monitoring"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-test_resource_monitoring.stdout` / `.stderr`

### test-t81_determinism_evidence_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_evidence_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Debug.zip` → `test-t81_determinism_evidence_test.stdout` / `.stderr`

## final / base01-ubuntu-24.04-Release

### source-head — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["git", "rev-parse", "HEAD"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `source-head.stdout` / `.stderr`

### compiler — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["g++-14", "--version"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `compiler.stdout` / `.stderr`

### cmake-version — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "--version"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `cmake-version.stdout` / `.stderr`

### ninja-version — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["ninja", "--version"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `ninja-version.stdout` / `.stderr`

### configure — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "-S", "/home/runner/work/t81-foundation/t81-foundation/source", "-B", "/home/runner/work/t81-foundation/t81-foundation/evidence/build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_C_COMPILER=gcc-14", "-DCMAKE_CXX_COMPILER=g++-14", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DT81_BUILD_BENCHMARKS=OFF", "-DT81_BUILD_TUI=OFF", "-DT81_BUILD_EXAMPLES=OFF", "-DT81_BUILD_FUZZ_TESTS=OFF", "-DT81_ENABLE_TERNARYOS=OFF", "-DT81_ENABLE_DPE=OFF", "-DT81_ENABLE_LLAMA_CPP=OFF", "-DT81_BUILD_DIAGNOSTICS=OFF"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `configure.stdout` / `.stderr`

### build-core — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "--build", "/home/runner/work/t81-foundation/t81-foundation/evidence/build", "--verbose", "--parallel", "2", "--target", "t81_sha3_test", "test_deterministic_float_boundaries", "test_map_determinism", "t81_determinism_containers_test", "t81_determinism_codec_test", "t81_determinism_primitives_test", "t81_determinism_float_test", "t81_determinism_math_test", "t81_determinism_vector_test", "t81_determinism_new_types_test", "test_t81float_soft_math", "t81_ternary_arith_test", "t81_canonfs_driver_test", "canonfs_read_verify_test", "canonfs_read_verify_env_contract_test", "canonfs_integrity_matrix_test", "canonfs_identity_contract_test", "t81_release_sha3_guard", "t81_release_float_guard", "t81_release_map_guard"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `build-core.stdout` / `.stderr`

### test-t81_sha3_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_sha3_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_sha3_test.stdout` / `.stderr`

### test-test_deterministic_float_boundaries — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_deterministic_float_boundaries"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-test_deterministic_float_boundaries.stdout` / `.stderr`

### test-test_map_determinism — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_map_determinism"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-test_map_determinism.stdout` / `.stderr`

### test-t81_determinism_containers_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_containers_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_determinism_containers_test.stdout` / `.stderr`

### test-t81_determinism_codec_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_codec_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_determinism_codec_test.stdout` / `.stderr`

### test-t81_determinism_primitives_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_primitives_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_determinism_primitives_test.stdout` / `.stderr`

### test-t81_determinism_float_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_float_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_determinism_float_test.stdout` / `.stderr`

### test-t81_determinism_math_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_math_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_determinism_math_test.stdout` / `.stderr`

### test-t81_determinism_vector_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_vector_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_determinism_vector_test.stdout` / `.stderr`

### test-t81_determinism_new_types_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_new_types_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_determinism_new_types_test.stdout` / `.stderr`

### test-test_t81float_soft_math — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_t81float_soft_math"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-test_t81float_soft_math.stdout` / `.stderr`

### test-t81_ternary_arith_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_ternary_arith_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_ternary_arith_test.stdout` / `.stderr`

### test-t81_canonfs_driver_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_canonfs_driver_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_canonfs_driver_test.stdout` / `.stderr`

### test-canonfs_read_verify_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_read_verify_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-canonfs_read_verify_test.stdout` / `.stderr`

### test-canonfs_read_verify_env_contract_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_read_verify_env_contract_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-canonfs_read_verify_env_contract_test.stdout` / `.stderr`

### test-canonfs_integrity_matrix_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_integrity_matrix_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-canonfs_integrity_matrix_test.stdout` / `.stderr`

### test-canonfs_identity_contract_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/canonfs_identity_contract_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-canonfs_identity_contract_test.stdout` / `.stderr`

### guard-sha3 — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "sha3", "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_sha3_guard"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `guard-sha3.stdout` / `.stderr`

### guard-float — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "float", "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_float_guard"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `guard-float.stdout` / `.stderr`

### guard-map — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "map", "/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_release_map_guard"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `guard-map.stdout` / `.stderr`

### mutants-configure — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "-S", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutants", "-B", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_CXX_COMPILER=g++-14", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DPR_SOURCE=/home/runner/work/t81-foundation/t81-foundation/source", "-DCORE_LIBRARY=/home/runner/work/t81-foundation/t81-foundation/evidence/build/libt81_core.a"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `mutants-configure.stdout` / `.stderr`

### mutants-build — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "--build", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build", "--verbose", "--parallel", "2"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `mutants-build.stdout` / `.stderr`

### mutant-rejection-sha3 — return code 1

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "sha3", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_sha3"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `mutant-rejection-sha3.stdout` / `.stderr`

### mutant-rejection-float — return code 1

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "float", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_float"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `mutant-rejection-float.stdout` / `.stderr`

### mutant-rejection-map — return code 1

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/opt/hostedtoolcache/Python/3.12.15/x64/bin/python", "-B", "/home/runner/work/t81-foundation/t81-foundation/source/tests/python/check_release_correctness_guard.py", "map", "/home/runner/work/t81-foundation/t81-foundation/evidence/mutant-build/mutant_map"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `mutant-rejection-map.stdout` / `.stderr`

### build-vm — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["cmake", "--build", "/home/runner/work/t81-foundation/t81-foundation/evidence/build", "--verbose", "--parallel", "2", "--target", "t81_vm_determinism_property_test", "t81_vm_deterministic_fault_test", "t81_vm_fault_family_determinism_matrix_test", "t81_vm_tloadhash_decodefault_determinism_matrix_test", "t81_axion_log_determinism_test", "t81_vm_system_registers_deterministic_test", "t81_isa_binary_io_determinism_test", "t81_vm_workload_determinism_tiers_test", "test_resource_monitoring", "t81_determinism_evidence_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `build-vm.stdout` / `.stderr`

### test-t81_vm_determinism_property_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_determinism_property_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_vm_determinism_property_test.stdout` / `.stderr`

### test-t81_vm_deterministic_fault_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_deterministic_fault_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_vm_deterministic_fault_test.stdout` / `.stderr`

### test-t81_vm_fault_family_determinism_matrix_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_fault_family_determinism_matrix_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_vm_fault_family_determinism_matrix_test.stdout` / `.stderr`

### test-t81_vm_tloadhash_decodefault_determinism_matrix_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_tloadhash_decodefault_determinism_matrix_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_vm_tloadhash_decodefault_determinism_matrix_test.stdout` / `.stderr`

### test-t81_axion_log_determinism_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_axion_log_determinism_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_axion_log_determinism_test.stdout` / `.stderr`

### test-t81_vm_system_registers_deterministic_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_system_registers_deterministic_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_vm_system_registers_deterministic_test.stdout` / `.stderr`

### test-t81_isa_binary_io_determinism_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_isa_binary_io_determinism_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_isa_binary_io_determinism_test.stdout` / `.stderr`

### test-t81_vm_workload_determinism_tiers_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_vm_workload_determinism_tiers_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_vm_workload_determinism_tiers_test.stdout` / `.stderr`

### test-test_resource_monitoring — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/test_resource_monitoring"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-test_resource_monitoring.stdout` / `.stderr`

### test-t81_determinism_evidence_test — return code 0

Working directory: `/home/runner/work/t81-foundation/t81-foundation/source`

```text
["/home/runner/work/t81-foundation/t81-foundation/evidence/build/t81_determinism_evidence_test"]
```

Raw output: `final/artifacts/base01-ubuntu-24.04-Release.zip` → `test-t81_determinism_evidence_test.stdout` / `.stderr`

## final / base01-windows-2022-Release

### source-head — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["git", "rev-parse", "HEAD"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `source-head.stdout` / `.stderr`

### compiler — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cl"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `compiler.stdout` / `.stderr`

### cmake-version — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--version"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `cmake-version.stdout` / `.stderr`

### ninja-version — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["ninja", "--version"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `ninja-version.stdout` / `.stderr`

### configure — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "-S", "D:\\a\\t81-foundation\\t81-foundation\\source", "-B", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_C_COMPILER=cl", "-DCMAKE_CXX_COMPILER=cl", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DT81_BUILD_BENCHMARKS=OFF", "-DT81_BUILD_TUI=OFF", "-DT81_BUILD_EXAMPLES=OFF", "-DT81_BUILD_FUZZ_TESTS=OFF", "-DT81_ENABLE_TERNARYOS=OFF", "-DT81_ENABLE_DPE=OFF", "-DT81_ENABLE_LLAMA_CPP=OFF", "-DT81_BUILD_DIAGNOSTICS=OFF"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `configure.stdout` / `.stderr`

### msvc-debug-configuration — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_msvc_debug_configuration.py", "--source", "D:\\a\\t81-foundation\\t81-foundation\\source", "--cmake", "C:\\Program Files\\CMake\\bin\\cmake.EXE", "--compiler", "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Tools\\MSVC\\14.44.35207\\bin\\HostX64\\x64\\cl.EXE", "--generator", "Ninja"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `msvc-debug-configuration.stdout` / `.stderr`

### build-core — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build", "--verbose", "--parallel", "2", "--target", "t81_sha3_test", "test_deterministic_float_boundaries", "test_map_determinism", "t81_determinism_containers_test", "t81_determinism_codec_test", "t81_determinism_primitives_test", "t81_determinism_float_test", "t81_determinism_math_test", "t81_determinism_vector_test", "t81_determinism_new_types_test", "test_t81float_soft_math", "t81_ternary_arith_test", "t81_canonfs_driver_test", "canonfs_read_verify_test", "canonfs_read_verify_env_contract_test", "canonfs_integrity_matrix_test", "canonfs_identity_contract_test", "t81_release_sha3_guard", "t81_release_float_guard", "t81_release_map_guard"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `build-core.stdout` / `.stderr`

### test-t81_sha3_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_sha3_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_sha3_test.stdout` / `.stderr`

### test-test_deterministic_float_boundaries — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_deterministic_float_boundaries.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-test_deterministic_float_boundaries.stdout` / `.stderr`

### test-test_map_determinism — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_map_determinism.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-test_map_determinism.stdout` / `.stderr`

### test-t81_determinism_containers_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_containers_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_determinism_containers_test.stdout` / `.stderr`

### test-t81_determinism_codec_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_codec_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_determinism_codec_test.stdout` / `.stderr`

### test-t81_determinism_primitives_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_primitives_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_determinism_primitives_test.stdout` / `.stderr`

### test-t81_determinism_float_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_float_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_determinism_float_test.stdout` / `.stderr`

### test-t81_determinism_math_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_math_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_determinism_math_test.stdout` / `.stderr`

### test-t81_determinism_vector_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_vector_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_determinism_vector_test.stdout` / `.stderr`

### test-t81_determinism_new_types_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_new_types_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_determinism_new_types_test.stdout` / `.stderr`

### test-test_t81float_soft_math — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_t81float_soft_math.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-test_t81float_soft_math.stdout` / `.stderr`

### test-t81_ternary_arith_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_ternary_arith_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_ternary_arith_test.stdout` / `.stderr`

### test-t81_canonfs_driver_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_canonfs_driver_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_canonfs_driver_test.stdout` / `.stderr`

### test-canonfs_read_verify_test — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_read_verify_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-canonfs_read_verify_test.stdout` / `.stderr`

### test-canonfs_read_verify_env_contract_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_read_verify_env_contract_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-canonfs_read_verify_env_contract_test.stdout` / `.stderr`

### test-canonfs_integrity_matrix_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_integrity_matrix_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-canonfs_integrity_matrix_test.stdout` / `.stderr`

### test-canonfs_identity_contract_test — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_identity_contract_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-canonfs_identity_contract_test.stdout` / `.stderr`

### guard-sha3 — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "sha3", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_sha3_guard.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `guard-sha3.stdout` / `.stderr`

### guard-float — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "float", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_float_guard.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `guard-float.stdout` / `.stderr`

### guard-map — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "map", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_map_guard.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `guard-map.stdout` / `.stderr`

### mutants-configure — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "-S", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutants", "-B", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_CXX_COMPILER=cl", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DPR_SOURCE=D:\\a\\t81-foundation\\t81-foundation\\source", "-DCORE_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_core.lib"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `mutants-configure.stdout` / `.stderr`

### mutants-build — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build", "--verbose", "--parallel", "2"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `mutants-build.stdout` / `.stderr`

### mutant-rejection-sha3 — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "sha3", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_sha3.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `mutant-rejection-sha3.stdout` / `.stderr`

### mutant-rejection-float — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "float", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_float.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `mutant-rejection-float.stdout` / `.stderr`

### mutant-rejection-map — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "map", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_map.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `mutant-rejection-map.stdout` / `.stderr`

### build-vm — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build", "--verbose", "--parallel", "2", "--target", "t81_vm_determinism_property_test", "t81_vm_deterministic_fault_test", "t81_vm_fault_family_determinism_matrix_test", "t81_vm_tloadhash_decodefault_determinism_matrix_test", "t81_axion_log_determinism_test", "t81_vm_system_registers_deterministic_test", "t81_isa_binary_io_determinism_test", "t81_vm_workload_determinism_tiers_test", "test_resource_monitoring", "t81_determinism_evidence_test"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `build-vm.stdout` / `.stderr`

### test-t81_vm_determinism_property_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_determinism_property_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_vm_determinism_property_test.stdout` / `.stderr`

### test-t81_vm_deterministic_fault_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_deterministic_fault_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_vm_deterministic_fault_test.stdout` / `.stderr`

### test-t81_vm_fault_family_determinism_matrix_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_fault_family_determinism_matrix_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_vm_fault_family_determinism_matrix_test.stdout` / `.stderr`

### test-t81_vm_tloadhash_decodefault_determinism_matrix_test — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_tloadhash_decodefault_determinism_matrix_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_vm_tloadhash_decodefault_determinism_matrix_test.stdout` / `.stderr`

### test-t81_axion_log_determinism_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_axion_log_determinism_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_axion_log_determinism_test.stdout` / `.stderr`

### test-t81_vm_system_registers_deterministic_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_system_registers_deterministic_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_vm_system_registers_deterministic_test.stdout` / `.stderr`

### test-t81_isa_binary_io_determinism_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_isa_binary_io_determinism_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_isa_binary_io_determinism_test.stdout` / `.stderr`

### test-t81_vm_workload_determinism_tiers_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_vm_workload_determinism_tiers_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_vm_workload_determinism_tiers_test.stdout` / `.stderr`

### test-test_resource_monitoring — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_resource_monitoring.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-test_resource_monitoring.stdout` / `.stderr`

### test-t81_determinism_evidence_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_evidence_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `test-t81_determinism_evidence_test.stdout` / `.stderr`

### diagnostics-configure — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "-S", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostics", "-B", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_CXX_COMPILER=cl", "-DPR_SOURCE=D:\\a\\t81-foundation\\t81-foundation\\source", "-DAXION_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_axion.lib", "-DCORE_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_core.lib"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `diagnostics-configure.stdout` / `.stderr`

### diagnostics-build — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build", "--verbose"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `diagnostics-build.stdout` / `.stderr`

### diagnostic-evidence — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build\\evidence_probe.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `diagnostic-evidence.stdout` / `.stderr`

### diagnostic-path — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build\\path_probe.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Release.zip` → `diagnostic-path.stdout` / `.stderr`

## final / base01-windows-2022-Debug

### source-head — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["git", "rev-parse", "HEAD"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `source-head.stdout` / `.stderr`

### compiler — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cl"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `compiler.stdout` / `.stderr`

### cmake-version — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--version"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `cmake-version.stdout` / `.stderr`

### ninja-version — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["ninja", "--version"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `ninja-version.stdout` / `.stderr`

### configure — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "-S", "D:\\a\\t81-foundation\\t81-foundation\\source", "-B", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_C_COMPILER=cl", "-DCMAKE_CXX_COMPILER=cl", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DT81_BUILD_BENCHMARKS=OFF", "-DT81_BUILD_TUI=OFF", "-DT81_BUILD_EXAMPLES=OFF", "-DT81_BUILD_FUZZ_TESTS=OFF", "-DT81_ENABLE_TERNARYOS=OFF", "-DT81_ENABLE_DPE=OFF", "-DT81_ENABLE_LLAMA_CPP=OFF", "-DT81_BUILD_DIAGNOSTICS=OFF"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `configure.stdout` / `.stderr`

### msvc-debug-configuration — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_msvc_debug_configuration.py", "--source", "D:\\a\\t81-foundation\\t81-foundation\\source", "--cmake", "C:\\Program Files\\CMake\\bin\\cmake.EXE", "--compiler", "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Tools\\MSVC\\14.44.35207\\bin\\HostX64\\x64\\cl.EXE", "--generator", "Ninja"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `msvc-debug-configuration.stdout` / `.stderr`

### build-core — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build", "--verbose", "--parallel", "2", "--target", "t81_sha3_test", "test_deterministic_float_boundaries", "test_map_determinism", "t81_determinism_containers_test", "t81_determinism_codec_test", "t81_determinism_primitives_test", "t81_determinism_float_test", "t81_determinism_math_test", "t81_determinism_vector_test", "t81_determinism_new_types_test", "test_t81float_soft_math", "t81_ternary_arith_test", "t81_canonfs_driver_test", "canonfs_read_verify_test", "canonfs_read_verify_env_contract_test", "canonfs_integrity_matrix_test", "canonfs_identity_contract_test", "t81_release_sha3_guard", "t81_release_float_guard", "t81_release_map_guard"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `build-core.stdout` / `.stderr`

### test-t81_sha3_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_sha3_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_sha3_test.stdout` / `.stderr`

### test-test_deterministic_float_boundaries — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_deterministic_float_boundaries.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-test_deterministic_float_boundaries.stdout` / `.stderr`

### test-test_map_determinism — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_map_determinism.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-test_map_determinism.stdout` / `.stderr`

### test-t81_determinism_containers_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_containers_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_determinism_containers_test.stdout` / `.stderr`

### test-t81_determinism_codec_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_codec_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_determinism_codec_test.stdout` / `.stderr`

### test-t81_determinism_primitives_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_primitives_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_determinism_primitives_test.stdout` / `.stderr`

### test-t81_determinism_float_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_float_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_determinism_float_test.stdout` / `.stderr`

### test-t81_determinism_math_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_math_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_determinism_math_test.stdout` / `.stderr`

### test-t81_determinism_vector_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_vector_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_determinism_vector_test.stdout` / `.stderr`

### test-t81_determinism_new_types_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_new_types_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_determinism_new_types_test.stdout` / `.stderr`

### test-test_t81float_soft_math — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\test_t81float_soft_math.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-test_t81float_soft_math.stdout` / `.stderr`

### test-t81_ternary_arith_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_ternary_arith_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_ternary_arith_test.stdout` / `.stderr`

### test-t81_canonfs_driver_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_canonfs_driver_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_canonfs_driver_test.stdout` / `.stderr`

### test-canonfs_read_verify_test — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_read_verify_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-canonfs_read_verify_test.stdout` / `.stderr`

### test-canonfs_read_verify_env_contract_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_read_verify_env_contract_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-canonfs_read_verify_env_contract_test.stdout` / `.stderr`

### test-canonfs_integrity_matrix_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_integrity_matrix_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-canonfs_integrity_matrix_test.stdout` / `.stderr`

### test-canonfs_identity_contract_test — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\canonfs_identity_contract_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-canonfs_identity_contract_test.stdout` / `.stderr`

### guard-sha3 — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "sha3", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_sha3_guard.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `guard-sha3.stdout` / `.stderr`

### guard-float — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "float", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_float_guard.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `guard-float.stdout` / `.stderr`

### guard-map — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "map", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_release_map_guard.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `guard-map.stdout` / `.stderr`

### mutants-configure — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "-S", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutants", "-B", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_CXX_COMPILER=cl", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DPR_SOURCE=D:\\a\\t81-foundation\\t81-foundation\\source", "-DCORE_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_core.lib"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `mutants-configure.stdout` / `.stderr`

### mutants-build — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build", "--verbose", "--parallel", "2"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `mutants-build.stdout` / `.stderr`

### mutant-rejection-sha3 — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "sha3", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_sha3.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `mutant-rejection-sha3.stdout` / `.stderr`

### mutant-rejection-float — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "float", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_float.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `mutant-rejection-float.stdout` / `.stderr`

### mutant-rejection-map — return code 1

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["C:\\hostedtoolcache\\windows\\Python\\3.12.10\\x64\\python.exe", "-B", "D:\\a\\t81-foundation\\t81-foundation\\source\\tests\\python\\check_release_correctness_guard.py", "map", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\mutant-build\\mutant_map.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `mutant-rejection-map.stdout` / `.stderr`

### build-vm — return code 2

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build", "--verbose", "--parallel", "2", "--target", "t81_vm_determinism_property_test", "t81_vm_deterministic_fault_test", "t81_vm_fault_family_determinism_matrix_test", "t81_vm_tloadhash_decodefault_determinism_matrix_test", "t81_axion_log_determinism_test", "t81_vm_system_registers_deterministic_test", "t81_isa_binary_io_determinism_test", "t81_vm_workload_determinism_tiers_test", "test_resource_monitoring", "t81_determinism_evidence_test"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `build-vm.stdout` / `.stderr`

### build-independent-evidence — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build", "--verbose", "--target", "t81_determinism_evidence_test"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `build-independent-evidence.stdout` / `.stderr`

### test-t81_determinism_evidence_test — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_determinism_evidence_test.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `test-t81_determinism_evidence_test.stdout` / `.stderr`

### diagnostics-configure — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "-S", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostics", "-B", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_CXX_COMPILER=cl", "-DPR_SOURCE=D:\\a\\t81-foundation\\t81-foundation\\source", "-DAXION_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_axion.lib", "-DCORE_LIBRARY=D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_core.lib"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `diagnostics-configure.stdout` / `.stderr`

### diagnostics-build — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build", "--verbose"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `diagnostics-build.stdout` / `.stderr`

### diagnostic-evidence — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build\\evidence_probe.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `diagnostic-evidence.stdout` / `.stderr`

### diagnostic-path — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\diagnostic-build\\path_probe.exe"]
```

Raw output: `final/artifacts/base01-windows-2022-Debug.zip` → `diagnostic-path.stdout` / `.stderr`

## independent-isa / base01-independent-windows-Debug-isa

### compiler — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cl"]
```

Raw output: `independent-isa/artifacts/base01-independent-windows-Debug-isa.zip` → `compiler.stdout` / `.stderr`

### cmake-version — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--version"]
```

Raw output: `independent-isa/artifacts/base01-independent-windows-Debug-isa.zip` → `cmake-version.stdout` / `.stderr`

### ninja-version — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["ninja", "--version"]
```

Raw output: `independent-isa/artifacts/base01-independent-windows-Debug-isa.zip` → `ninja-version.stdout` / `.stderr`

### configure — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "-S", "D:\\a\\t81-foundation\\t81-foundation\\source", "-B", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build", "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_C_COMPILER=cl", "-DCMAKE_CXX_COMPILER=cl", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DT81_BUILD_BENCHMARKS=OFF", "-DT81_BUILD_TUI=OFF", "-DT81_BUILD_EXAMPLES=OFF", "-DT81_BUILD_FUZZ_TESTS=OFF", "-DT81_ENABLE_TERNARYOS=OFF", "-DT81_ENABLE_DPE=OFF", "-DT81_ENABLE_LLAMA_CPP=OFF"]
```

Raw output: `independent-isa/artifacts/base01-independent-windows-Debug-isa.zip` → `configure.stdout` / `.stderr`

### build-isa — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["cmake", "--build", "D:\\a\\t81-foundation\\t81-foundation\\evidence\\build", "--verbose", "--parallel", "2", "--target", "t81_isa_binary_io_determinism_test"]
```

Raw output: `independent-isa/artifacts/base01-independent-windows-Debug-isa.zip` → `build-isa.stdout` / `.stderr`

### test-isa — return code 0

Working directory: `D:\a\t81-foundation\t81-foundation\source`

```text
["D:\\a\\t81-foundation\\t81-foundation\\evidence\\build\\t81_isa_binary_io_determinism_test.exe"]
```

Raw output: `independent-isa/artifacts/base01-independent-windows-Debug-isa.zip` → `test-isa.stdout` / `.stderr`
