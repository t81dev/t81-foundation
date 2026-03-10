# C2 Month-Close Preflight Report (2026-03-31)

Generated (UTC): 2026-03-10 16:32:15Z
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
Test project /Users/t81dev/Code/t81-foundation/build
        Start   1: tui_snapshot_test
  1/336 Test   #1: tui_snapshot_test ..............................................   Passed    0.01 sec
        Start   2: t81_test_axion_opcodes
  2/336 Test   #2: t81_test_axion_opcodes .........................................   Passed    0.00 sec
        Start   3: t81_vm_neural_opcodes_test
  3/336 Test   #3: t81_vm_neural_opcodes_test .....................................   Passed    0.00 sec
        Start   4: test_bitwise
  4/336 Test   #4: test_bitwise ...................................................   Passed    0.00 sec
        Start   5: t81_vm_resource_monitoring_test
  5/336 Test   #5: t81_vm_resource_monitoring_test ................................   Passed    0.00 sec
        Start   6: t81_tritwise_backend_equivalence_test
  6/336 Test   #6: t81_tritwise_backend_equivalence_test ..........................   Passed    0.00 sec
        Start   7: t81lang_surface_gate_test
  7/336 Test   #7: t81lang_surface_gate_test ......................................   Passed    0.00 sec
        Start   8: t81lang_conformance_edge_semantics_test
  8/336 Test   #8: t81lang_conformance_edge_semantics_test ........................   Passed    0.00 sec
        Start   9: t81_ethics_test
  9/336 Test   #9: t81_ethics_test ................................................   Passed    0.00 sec
        Start  10: t81_ethics_invariants_test
 10/336 Test  #10: t81_ethics_invariants_test .....................................   Passed    0.00 sec
        Start  11: test_resource_monitoring
 11/336 Test  #11: test_resource_monitoring .......................................   Passed    0.00 sec
        Start  12: test_tier3_opcodes
 12/336 Test  #12: test_tier3_opcodes .............................................   Passed    0.00 sec
        Start  13: vm_bounds_trace_test
 13/336 Test  #13: vm_bounds_trace_test ...........................................   Passed    0.00 sec
        Start  14: t81_parser_regression_audit_test
 14/336 Test  #14: t81_parser_regression_audit_test ...............................   Passed    0.00 sec
        Start  15: t81_frontend_parser_appendix_coverage_test
 15/336 Test  #15: t81_frontend_parser_appendix_coverage_test .....................   Passed    0.00 sec
        Start  16: t81_ir_snapshot_audit_test
 16/336 Test  #16: t81_ir_snapshot_audit_test .....................................   Passed    0.00 sec
        Start  17: t81_semantic_analyzer_stage3_rules_test
 17/336 Test  #17: t81_semantic_analyzer_stage3_rules_test ........................   Passed    0.00 sec
        Start  18: vm_reflection_tier2_test
 18/336 Test  #18: vm_reflection_tier2_test .......................................   Passed    0.01 sec
        Start  19: reflective_evidence_replay_test
 19/336 Test  #19: reflective_evidence_replay_test ................................   Passed    0.00 sec
        Start  20: t81_float_deterministic_test
 20/336 Test  #20: t81_float_deterministic_test ...................................   Passed    0.00 sec
        Start  21: t81_bigint_float_conversion_test
 21/336 Test  #21: t81_bigint_float_conversion_test ...............................   Passed    0.00 sec
        Start  22: debug_matrix_crash
 22/336 Test  #22: debug_matrix_crash .............................................   Passed    0.03 sec
        Start  23: t81_float_test
 23/336 Test  #23: t81_float_test .................................................   Passed    0.00 sec
        Start  24: t81_float_properties_test
 24/336 Test  #24: t81_float_properties_test ......................................   Passed    0.02 sec
        Start  25: t81_t81int_to_binary_test
 25/336 Test  #25: t81_t81int_to_binary_test ......................................   Passed    0.00 sec
        Start  26: t81_division_test
 26/336 Test  #26: t81_division_test ..............................................   Passed    0.00 sec
        Start  27: t81_t81int_test
 27/336 Test  #27: t81_t81int_test ................................................   Passed    0.00 sec
        Start  28: t81_t81int_overflow_test
 28/336 Test  #28: t81_t81int_overflow_test .......................................   Passed    0.00 sec
        Start  29: t81_t81int_generic_div_test
 29/336 Test  #29: t81_t81int_generic_div_test ....................................   Passed    0.00 sec
        Start  30: t81_int_sign_trit_test
 30/336 Test  #30: t81_int_sign_trit_test .........................................   Passed    0.00 sec
        Start  31: t81_bigint_test
 31/336 Test  #31: t81_bigint_test ................................................   Passed    0.01 sec
        Start  32: t81_fraction_test
 32/336 Test  #32: t81_fraction_test ..............................................   Passed    0.00 sec
        Start  33: t81_bigint_properties_test
 33/336 Test  #33: t81_bigint_properties_test .....................................   Passed    0.05 sec
        Start  34: t81_prob_properties_test
 34/336 Test  #34: t81_prob_properties_test .......................................   Passed    0.01 sec
        Start  35: t81_t81int_properties_test
 35/336 Test  #35: t81_t81int_properties_test .....................................   Passed    0.02 sec
        Start  36: t81_bigint_division_edge_properties_test
 36/336 Test  #36: t81_bigint_division_edge_properties_test .......................   Passed    0.04 sec
        Start  37: t81_bigint_division_semantics_test
 37/336 Test  #37: t81_bigint_division_semantics_test .............................   Passed    0.00 sec
        Start  38: t81_bigint_gcd_divmod_property_test
 38/336 Test  #38: t81_bigint_gcd_divmod_property_test ............................   Passed    0.03 sec
        Start  39: t81_bigint_modular_inverse_test
 39/336 Test  #39: t81_bigint_modular_inverse_test ................................   Passed    0.00 sec
        Start  40: fuzz_bigint_libfuzzer
 40/336 Test  #40: fuzz_bigint_libfuzzer ..........................................   Passed    0.78 sec
        Start  41: t81_fraction_properties_test
 41/336 Test  #41: t81_fraction_properties_test ...................................   Passed    0.31 sec
        Start  42: t81_core_numeric_compat_test
 42/336 Test  #42: t81_core_numeric_compat_test ...................................   Passed    0.01 sec
        Start  43: t81_core_bigint_compat_properties_test
 43/336 Test  #43: t81_core_bigint_compat_properties_test .........................   Passed    0.14 sec
        Start  44: t81_core_fraction_compat_properties_test
 44/336 Test  #44: t81_core_fraction_compat_properties_test .......................   Passed    0.07 sec
        Start  45: t81_v1_canonical_numeric_contract_test
 45/336 Test  #45: t81_v1_canonical_numeric_contract_test .........................   Passed    0.00 sec
        Start  46: t81_complex_test
 46/336 Test  #46: t81_complex_test ...............................................   Passed    0.00 sec
        Start  47: t81_prob_test
 47/336 Test  #47: t81_prob_test ..................................................   Passed    0.00 sec
        Start  48: t81_symbol_test
 48/336 Test  #48: t81_symbol_test ................................................   Passed    0.00 sec
        Start  49: t81_string_test
 49/336 Test  #49: t81_string_test ................................................   Passed    0.00 sec
        Start  50: t81_qutrit_test
 50/336 Test  #50: t81_qutrit_test ................................................   Passed    0.00 sec
        Start  51: t81_fixed_test
 51/336 Test  #51: t81_fixed_test .................................................   Passed    0.00 sec
        Start  52: t81_uint_test
 52/336 Test  #52: t81_uint_test ..................................................   Passed    0.00 sec
        Start  53: t81_list_test
 53/336 Test  #53: t81_list_test ..................................................   Passed    0.00 sec
        Start  54: t81_map_test
 54/336 Test  #54: t81_map_test ...................................................   Passed    0.00 sec
        Start  55: t81_set_test
 55/336 Test  #55: t81_set_test ...................................................   Passed    0.00 sec
        Start  56: t81_vector_test
 56/336 Test  #56: t81_vector_test ................................................   Passed    0.00 sec
        Start  57: t81_matrix_test
 57/336 Test  #57: t81_matrix_test ................................................   Passed    0.04 sec
        Start  58: t81_matrix_singular_test
 58/336 Test  #58: t81_matrix_singular_test .......................................   Passed    0.01 sec
        Start  59: t81_maybe_test
 59/336 Test  #59: t81_maybe_test .................................................   Passed    0.00 sec
        Start  60: t81_result_test
