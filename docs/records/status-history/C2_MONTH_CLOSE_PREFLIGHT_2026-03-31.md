# C2 Month-Close Preflight Report (2026-03-31)

Generated (UTC): 2026-02-28 16:20:22Z
Generator: `scripts/governance/c2_month_close_preflight.py`
Overall: PASS

## Summary

| Check | Status |
| :--- | :--- |
| C2 runbook consolidated check | PASS |
| Full local CTest sweep | PASS |
| Determinism slice | PASS |
| Stdlib surface baseline | PASS |
| Stdlib promotion snapshot | PASS |

## Command Outputs

### C2 runbook consolidated check

- Status: PASS
- Command: `/opt/homebrew/opt/python@3.14/bin/python3.14 scripts/governance/c2_month_close_check.py --output docs/status/C2_MONTH_CLOSE_CHECK_2026-03-31.md`

```text
Wrote report: docs/status/C2_MONTH_CLOSE_CHECK_2026-03-31.md
Governance hygiene check: PASS
Promotion gate snapshot refresh: PASS
Markdown link-target sweep: PASS
```

### Full local CTest sweep

- Status: PASS
- Command: `ctest --test-dir build --output-on-failure`

```text
Test project /build
        Start   1: t81_test_axion_opcodes
  1/297 Test   #1: t81_test_axion_opcodes .................................   Passed    0.01 sec
        Start   2: t81_vm_neural_opcodes_test
  2/297 Test   #2: t81_vm_neural_opcodes_test .............................   Passed    0.01 sec
        Start   3: test_bitwise
  3/297 Test   #3: test_bitwise ...........................................   Passed    0.01 sec
        Start   4: t81_vm_resource_monitoring_test
  4/297 Test   #4: t81_vm_resource_monitoring_test ........................   Passed    0.01 sec
        Start   5: t81_tritwise_backend_equivalence_test
  5/297 Test   #5: t81_tritwise_backend_equivalence_test ..................   Passed    0.01 sec
        Start   6: t81lang_surface_gate_test
  6/297 Test   #6: t81lang_surface_gate_test ..............................   Passed    0.00 sec
        Start   7: t81lang_conformance_edge_semantics_test
  7/297 Test   #7: t81lang_conformance_edge_semantics_test ................   Passed    0.00 sec
        Start   8: t81_ethics_test
  8/297 Test   #8: t81_ethics_test ........................................   Passed    0.00 sec
        Start   9: t81_ethics_invariants_test
  9/297 Test   #9: t81_ethics_invariants_test .............................   Passed    0.00 sec
        Start  10: test_resource_monitoring
 10/297 Test  #10: test_resource_monitoring ...............................   Passed    0.01 sec
        Start  11: test_tier3_opcodes
 11/297 Test  #11: test_tier3_opcodes .....................................   Passed    0.01 sec
        Start  12: vm_bounds_trace_test
 12/297 Test  #12: vm_bounds_trace_test ...................................   Passed    0.01 sec
        Start  13: t81_parser_regression_audit_test
 13/297 Test  #13: t81_parser_regression_audit_test .......................   Passed    0.00 sec
        Start  14: t81_frontend_parser_appendix_coverage_test
 14/297 Test  #14: t81_frontend_parser_appendix_coverage_test .............   Passed    0.00 sec
        Start  15: t81_ir_snapshot_audit_test
 15/297 Test  #15: t81_ir_snapshot_audit_test .............................   Passed    0.00 sec
        Start  16: t81_semantic_analyzer_stage3_rules_test
 16/297 Test  #16: t81_semantic_analyzer_stage3_rules_test ................   Passed    0.00 sec
        Start  17: t81_float_deterministic_test
 17/297 Test  #17: t81_float_deterministic_test ...........................   Passed    0.46 sec
        Start  18: t81_bigint_float_conversion_test
 18/297 Test  #18: t81_bigint_float_conversion_test .......................   Passed    0.01 sec
        Start  19: debug_matrix_crash
 19/297 Test  #19: debug_matrix_crash .....................................   Passed    0.00 sec
        Start  20: t81_float_test
 20/297 Test  #20: t81_float_test .........................................   Passed    0.45 sec
        Start  21: t81_float_properties_test
 21/297 Test  #21: t81_float_properties_test ..............................   Passed    0.03 sec
        Start  22: t81_float_arithmetic_test
 22/297 Test  #22: t81_float_arithmetic_test ..............................   Passed    0.00 sec
        Start  23: t81_t81int_to_binary_test
 23/297 Test  #23: t81_t81int_to_binary_test ..............................   Passed    0.00 sec
        Start  24: t81_division_test
 24/297 Test  #24: t81_division_test ......................................   Passed    0.00 sec
        Start  25: t81_t81int_test
 25/297 Test  #25: t81_t81int_test ........................................   Passed    0.00 sec
        Start  26: t81_t81int_overflow_test
 26/297 Test  #26: t81_t81int_overflow_test ...............................   Passed    0.00 sec
        Start  27: t81_t81int_generic_div_test
 27/297 Test  #27: t81_t81int_generic_div_test ............................   Passed    0.01 sec
        Start  28: t81_int_sign_trit_test
 28/297 Test  #28: t81_int_sign_trit_test .................................   Passed    0.01 sec
        Start  29: t81_bigint_test
 29/297 Test  #29: t81_bigint_test ........................................   Passed    0.01 sec
        Start  30: t81_fraction_test
 30/297 Test  #30: t81_fraction_test ......................................   Passed    0.01 sec
        Start  31: t81_bigint_properties_test
 31/297 Test  #31: t81_bigint_properties_test .............................   Passed    0.08 sec
        Start  32: t81_prob_properties_test
 32/297 Test  #32: t81_prob_properties_test ...............................   Passed    0.01 sec
        Start  33: t81_t81int_properties_test
 33/297 Test  #33: t81_t81int_properties_test .............................   Passed    0.04 sec
        Start  34: t81_bigint_division_edge_properties_test
 34/297 Test  #34: t81_bigint_division_edge_properties_test ...............   Passed    0.07 sec
        Start  35: t81_bigint_division_semantics_test
 35/297 Test  #35: t81_bigint_division_semantics_test .....................   Passed    0.01 sec
        Start  36: t81_bigint_gcd_divmod_property_test
 36/297 Test  #36: t81_bigint_gcd_divmod_property_test ....................   Passed    0.06 sec
        Start  37: t81_bigint_modular_inverse_test
 37/297 Test  #37: t81_bigint_modular_inverse_test ........................   Passed    0.01 sec
        Start  38: fuzz_bigint_libfuzzer
 38/297 Test  #38: fuzz_bigint_libfuzzer ..................................   Passed    1.33 sec
        Start  39: t81_fraction_properties_test
 39/297 Test  #39: t81_fraction_properties_test ...........................   Passed    0.52 sec
        Start  40: t81_core_numeric_compat_test
 40/297 Test  #40: t81_core_numeric_compat_test ...........................   Passed    0.01 sec
        Start  41: t81_core_bigint_compat_properties_test
 41/297 Test  #41: t81_core_bigint_compat_properties_test .................   Passed    0.23 sec
        Start  42: t81_core_fraction_compat_properties_test
 42/297 Test  #42: t81_core_fraction_compat_properties_test ...............   Passed    0.11 sec
        Start  43: t81_v1_canonical_numeric_contract_test
 43/297 Test  #43: t81_v1_canonical_numeric_contract_test .................   Passed    0.01 sec
        Start  44: t81_complex_test
 44/297 Test  #44: t81_complex_test .......................................   Passed    0.00 sec
        Start  45: t81_prob_test
 45/297 Test  #45: t81_prob_test ..........................................   Passed    0.00 sec
        Start  46: t81_symbol_test
 46/297 Test  #46: t81_symbol_test ........................................   Passed    0.00 sec
        Start  47: t81_string_test
 47/297 Test  #47: t81_string_test ........................................   Passed    0.00 sec
        Start  48: t81_qutrit_test
 48/297 Test  #48: t81_qutrit_test ........................................   Passed    0.00 sec
        Start  49: t81_fixed_test
 49/297 Test  #49: t81_fixed_test .........................................   Passed    0.00 sec
        Start  50: t81_uint_test
 50/297 Test  #50: t81_uint_test ..........................................   Passed    0.00 sec
        Start  51: t81_list_test
 51/297 Test  #51: t81_list_test ..........................................   Passed    0.00 sec
        Start  52: t81_map_test
 52/297 Test  #52: t81_map_test ...........................................   Passed    0.00 sec
        Start  53: t81_set_test
 53/297 Test  #53: t81_set_test ...........................................   Passed    0.00 sec
        Start  54: t81_vector_test
 54/297 Test  #54: t81_vector_test ........................................   Passed    0.00 sec
        Start  55: t81_matrix_test
 55/297 Test  #55: t81_matrix_test ........................................   Passed    0.00 sec
        Start  56: t81_matrix_singular_test
 56/297 Test  #56: t81_matrix_singular_test ...............................   Passed    0.00 sec
        Start  57: t81_maybe_test
 57/297 Test  #57: t81_maybe_test .........................................   Passed    0.00 sec
        Start  58: t81_result_test
 58/297 Test  #58: t81_result_test ........................................   Passed    0.00 sec
        Start  59: t81_quaternion_test
 59/297 Test  #59: t81_quaternion_test ....................................   Passed    0.00 sec
        Start  60: t81_time_test
... [truncated 479 lines; see local command logs for full output]
```

