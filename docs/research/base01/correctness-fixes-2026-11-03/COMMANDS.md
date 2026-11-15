# Publication rerun: exact commands

Outputs and command JSON are retained under rerun/logs/ in evidence.tar.gz. Original commands remain unchanged at original/COMMANDS.md in that archive.

## publication-canonfs_identity_contract_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/canonfs_identity_contract_test`

```sh
/private/tmp/t81-base01-execution/latest-build/canonfs_identity_contract_test
```

Archive output: `rerun/logs/publication-canonfs_identity_contract_test.log`

## publication-canonfs_integrity_matrix_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/canonfs_integrity_matrix_test`

```sh
/private/tmp/t81-base01-execution/latest-build/canonfs_integrity_matrix_test
```

Archive output: `rerun/logs/publication-canonfs_integrity_matrix_test.log`

## publication-canonfs_read_verify_env_contract_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/canonfs_read_verify_env_contract_test`

```sh
/private/tmp/t81-base01-execution/latest-build/canonfs_read_verify_env_contract_test
```

Archive output: `rerun/logs/publication-canonfs_read_verify_env_contract_test.log`

## publication-canonfs_read_verify_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/canonfs_read_verify_test`

```sh
/private/tmp/t81-base01-execution/latest-build/canonfs_read_verify_test
```

Archive output: `rerun/logs/publication-canonfs_read_verify_test.log`

## publication-committed-guard-float (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /private/tmp/t81-base01-correctness-latest/tests/python/check_release_correctness_guard.py float /private/tmp/t81-base01-execution/latest-build/t81_release_float_guard
```

Archive output: `rerun/logs/publication-committed-guard-float.log`

## publication-committed-guard-map (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /private/tmp/t81-base01-correctness-latest/tests/python/check_release_correctness_guard.py map /private/tmp/t81-base01-execution/latest-build/t81_release_map_guard
```

Archive output: `rerun/logs/publication-committed-guard-map.log`

## publication-committed-guard-sha3 (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /private/tmp/t81-base01-correctness-latest/tests/python/check_release_correctness_guard.py sha3 /private/tmp/t81-base01-execution/latest-build/t81_release_sha3_guard
```

Archive output: `rerun/logs/publication-committed-guard-sha3.log`

## publication-ctest-registration (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
ctest --test-dir /private/tmp/t81-base01-execution/latest-build --show-only=json-v1 -R '^t81_release_(sha3|float|map)_guard$'
```

Archive output: `rerun/logs/publication-ctest-registration.log`

## publication-foundation-build (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
cmake --build /private/tmp/t81-base01-execution/foundation-build --verbose --parallel 2
```

Archive output: `rerun/logs/publication-foundation-build.log`

## publication-foundation-configure (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
cmake -S /private/tmp/t81-base01-correctness-latest/experiments/base01_b1_foundation -B /private/tmp/t81-base01-execution/foundation-build -G Ninja -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DAUDITED_SOURCE=/private/tmp/t81-base01-execution/source -DAUDITED_BUILD=/private/tmp/t81-base01-execution/source-build
```

Archive output: `rerun/logs/publication-foundation-configure.log`

## publication-foundation-tests (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
BASE01_FOUNDATION_NATIVE=/private/tmp/t81-base01-execution/foundation-build/foundation_check BASE01_FOUNDATION_RAW=/private/tmp/t81-base01-execution/publication-foundation-evidence/native.json.gz python3 -B /private/tmp/t81-base01-execution/unit_no_timing.py /private/tmp/t81-base01-correctness-latest/experiments/base01_b1_foundation /private/tmp/t81-base01-execution/publication-foundation-tests.json
```

Archive output: `rerun/logs/publication-foundation-tests.log`

## publication-foundation-validation (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
python3 -B /private/tmp/t81-base01-correctness-latest/experiments/base01_b1_foundation/harness.py --validate-only --source /private/tmp/t81-base01-execution/source --source-build /private/tmp/t81-base01-execution/source-build --build /private/tmp/t81-base01-execution/foundation-build --output /private/tmp/t81-base01-execution/publication-foundation-evidence
```

Archive output: `rerun/logs/publication-foundation-validation.log`