... [truncated 560 lines; see local command logs for full output]
```

### Determinism slice

- Status: PASS
- Command: `scripts/ci/run_determinism_slice.sh build`

```text
Test project /Users/t81dev/Code/t81-foundation/build
        Start   3: t81_vm_neural_opcodes_test
  1/107 Test   #3: t81_vm_neural_opcodes_test .............................   Passed    0.01 sec
        Start   5: t81_vm_resource_monitoring_test
  2/107 Test   #5: t81_vm_resource_monitoring_test ........................   Passed    0.01 sec
        Start  13: vm_bounds_trace_test
  3/107 Test  #13: vm_bounds_trace_test ...................................   Passed    0.01 sec
        Start  18: vm_reflection_tier2_test
  4/107 Test  #18: vm_reflection_tier2_test ...............................   Passed    0.01 sec
        Start  21: t81_bigint_float_conversion_test
  5/107 Test  #21: t81_bigint_float_conversion_test .......................   Passed    0.00 sec
        Start  31: t81_bigint_test
  6/107 Test  #31: t81_bigint_test ........................................   Passed    0.00 sec
        Start  33: t81_bigint_properties_test
  7/107 Test  #33: t81_bigint_properties_test .............................   Passed    0.07 sec
        Start  36: t81_bigint_division_edge_properties_test
  8/107 Test  #36: t81_bigint_division_edge_properties_test ...............   Passed    0.05 sec
        Start  37: t81_bigint_division_semantics_test
  9/107 Test  #37: t81_bigint_division_semantics_test .....................   Passed    0.00 sec
        Start  38: t81_bigint_gcd_divmod_property_test
 10/107 Test  #38: t81_bigint_gcd_divmod_property_test ....................   Passed    0.04 sec
        Start  39: t81_bigint_modular_inverse_test
 11/107 Test  #39: t81_bigint_modular_inverse_test ........................   Passed    0.09 sec
        Start  40: fuzz_bigint_libfuzzer
 12/107 Test  #40: fuzz_bigint_libfuzzer ..................................   Passed    0.95 sec
        Start  43: t81_core_bigint_compat_properties_test
 13/107 Test  #43: t81_core_bigint_compat_properties_test .................   Passed    0.17 sec
        Start  45: t81_v1_canonical_numeric_contract_test
 14/107 Test  #45: t81_v1_canonical_numeric_contract_test .................   Passed    0.00 sec
        Start  75: t81_vm_canonfs_root_env_contract_test
 15/107 Test  #75: t81_vm_canonfs_root_env_contract_test ..................   Passed    0.01 sec
        Start  97: t81_tensor_serialization_canonical_fixed_test
 16/107 Test  #97: t81_tensor_serialization_canonical_fixed_test ..........   Passed    0.38 sec
        Start 112: t81_c_api_bigint_test
 17/107 Test #112: t81_c_api_bigint_test ..................................   Passed    0.00 sec
        Start 115: t81_vm_load_store_test
 18/107 Test #115: t81_vm_load_store_test .................................   Passed    0.01 sec
        Start 116: t81_vm_illegal_test
 19/107 Test #116: t81_vm_illegal_test ....................................   Passed    0.01 sec
        Start 117: t81_vm_bounds_test
 20/107 Test #117: t81_vm_bounds_test .....................................   Passed    0.01 sec
        Start 118: t81_vm_tensor_test
 21/107 Test #118: t81_vm_tensor_test .....................................   Passed    2.92 sec
        Start 119: t81_vm_tloadhash_conformance_test
 22/107 Test #119: t81_vm_tloadhash_conformance_test ......................   Passed    0.13 sec
        Start 120: t81_vm_tloadhash_canonical_fixed_test
 23/107 Test #120: t81_vm_tloadhash_canonical_fixed_test ..................   Passed    0.27 sec
        Start 121: t81_vm_tloadhash_decodefault_determinism_matrix_test
 24/107 Test #121: t81_vm_tloadhash_decodefault_determinism_matrix_test ...   Passed    0.01 sec
        Start 122: t81_vm_ai_phase1_attention_conformance_test
 25/107 Test #122: t81_vm_ai_phase1_attention_conformance_test ............   Passed    2.31 sec
        Start 123: t81_vm_ai_phase1_embed_conformance_test
 26/107 Test #123: t81_vm_ai_phase1_embed_conformance_test ................   Passed    0.38 sec
        Start 124: t81_vm_ai_phase1_qmatmul_conformance_test
 27/107 Test #124: t81_vm_ai_phase1_qmatmul_conformance_test ..............   Passed    1.48 sec
        Start 125: t81_vm_ai_phase1_shared_helper_parity_test
 28/107 Test #125: t81_vm_ai_phase1_shared_helper_parity_test .............   Passed    2.52 sec
        Start 126: t81_vm_ai_phase1_wload_conformance_test
 29/107 Test #126: t81_vm_ai_phase1_wload_conformance_test ................   Passed    0.37 sec
        Start 127: t81_vm_ai_phase1_gather_conformance_test
 30/107 Test #127: t81_vm_ai_phase1_gather_conformance_test ...............   Passed    0.37 sec
        Start 128: t81_vm_ai_phase1_scatter_conformance_test
 31/107 Test #128: t81_vm_ai_phase1_scatter_conformance_test ..............   Passed    0.23 sec
        Start 129: t81_vm_ai_phase1_wload_canonfs_audit_test
 32/107 Test #129: t81_vm_ai_phase1_wload_canonfs_audit_test ..............   Passed    0.73 sec
        Start 130: t81_vm_ai_phase1_scatter_aliasing_test
 33/107 Test #130: t81_vm_ai_phase1_scatter_aliasing_test .................   Passed    0.44 sec
        Start 131: t81_vm_ai_phase1_gather_axis1_test
 34/107 Test #131: t81_vm_ai_phase1_gather_axis1_test .....................   Passed    0.75 sec
        Start 133: t81_vm_tensor_shape_faults_test
 35/107 Test #133: t81_vm_tensor_shape_faults_test ........................   Passed    1.57 sec
        Start 134: t81_vm_tensor_get_set_conformance_test
 36/107 Test #134: t81_vm_tensor_get_set_conformance_test .................   Passed    0.64 sec
        Start 135: t81_vm_tensor_helper_predicates_test
 37/107 Test #135: t81_vm_tensor_helper_predicates_test ...................   Passed    2.48 sec
        Start 136: t81_vm_tensor_provenance_trace_test
 38/107 Test #136: t81_vm_tensor_provenance_trace_test ....................   Passed    0.11 sec
        Start 137: t81_vm_predispatch_policy_deny_logging_test
 39/107 Test #137: t81_vm_predispatch_policy_deny_logging_test ............   Passed    0.01 sec
        Start 138: t81_vm_fault_test
 40/107 Test #138: t81_vm_fault_test ......................................   Passed    0.00 sec
        Start 139: t81_vm_deterministic_fault_test
 41/107 Test #139: t81_vm_deterministic_fault_test ........................   Passed    0.01 sec
        Start 140: t81_vm_fault_family_determinism_matrix_test
 42/107 Test #140: t81_vm_fault_family_determinism_matrix_test ............   Passed    0.23 sec
        Start 141: t81_vm_memory_test
 43/107 Test #141: t81_vm_memory_test .....................................   Passed    0.01 sec
        Start 142: t81_axion_log_determinism_test
 44/107 Test #142: t81_axion_log_determinism_test .........................   Passed    0.01 sec
        Start 147: t81_vm_trace_test
 45/107 Test #147: t81_vm_trace_test ......................................   Passed    0.01 sec
        Start 148: t81_vm_determinism_property_test
 46/107 Test #148: t81_vm_determinism_property_test .......................   Passed    0.02 sec
        Start 149: t81_vm_state_transition_invariants_test
 47/107 Test #149: t81_vm_state_transition_invariants_test ................   Passed    0.01 sec
        Start 150: t81_vm_state_transition_conformance_matrix_test
 48/107 Test #150: t81_vm_state_transition_conformance_matrix_test ........   Passed    0.01 sec
        Start 151: t81_vm_workload_determinism_tiers_test
 49/107 Test #151: t81_vm_workload_determinism_tiers_test .................   Passed    0.31 sec
        Start 152: t81_vm_mixed_workload_conformance_matrix_test
 50/107 Test #152: t81_vm_mixed_workload_conformance_matrix_test ..........   Passed    0.55 sec
        Start 153: t81_vm_policy_parse_fail_closed_test
 51/107 Test #153: t81_vm_policy_parse_fail_closed_test ...................   Passed    0.01 sec
        Start 154: t81_vm_axreport_policy_deny_fail_closed_test
 52/107 Test #154: t81_vm_axreport_policy_deny_fail_closed_test ...........   Passed    0.00 sec
        Start 155: t81_vm_system_registers_deterministic_test
 53/107 Test #155: t81_vm_system_registers_deterministic_test .............   Passed    0.00 sec
        Start 156: t81_vm_stubbed_opcode_fail_closed_test
 54/107 Test #156: t81_vm_stubbed_opcode_fail_closed_test .................   Passed    0.01 sec
        Start 157: vm_extended_ops_test
 55/107 Test #157: vm_extended_ops_test ...................................   Passed    0.00 sec
        Start 159: t81_vm_jump_flags_test
 56/107 Test #159: t81_vm_jump_flags_test .................................   Passed    0.00 sec
        Start 160: t81_vm_float_fraction_ops_test
 57/107 Test #160: t81_vm_float_fraction_ops_test .........................   Passed    0.01 sec
        Start 161: t81_vm_literal_pool_extension_test
 58/107 Test #161: t81_vm_literal_pool_extension_test .....................   Passed    0.00 sec
        Start 162: t81_vm_neg_jumps_test
 59/107 Test #162: t81_vm_neg_jumps_test ..................................   Passed    0.00 sec
        Start 163: t81_vm_print_test
... [truncated 102 lines; see local command logs for full output]
```

### Stdlib surface baseline

- Status: PASS
- Command: `/opt/homebrew/opt/python@3.14/bin/python3.14 scripts/governance/check_stdlib_surface_baseline.py`

```text
stdlib surface baseline check PASSED
- modules validated: 13
- fixture directories validated: 13
- fixture tests validated: 1
- collection determinism tests validated: 4
```

### Stdlib promotion snapshot

- Status: PASS
- Command: `/opt/homebrew/opt/python@3.14/bin/python3.14 scripts/governance/check_stdlib_promotion_snapshot.py`

```text
stdlib promotion snapshot check PASSED
- modules validated: 11
```