### Determinism slice

- Status: PASS
- Command: `scripts/ci/run_determinism_slice.sh build`

```text
Test project /build
      Start   2: t81_vm_neural_opcodes_test
 1/81 Test   #2: t81_vm_neural_opcodes_test .............................   Passed    0.00 sec
      Start   4: t81_vm_resource_monitoring_test
 2/81 Test   #4: t81_vm_resource_monitoring_test ........................   Passed    0.00 sec
      Start  12: vm_bounds_trace_test
 3/81 Test  #12: vm_bounds_trace_test ...................................   Passed    0.00 sec
      Start  18: t81_bigint_float_conversion_test
 4/81 Test  #18: t81_bigint_float_conversion_test .......................   Passed    0.00 sec
      Start  29: t81_bigint_test
 5/81 Test  #29: t81_bigint_test ........................................   Passed    0.00 sec
      Start  31: t81_bigint_properties_test
 6/81 Test  #31: t81_bigint_properties_test .............................   Passed    0.08 sec
      Start  34: t81_bigint_division_edge_properties_test
 7/81 Test  #34: t81_bigint_division_edge_properties_test ...............   Passed    0.07 sec
      Start  35: t81_bigint_division_semantics_test
 8/81 Test  #35: t81_bigint_division_semantics_test .....................   Passed    0.00 sec
      Start  36: t81_bigint_gcd_divmod_property_test
 9/81 Test  #36: t81_bigint_gcd_divmod_property_test ....................   Passed    0.06 sec
      Start  37: t81_bigint_modular_inverse_test
10/81 Test  #37: t81_bigint_modular_inverse_test ........................   Passed    0.00 sec
      Start  38: fuzz_bigint_libfuzzer
11/81 Test  #38: fuzz_bigint_libfuzzer ..................................   Passed    1.30 sec
      Start  41: t81_core_bigint_compat_properties_test
12/81 Test  #41: t81_core_bigint_compat_properties_test .................   Passed    0.23 sec
      Start  43: t81_v1_canonical_numeric_contract_test
13/81 Test  #43: t81_v1_canonical_numeric_contract_test .................   Passed    0.00 sec
      Start 107: t81_c_api_bigint_test
14/81 Test #107: t81_c_api_bigint_test ..................................   Passed    0.00 sec
      Start 110: t81_vm_load_store_test
15/81 Test #110: t81_vm_load_store_test .................................   Passed    0.00 sec
      Start 111: t81_vm_divmod_test
16/81 Test #111: t81_vm_divmod_test .....................................   Passed    0.00 sec
      Start 112: t81_vm_illegal_test
17/81 Test #112: t81_vm_illegal_test ....................................   Passed    0.00 sec
      Start 113: t81_vm_bounds_test
18/81 Test #113: t81_vm_bounds_test .....................................   Passed    0.00 sec
      Start 114: t81_vm_tensor_test
19/81 Test #114: t81_vm_tensor_test .....................................   Passed    0.00 sec
      Start 115: t81_vm_tloadhash_conformance_test
20/81 Test #115: t81_vm_tloadhash_conformance_test ......................   Passed    0.01 sec
      Start 116: t81_vm_tloadhash_decodefault_determinism_matrix_test
21/81 Test #116: t81_vm_tloadhash_decodefault_determinism_matrix_test ...   Passed    0.01 sec
      Start 117: t81_vm_tensor_shape_faults_test
22/81 Test #117: t81_vm_tensor_shape_faults_test ........................   Passed    0.00 sec
      Start 118: t81_vm_tensor_get_set_conformance_test
23/81 Test #118: t81_vm_tensor_get_set_conformance_test .................   Passed    0.00 sec
      Start 119: t81_vm_tensor_helper_predicates_test
24/81 Test #119: t81_vm_tensor_helper_predicates_test ...................   Passed    0.00 sec
      Start 120: t81_vm_predispatch_policy_deny_logging_test
25/81 Test #120: t81_vm_predispatch_policy_deny_logging_test ............   Passed    0.00 sec
      Start 121: t81_vm_fault_test
26/81 Test #121: t81_vm_fault_test ......................................   Passed    0.00 sec
      Start 122: t81_vm_deterministic_fault_test
27/81 Test #122: t81_vm_deterministic_fault_test ........................   Passed    0.00 sec
      Start 123: t81_vm_fault_family_determinism_matrix_test
28/81 Test #123: t81_vm_fault_family_determinism_matrix_test ............   Passed    0.01 sec
      Start 124: t81_vm_memory_test
29/81 Test #124: t81_vm_memory_test .....................................   Passed    0.00 sec
      Start 125: t81_axion_log_determinism_test
30/81 Test #125: t81_axion_log_determinism_test .........................   Passed    0.00 sec
      Start 129: t81_vm_trace_test
31/81 Test #129: t81_vm_trace_test ......................................   Passed    0.00 sec
      Start 130: t81_vm_determinism_property_test
32/81 Test #130: t81_vm_determinism_property_test .......................   Passed    0.03 sec
      Start 131: t81_vm_state_transition_invariants_test
33/81 Test #131: t81_vm_state_transition_invariants_test ................   Passed    0.00 sec
      Start 132: t81_vm_state_transition_conformance_matrix_test
34/81 Test #132: t81_vm_state_transition_conformance_matrix_test ........   Passed    0.01 sec
      Start 133: t81_vm_workload_determinism_test
35/81 Test #133: t81_vm_workload_determinism_test .......................   Passed    0.01 sec
      Start 134: t81_vm_workload_determinism_tiers_test
36/81 Test #134: t81_vm_workload_determinism_tiers_test .................   Passed    0.01 sec
      Start 135: t81_vm_mixed_workload_conformance_matrix_test
37/81 Test #135: t81_vm_mixed_workload_conformance_matrix_test ..........   Passed    0.01 sec
      Start 136: t81_vm_policy_parse_fail_closed_test
38/81 Test #136: t81_vm_policy_parse_fail_closed_test ...................   Passed    0.00 sec
      Start 137: t81_vm_axreport_policy_deny_fail_closed_test
39/81 Test #137: t81_vm_axreport_policy_deny_fail_closed_test ...........   Passed    0.00 sec
      Start 138: t81_vm_system_registers_deterministic_test
40/81 Test #138: t81_vm_system_registers_deterministic_test .............   Passed    0.00 sec
      Start 139: t81_vm_stubbed_privileged_opcode_fail_closed_test
41/81 Test #139: t81_vm_stubbed_privileged_opcode_fail_closed_test ......   Passed    0.00 sec
      Start 140: t81_vm_stubbed_async_network_opcode_fail_closed_test
42/81 Test #140: t81_vm_stubbed_async_network_opcode_fail_closed_test ...   Passed    0.00 sec
      Start 141: vm_extended_ops_test
43/81 Test #141: vm_extended_ops_test ...................................   Passed    0.00 sec
      Start 143: t81_vm_jump_flags_test
44/81 Test #143: t81_vm_jump_flags_test .................................   Passed    0.00 sec
      Start 144: t81_vm_float_fraction_ops_test
45/81 Test #144: t81_vm_float_fraction_ops_test .........................   Passed    0.00 sec
      Start 145: t81_vm_literal_pool_extension_test
46/81 Test #145: t81_vm_literal_pool_extension_test .....................   Passed    0.00 sec
      Start 146: t81_vm_neg_jumps_test
47/81 Test #146: t81_vm_neg_jumps_test ..................................   Passed    0.00 sec
      Start 147: t81_vm_print_test
48/81 Test #147: t81_vm_print_test ......................................   Passed    0.00 sec
      Start 154: t81_bigint_v1_baseline_test
49/81 Test #154: t81_bigint_v1_baseline_test ............................   Passed    0.08 sec
      Start 182: t81_isa_binary_io_determinism_test
50/81 Test #182: t81_isa_binary_io_determinism_test .....................   Passed    0.00 sec
      Start 191: e2e_compile_determinism_test
51/81 Test #191: e2e_compile_determinism_test ...........................   Passed    0.01 sec
      Start 192: e2e_match_metadata_determinism_test
52/81 Test #192: e2e_match_metadata_determinism_test ....................   Passed    0.01 sec
      Start 193: e2e_ast_ir_canonical_determinism_test
53/81 Test #193: e2e_ast_ir_canonical_determinism_test ..................   Passed    0.01 sec
      Start 194: e2e_enum_metadata_determinism_test
54/81 Test #194: e2e_enum_metadata_determinism_test .....................   Passed    0.00 sec
      Start 202: t81_cli_trace_export_test
55/81 Test #202: t81_cli_trace_export_test ..............................   Passed    0.01 sec
      Start 207: axion_policy_allow_deny_determinism_test
56/81 Test #207: axion_policy_allow_deny_determinism_test ...............   Passed    0.01 sec
      Start 214: canonfs_axion_trace_test
57/81 Test #214: canonfs_axion_trace_test ...............................   Passed    0.00 sec
      Start 218: t81_tier4_vm_test
58/81 Test #218: t81_tier4_vm_test ......................................   Passed    0.00 sec
      Start 220: t81_jit_test
59/81 Test #220: t81_jit_test ...........................................   Passed    0.00 sec
      Start 221: jit_trace_equivalence_test
... [truncated 47 lines; see local command logs for full output]
```

### Stdlib surface baseline

- Status: PASS
- Command: `/opt/homebrew/opt/python@3.14/bin/python3.14 scripts/governance/check_stdlib_surface_baseline.py`

```text
stdlib surface baseline check PASSED
- modules validated: 13
- fixture directories validated: 10
- fixture tests validated: 10
```

### Stdlib promotion snapshot

- Status: PASS
- Command: `/opt/homebrew/opt/python@3.14/bin/python3.14 scripts/governance/check_stdlib_promotion_snapshot.py`

```text
stdlib promotion snapshot check PASSED
- modules validated: 11
```