## publication-guard-build (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
cmake --build /private/tmp/t81-base01-execution/latest-build --verbose --parallel 4 --target t81_release_sha3_guard t81_release_float_guard t81_release_map_guard t81_sha3_test test_deterministic_float_boundaries test_map_determinism t81_determinism_containers_test
```

Archive output: `rerun/logs/publication-guard-build.log`

## publication-guard-float (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /private/tmp/t81-base01-correctness-latest/tests/python/check_release_correctness_guard.py float /private/tmp/t81-base01-execution/latest-build/t81_release_float_guard
```

Archive output: `rerun/logs/publication-guard-float.log`

## publication-guard-map (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /private/tmp/t81-base01-correctness-latest/tests/python/check_release_correctness_guard.py map /private/tmp/t81-base01-execution/latest-build/t81_release_map_guard
```

Archive output: `rerun/logs/publication-guard-map.log`

## publication-guard-sha3 (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /private/tmp/t81-base01-correctness-latest/tests/python/check_release_correctness_guard.py sha3 /private/tmp/t81-base01-execution/latest-build/t81_release_sha3_guard
```

Archive output: `rerun/logs/publication-guard-sha3.log`

## publication-identity (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
python3 -B -c 'import json,pathlib,platform,subprocess,sys; r=pathlib.Path("/private/tmp/t81-base01-execution"); get=lambda *a:subprocess.check_output(a,text=True).strip(); print(json.dumps({"source_commit":get("git","rev-parse","HEAD"),"source_tree":get("git","rev-parse","HEAD^{tree}"),"source_status":get("git","status","--porcelain"),"audited_commit":get("git","-C",str(r/"source"),"rev-parse","HEAD"),"audited_status":get("git","-C",str(r/"source"),"status","--porcelain"),"platform":platform.platform(),"python":sys.version,"compiler":get("/usr/bin/c++","--version"),"cmake":get("cmake","--version"),"ninja":get("ninja","--version")},indent=2))'
```

Archive output: `rerun/logs/publication-identity.log`

## publication-lane-a-build (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
cmake --build /private/tmp/t81-base01-execution/lane-a-build --verbose --parallel 2
```

Archive output: `rerun/logs/publication-lane-a-build.log`

## publication-lane-a-configure (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
cmake -S /private/tmp/t81-base01-correctness-latest/experiments/base01_lane_a -B /private/tmp/t81-base01-execution/lane-a-build -G Ninja -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBASE01_SOURCE=/private/tmp/t81-base01-execution/source -DBASE01_BUILD=/private/tmp/t81-base01-execution/source-build
```

Archive output: `rerun/logs/publication-lane-a-configure.log`

## publication-lane-a-tests (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
BASE01_ACCEPTANCE_NATIVE=/private/tmp/t81-base01-execution/lane-a-build/base01_lane_a python3 -B /private/tmp/t81-base01-execution/unit_no_timing.py /private/tmp/t81-base01-correctness-latest/experiments/base01_lane_a /private/tmp/t81-base01-execution/publication-lane-a-tests.json
```

Archive output: `rerun/logs/publication-lane-a-tests.log`

## publication-lane-a-validation (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
python3 -B /private/tmp/t81-base01-correctness-latest/experiments/base01_lane_a/lane_a.py --validate-only --source /private/tmp/t81-base01-execution/source --source-build /private/tmp/t81-base01-execution/source-build --native-build /private/tmp/t81-base01-execution/lane-a-build --output /private/tmp/t81-base01-execution/publication-lane-a-evidence
```

Archive output: `rerun/logs/publication-lane-a-validation.log`

## publication-mutant-build-float (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/usr/bin/c++ -std=c++23 -O3 -DNDEBUG -DT81_DETERMINISTIC -DT81_RELEASE_GUARD_FLOAT -I /private/tmp/t81-base01-correctness-latest/include /private/tmp/t81-base01-execution/publication-guard-mutants/release_correctness_guard.cpp /private/tmp/t81-base01-execution/latest-build/libt81_core.a -o /private/tmp/t81-base01-execution/publication-guard-mutants/float
```

Archive output: `rerun/logs/publication-mutant-build-float.log`

## publication-mutant-build-map (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/usr/bin/c++ -std=c++23 -O3 -DNDEBUG -DT81_DETERMINISTIC -DT81_RELEASE_GUARD_MAP -I /private/tmp/t81-base01-correctness-latest/include /private/tmp/t81-base01-execution/publication-guard-mutants/release_correctness_guard.cpp /private/tmp/t81-base01-execution/latest-build/libt81_core.a -o /private/tmp/t81-base01-execution/publication-guard-mutants/map
```

Archive output: `rerun/logs/publication-mutant-build-map.log`

## publication-mutant-build-sha3 (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/usr/bin/c++ -std=c++23 -O3 -DNDEBUG -DT81_DETERMINISTIC -DT81_RELEASE_GUARD_SHA3 -I /private/tmp/t81-base01-correctness-latest/include /private/tmp/t81-base01-execution/publication-guard-mutants/release_correctness_guard.cpp /private/tmp/t81-base01-execution/latest-build/libt81_core.a -o /private/tmp/t81-base01-execution/publication-guard-mutants/sha3
```

Archive output: `rerun/logs/publication-mutant-build-sha3.log`

## publication-mutant-rejection-float (exit 1)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /private/tmp/t81-base01-correctness-latest/tests/python/check_release_correctness_guard.py float /private/tmp/t81-base01-execution/publication-guard-mutants/float
```

Archive output: `rerun/logs/publication-mutant-rejection-float.log`

## publication-mutant-rejection-map (exit 1)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /private/tmp/t81-base01-correctness-latest/tests/python/check_release_correctness_guard.py map /private/tmp/t81-base01-execution/publication-guard-mutants/map
```

Archive output: `rerun/logs/publication-mutant-rejection-map.log`

## publication-mutant-rejection-sha3 (exit 1)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /private/tmp/t81-base01-correctness-latest/tests/python/check_release_correctness_guard.py sha3 /private/tmp/t81-base01-execution/publication-guard-mutants/sha3
```

Archive output: `rerun/logs/publication-mutant-rejection-sha3.log`

## publication-pinned-t81_canonfs_driver_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/private/tmp/t81-base01-execution/source-build/t81_canonfs_driver_test
```

Archive output: `rerun/logs/publication-pinned-t81_canonfs_driver_test.log`

## publication-pinned-t81_ternary_arith_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/private/tmp/t81-base01-execution/source-build/t81_ternary_arith_test
```

Archive output: `rerun/logs/publication-pinned-t81_ternary_arith_test.log`

## publication-pinned-t81_vm_workload_determinism_tiers_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/private/tmp/t81-base01-execution/source-build/t81_vm_workload_determinism_tiers_test
```

Archive output: `rerun/logs/publication-pinned-t81_vm_workload_determinism_tiers_test.log`

## publication-pinned-test_resource_monitoring (exit 0)

Working directory: `/private/tmp/t81-base01-execution`

```sh
/private/tmp/t81-base01-execution/source-build/test_resource_monitoring
```

Archive output: `rerun/logs/publication-pinned-test_resource_monitoring.log`

## publication-r75-build (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
cmake --build /private/tmp/t81-base01-execution/r75-build --verbose --parallel 2
```

Archive output: `rerun/logs/publication-r75-build.log`

## publication-r75-configure (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
cmake -S /private/tmp/t81-base01-correctness-latest/experiments/base01_r75_semantics -B /private/tmp/t81-base01-execution/r75-build -G Ninja -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DAUDITED_SOURCE=/private/tmp/t81-base01-execution/source -DAUDITED_BUILD=/private/tmp/t81-base01-execution/source-build
```

Archive output: `rerun/logs/publication-r75-configure.log`

## publication-r75-validation (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
python3 -B /private/tmp/t81-base01-correctness-latest/experiments/base01_r75_semantics/verify.py --validate-only --source /private/tmp/t81-base01-execution/source --source-build /private/tmp/t81-base01-execution/source-build --build /private/tmp/t81-base01-execution/r75-build --output /private/tmp/t81-base01-execution/publication-r75-evidence
```

Archive output: `rerun/logs/publication-r75-validation.log`

## publication-resource-validation (exit 0)

Working directory: `/private/tmp/t81-base01-correctness-latest`

```sh
python3 -B experiments/base01_resource_accounting/validate.py --validate-only --cc /usr/bin/cc --build-dir /private/tmp/t81-base01-execution/publication-resource-build --output /private/tmp/t81-base01-execution/publication-resource.json
```

Archive output: `rerun/logs/publication-resource-validation.log`

## publication-t81_axion_log_determinism_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_axion_log_determinism_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_axion_log_determinism_test
```

Archive output: `rerun/logs/publication-t81_axion_log_determinism_test.log`

## publication-t81_canonfs_driver_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_canonfs_driver_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_canonfs_driver_test
```

Archive output: `rerun/logs/publication-t81_canonfs_driver_test.log`

## publication-t81_determinism_codec_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_determinism_codec_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_determinism_codec_test
```

Archive output: `rerun/logs/publication-t81_determinism_codec_test.log`

## publication-t81_determinism_containers_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_determinism_containers_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_determinism_containers_test
```

Archive output: `rerun/logs/publication-t81_determinism_containers_test.log`

## publication-t81_determinism_evidence_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_determinism_evidence_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_determinism_evidence_test
```

Archive output: `rerun/logs/publication-t81_determinism_evidence_test.log`

## publication-t81_determinism_float_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_determinism_float_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_determinism_float_test
```

Archive output: `rerun/logs/publication-t81_determinism_float_test.log`

## publication-t81_determinism_math_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_determinism_math_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_determinism_math_test
```

Archive output: `rerun/logs/publication-t81_determinism_math_test.log`

## publication-t81_determinism_new_types_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_determinism_new_types_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_determinism_new_types_test
```

Archive output: `rerun/logs/publication-t81_determinism_new_types_test.log`

## publication-t81_determinism_primitives_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_determinism_primitives_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_determinism_primitives_test
```

Archive output: `rerun/logs/publication-t81_determinism_primitives_test.log`

## publication-t81_determinism_vector_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_determinism_vector_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_determinism_vector_test
```

Archive output: `rerun/logs/publication-t81_determinism_vector_test.log`

## publication-t81_isa_binary_io_determinism_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_isa_binary_io_determinism_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_isa_binary_io_determinism_test
```

Archive output: `rerun/logs/publication-t81_isa_binary_io_determinism_test.log`

## publication-t81_sha3_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_sha3_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_sha3_test
```

Archive output: `rerun/logs/publication-t81_sha3_test.log`

## publication-t81_ternary_arith_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_ternary_arith_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_ternary_arith_test
```

Archive output: `rerun/logs/publication-t81_ternary_arith_test.log`

## publication-t81_vm_determinism_property_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_vm_determinism_property_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_vm_determinism_property_test
```

Archive output: `rerun/logs/publication-t81_vm_determinism_property_test.log`

## publication-t81_vm_deterministic_fault_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_vm_deterministic_fault_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_vm_deterministic_fault_test
```

Archive output: `rerun/logs/publication-t81_vm_deterministic_fault_test.log`

## publication-t81_vm_fault_family_determinism_matrix_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_vm_fault_family_determinism_matrix_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_vm_fault_family_determinism_matrix_test
```

Archive output: `rerun/logs/publication-t81_vm_fault_family_determinism_matrix_test.log`

## publication-t81_vm_system_registers_deterministic_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_vm_system_registers_deterministic_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_vm_system_registers_deterministic_test
```

Archive output: `rerun/logs/publication-t81_vm_system_registers_deterministic_test.log`

## publication-t81_vm_tloadhash_decodefault_determinism_matrix_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_vm_tloadhash_decodefault_determinism_matrix_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_vm_tloadhash_decodefault_determinism_matrix_test
```

Archive output: `rerun/logs/publication-t81_vm_tloadhash_decodefault_determinism_matrix_test.log`

## publication-t81_vm_workload_determinism_tiers_test (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/t81_vm_workload_determinism_tiers_test`

```sh
/private/tmp/t81-base01-execution/latest-build/t81_vm_workload_determinism_tiers_test
```

Archive output: `rerun/logs/publication-t81_vm_workload_determinism_tiers_test.log`

## publication-test_deterministic_float_boundaries (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/test_deterministic_float_boundaries`

```sh
/private/tmp/t81-base01-execution/latest-build/test_deterministic_float_boundaries
```

Archive output: `rerun/logs/publication-test_deterministic_float_boundaries.log`

## publication-test_map_determinism (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/test_map_determinism`

```sh
/private/tmp/t81-base01-execution/latest-build/test_map_determinism
```

Archive output: `rerun/logs/publication-test_map_determinism.log`

## publication-test_resource_monitoring (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/test_resource_monitoring`

```sh
/private/tmp/t81-base01-execution/latest-build/test_resource_monitoring
```

Archive output: `rerun/logs/publication-test_resource_monitoring.log`

## publication-test_t81float_soft_math (exit 0)

Working directory: `/private/tmp/t81-base01-execution/publication-test-runs/test_t81float_soft_math`

```sh
/private/tmp/t81-base01-execution/latest-build/test_t81float_soft_math
```

Archive output: `rerun/logs/publication-test_t81float_soft_math.log`
