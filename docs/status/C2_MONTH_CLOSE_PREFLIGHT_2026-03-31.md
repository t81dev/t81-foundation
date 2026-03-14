# C2 Month-Close Preflight Report (2026-03-31)

Generated (UTC): 2026-03-14 16:40:46Z
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
        Start   1: t81_ternaryos_hal_boot_test
  1/345 Test   #1: t81_ternaryos_hal_boot_test ....................................   Passed    0.02 sec
        Start   2: t81_ternaryos_page_alloc_test
  2/345 Test   #2: t81_ternaryos_page_alloc_test ..................................   Passed    0.00 sec
        Start   3: t81_ternaryos_context_switch_test
  3/345 Test   #3: t81_ternaryos_context_switch_test ..............................   Passed    0.00 sec
        Start   4: t81_ternaryos_mmu_test
  4/345 Test   #4: t81_ternaryos_mmu_test .........................................   Passed    0.00 sec
        Start   5: t81_ternaryos_scheduler_test
  5/345 Test   #5: t81_ternaryos_scheduler_test ...................................   Passed    0.00 sec
        Start   6: t81_ternaryos_ipc_test
  6/345 Test   #6: t81_ternaryos_ipc_test .........................................   Passed    0.00 sec
        Start   7: t81_ternaryos_device_driver_test
  7/345 Test   #7: t81_ternaryos_device_driver_test ...............................   Passed    0.01 sec
        Start   8: t81_ternaryos_shell_session_test
  8/345 Test   #8: t81_ternaryos_shell_session_test ...............................   Passed    0.04 sec
        Start   9: tui_snapshot_test
  9/345 Test   #9: tui_snapshot_test ..............................................   Passed    0.01 sec
        Start  10: t81_test_axion_opcodes
 10/345 Test  #10: t81_test_axion_opcodes .........................................   Passed    0.01 sec
        Start  11: t81_vm_neural_opcodes_test
 11/345 Test  #11: t81_vm_neural_opcodes_test .....................................   Passed    0.01 sec
        Start  12: test_bitwise
 12/345 Test  #12: test_bitwise ...................................................   Passed    0.01 sec
        Start  13: t81_vm_resource_monitoring_test
 13/345 Test  #13: t81_vm_resource_monitoring_test ................................   Passed    0.01 sec
        Start  14: t81_tritwise_backend_equivalence_test
 14/345 Test  #14: t81_tritwise_backend_equivalence_test ..........................   Passed    0.01 sec
        Start  15: t81lang_surface_gate_test
 15/345 Test  #15: t81lang_surface_gate_test ......................................   Passed    0.01 sec
        Start  16: t81lang_conformance_edge_semantics_test
 16/345 Test  #16: t81lang_conformance_edge_semantics_test ........................   Passed    0.00 sec
        Start  17: t81_ethics_test
 17/345 Test  #17: t81_ethics_test ................................................   Passed    0.01 sec
        Start  18: t81_ethics_invariants_test
 18/345 Test  #18: t81_ethics_invariants_test .....................................   Passed    0.01 sec
        Start  19: test_resource_monitoring
 19/345 Test  #19: test_resource_monitoring .......................................   Passed    0.01 sec
        Start  20: test_tier3_opcodes
 20/345 Test  #20: test_tier3_opcodes .............................................   Passed    0.01 sec
        Start  21: vm_bounds_trace_test
 21/345 Test  #21: vm_bounds_trace_test ...........................................   Passed    0.01 sec
        Start  22: t81_parser_regression_audit_test
 22/345 Test  #22: t81_parser_regression_audit_test ...............................   Passed    0.01 sec
        Start  23: t81_frontend_parser_appendix_coverage_test
 23/345 Test  #23: t81_frontend_parser_appendix_coverage_test .....................   Passed    0.01 sec
        Start  24: t81_ir_snapshot_audit_test
 24/345 Test  #24: t81_ir_snapshot_audit_test .....................................   Passed    0.02 sec
        Start  25: t81_semantic_analyzer_stage3_rules_test
 25/345 Test  #25: t81_semantic_analyzer_stage3_rules_test ........................   Passed    0.01 sec
        Start  26: vm_reflection_tier2_test
 26/345 Test  #26: vm_reflection_tier2_test .......................................   Passed    0.01 sec
        Start  27: reflective_evidence_replay_test
 27/345 Test  #27: reflective_evidence_replay_test ................................   Passed    0.01 sec
        Start  28: t81_float_deterministic_test
 28/345 Test  #28: t81_float_deterministic_test ...................................   Passed    0.01 sec
        Start  29: t81_bigint_float_conversion_test
 29/345 Test  #29: t81_bigint_float_conversion_test ...............................   Passed    0.01 sec
        Start  30: debug_matrix_crash
 30/345 Test  #30: debug_matrix_crash .............................................   Passed    0.04 sec
        Start  31: t81_float_test
 31/345 Test  #31: t81_float_test .................................................   Passed    0.01 sec
        Start  32: t81_float_properties_test
 32/345 Test  #32: t81_float_properties_test ......................................   Passed    0.02 sec
        Start  33: t81_t81int_to_binary_test
 33/345 Test  #33: t81_t81int_to_binary_test ......................................   Passed    0.01 sec
        Start  34: t81_division_test
 34/345 Test  #34: t81_division_test ..............................................   Passed    0.01 sec
        Start  35: t81_t81int_test
 35/345 Test  #35: t81_t81int_test ................................................   Passed    0.01 sec
        Start  36: t81_t81int_overflow_test
 36/345 Test  #36: t81_t81int_overflow_test .......................................   Passed    0.01 sec
        Start  37: t81_t81int_generic_div_test
 37/345 Test  #37: t81_t81int_generic_div_test ....................................   Passed    0.01 sec
        Start  38: t81_int_sign_trit_test
 38/345 Test  #38: t81_int_sign_trit_test .........................................   Passed    0.01 sec
        Start  39: t81_bigint_test
 39/345 Test  #39: t81_bigint_test ................................................   Passed    0.01 sec
        Start  40: t81_fraction_test
 40/345 Test  #40: t81_fraction_test ..............................................   Passed    0.01 sec
        Start  41: t81_bigint_properties_test
 41/345 Test  #41: t81_bigint_properties_test .....................................   Passed    0.05 sec
        Start  42: t81_prob_properties_test
 42/345 Test  #42: t81_prob_properties_test .......................................   Passed    0.01 sec
        Start  43: t81_t81int_properties_test
 43/345 Test  #43: t81_t81int_properties_test .....................................   Passed    0.03 sec
        Start  44: t81_bigint_division_edge_properties_test
 44/345 Test  #44: t81_bigint_division_edge_properties_test .......................   Passed    0.05 sec
        Start  45: t81_bigint_division_semantics_test
 45/345 Test  #45: t81_bigint_division_semantics_test .............................   Passed    0.01 sec
        Start  46: t81_bigint_gcd_divmod_property_test
 46/345 Test  #46: t81_bigint_gcd_divmod_property_test ............................   Passed    0.04 sec
        Start  47: t81_bigint_modular_inverse_test
 47/345 Test  #47: t81_bigint_modular_inverse_test ................................   Passed    0.01 sec
        Start  48: fuzz_bigint_libfuzzer
 48/345 Test  #48: fuzz_bigint_libfuzzer ..........................................   Passed    0.80 sec
        Start  49: t81_fraction_properties_test
 49/345 Test  #49: t81_fraction_properties_test ...................................   Passed    0.36 sec
        Start  50: t81_core_numeric_compat_test
 50/345 Test  #50: t81_core_numeric_compat_test ...................................   Passed    0.02 sec
        Start  51: t81_core_bigint_compat_properties_test
 51/345 Test  #51: t81_core_bigint_compat_properties_test .........................   Passed    0.15 sec
        Start  52: t81_core_fraction_compat_properties_test
 52/345 Test  #52: t81_core_fraction_compat_properties_test .......................   Passed    0.08 sec
        Start  53: t81_v1_canonical_numeric_contract_test
 53/345 Test  #53: t81_v1_canonical_numeric_contract_test .........................   Passed    0.01 sec
        Start  54: t81_complex_test
 54/345 Test  #54: t81_complex_test ...............................................   Passed    0.01 sec
        Start  55: t81_prob_test
 55/345 Test  #55: t81_prob_test ..................................................   Passed    0.01 sec
        Start  56: t81_symbol_test
 56/345 Test  #56: t81_symbol_test ................................................   Passed    0.01 sec
        Start  57: t81_string_test
 57/345 Test  #57: t81_string_test ................................................   Passed    0.01 sec
        Start  58: t81_qutrit_test
 58/345 Test  #58: t81_qutrit_test ................................................   Passed    0.01 sec
        Start  59: t81_fixed_test
 59/345 Test  #59: t81_fixed_test .................................................   Passed    0.01 sec
        Start  60: t81_uint_test
... [truncated 578 lines; see local command logs for full output]
```

### Determinism slice

- Status: PASS
- Command: `scripts/ci/run_determinism_slice.sh build`

```text
Test project /Users/t81dev/Code/t81-foundation/build
        Start  11: t81_vm_neural_opcodes_test
  1/107 Test  #11: t81_vm_neural_opcodes_test .............................   Passed    0.01 sec
        Start  13: t81_vm_resource_monitoring_test
  2/107 Test  #13: t81_vm_resource_monitoring_test ........................   Passed    0.00 sec
        Start  21: vm_bounds_trace_test
  3/107 Test  #21: vm_bounds_trace_test ...................................   Passed    0.00 sec
        Start  26: vm_reflection_tier2_test
  4/107 Test  #26: vm_reflection_tier2_test ...............................   Passed    0.01 sec
        Start  29: t81_bigint_float_conversion_test
  5/107 Test  #29: t81_bigint_float_conversion_test .......................   Passed    0.00 sec
        Start  39: t81_bigint_test
  6/107 Test  #39: t81_bigint_test ........................................   Passed    0.00 sec
        Start  41: t81_bigint_properties_test
  7/107 Test  #41: t81_bigint_properties_test .............................   Passed    0.05 sec
        Start  44: t81_bigint_division_edge_properties_test
  8/107 Test  #44: t81_bigint_division_edge_properties_test ...............   Passed    0.05 sec
        Start  45: t81_bigint_division_semantics_test
  9/107 Test  #45: t81_bigint_division_semantics_test .....................   Passed    0.00 sec
        Start  46: t81_bigint_gcd_divmod_property_test
 10/107 Test  #46: t81_bigint_gcd_divmod_property_test ....................   Passed    0.04 sec
        Start  47: t81_bigint_modular_inverse_test
 11/107 Test  #47: t81_bigint_modular_inverse_test ........................   Passed    0.01 sec
        Start  48: fuzz_bigint_libfuzzer
 12/107 Test  #48: fuzz_bigint_libfuzzer ..................................   Passed    0.80 sec
        Start  51: t81_core_bigint_compat_properties_test
 13/107 Test  #51: t81_core_bigint_compat_properties_test .................   Passed    0.14 sec
        Start  53: t81_v1_canonical_numeric_contract_test
 14/107 Test  #53: t81_v1_canonical_numeric_contract_test .................   Passed    0.00 sec
        Start  83: t81_vm_canonfs_root_env_contract_test
 15/107 Test  #83: t81_vm_canonfs_root_env_contract_test ..................   Passed    0.01 sec
        Start 105: t81_tensor_serialization_canonical_fixed_test
 16/107 Test #105: t81_tensor_serialization_canonical_fixed_test ..........   Passed    0.37 sec
        Start 120: t81_c_api_bigint_test
 17/107 Test #120: t81_c_api_bigint_test ..................................   Passed    0.00 sec
        Start 123: t81_vm_load_store_test
 18/107 Test #123: t81_vm_load_store_test .................................   Passed    0.01 sec
        Start 124: t81_vm_illegal_test
 19/107 Test #124: t81_vm_illegal_test ....................................   Passed    0.00 sec
        Start 125: t81_vm_bounds_test
 20/107 Test #125: t81_vm_bounds_test .....................................   Passed    0.01 sec
        Start 126: t81_vm_tensor_test
 21/107 Test #126: t81_vm_tensor_test .....................................   Passed    2.67 sec
        Start 127: t81_vm_tloadhash_conformance_test
 22/107 Test #127: t81_vm_tloadhash_conformance_test ......................   Passed    0.13 sec
        Start 128: t81_vm_tloadhash_canonical_fixed_test
 23/107 Test #128: t81_vm_tloadhash_canonical_fixed_test ..................   Passed    0.25 sec
        Start 129: t81_vm_tloadhash_decodefault_determinism_matrix_test
 24/107 Test #129: t81_vm_tloadhash_decodefault_determinism_matrix_test ...   Passed    0.01 sec
        Start 130: t81_vm_ai_phase1_attention_conformance_test
 25/107 Test #130: t81_vm_ai_phase1_attention_conformance_test ............   Passed    2.08 sec
        Start 131: t81_vm_ai_phase1_embed_conformance_test
 26/107 Test #131: t81_vm_ai_phase1_embed_conformance_test ................   Passed    0.36 sec
        Start 132: t81_vm_ai_phase1_qmatmul_conformance_test
 27/107 Test #132: t81_vm_ai_phase1_qmatmul_conformance_test ..............   Passed    1.45 sec
        Start 133: t81_vm_ai_phase1_shared_helper_parity_test
 28/107 Test #133: t81_vm_ai_phase1_shared_helper_parity_test .............   Passed    2.37 sec
        Start 134: t81_vm_ai_phase1_wload_conformance_test
 29/107 Test #134: t81_vm_ai_phase1_wload_conformance_test ................   Passed    0.32 sec
        Start 135: t81_vm_ai_phase1_gather_conformance_test
 30/107 Test #135: t81_vm_ai_phase1_gather_conformance_test ...............   Passed    0.36 sec
        Start 136: t81_vm_ai_phase1_scatter_conformance_test
 31/107 Test #136: t81_vm_ai_phase1_scatter_conformance_test ..............   Passed    0.21 sec
        Start 137: t81_vm_ai_phase1_wload_canonfs_audit_test
 32/107 Test #137: t81_vm_ai_phase1_wload_canonfs_audit_test ..............   Passed    0.67 sec
        Start 138: t81_vm_ai_phase1_scatter_aliasing_test
 33/107 Test #138: t81_vm_ai_phase1_scatter_aliasing_test .................   Passed    0.39 sec
        Start 139: t81_vm_ai_phase1_gather_axis1_test
 34/107 Test #139: t81_vm_ai_phase1_gather_axis1_test .....................   Passed    0.66 sec
        Start 141: t81_vm_tensor_shape_faults_test
 35/107 Test #141: t81_vm_tensor_shape_faults_test ........................   Passed    1.34 sec
        Start 142: t81_vm_tensor_get_set_conformance_test
 36/107 Test #142: t81_vm_tensor_get_set_conformance_test .................   Passed    0.59 sec
        Start 143: t81_vm_tensor_helper_predicates_test
 37/107 Test #143: t81_vm_tensor_helper_predicates_test ...................   Passed    2.22 sec
        Start 144: t81_vm_tensor_provenance_trace_test
 38/107 Test #144: t81_vm_tensor_provenance_trace_test ....................   Passed    0.11 sec
        Start 145: t81_vm_predispatch_policy_deny_logging_test
 39/107 Test #145: t81_vm_predispatch_policy_deny_logging_test ............   Passed    0.01 sec
        Start 146: t81_vm_fault_test
 40/107 Test #146: t81_vm_fault_test ......................................   Passed    0.01 sec
        Start 147: t81_vm_deterministic_fault_test
 41/107 Test #147: t81_vm_deterministic_fault_test ........................   Passed    0.01 sec
        Start 148: t81_vm_fault_family_determinism_matrix_test
 42/107 Test #148: t81_vm_fault_family_determinism_matrix_test ............   Passed    0.22 sec
        Start 149: t81_vm_memory_test
 43/107 Test #149: t81_vm_memory_test .....................................   Passed    0.01 sec
        Start 150: t81_axion_log_determinism_test
 44/107 Test #150: t81_axion_log_determinism_test .........................   Passed    0.01 sec
        Start 156: t81_vm_trace_test
 45/107 Test #156: t81_vm_trace_test ......................................   Passed    0.00 sec
        Start 157: t81_vm_determinism_property_test
 46/107 Test #157: t81_vm_determinism_property_test .......................   Passed    0.02 sec
        Start 158: t81_vm_state_transition_invariants_test
 47/107 Test #158: t81_vm_state_transition_invariants_test ................   Passed    0.00 sec
        Start 159: t81_vm_state_transition_conformance_matrix_test
 48/107 Test #159: t81_vm_state_transition_conformance_matrix_test ........   Passed    0.01 sec
        Start 160: t81_vm_workload_determinism_tiers_test
 49/107 Test #160: t81_vm_workload_determinism_tiers_test .................   Passed    0.30 sec
        Start 161: t81_vm_mixed_workload_conformance_matrix_test
 50/107 Test #161: t81_vm_mixed_workload_conformance_matrix_test ..........   Passed    0.55 sec
        Start 162: t81_vm_policy_parse_fail_closed_test
 51/107 Test #162: t81_vm_policy_parse_fail_closed_test ...................   Passed    0.01 sec
        Start 163: t81_vm_axreport_policy_deny_fail_closed_test
 52/107 Test #163: t81_vm_axreport_policy_deny_fail_closed_test ...........   Passed    0.00 sec
        Start 164: t81_vm_system_registers_deterministic_test
 53/107 Test #164: t81_vm_system_registers_deterministic_test .............   Passed    0.01 sec
        Start 165: t81_vm_stubbed_opcode_fail_closed_test
 54/107 Test #165: t81_vm_stubbed_opcode_fail_closed_test .................   Passed    0.01 sec
        Start 166: vm_extended_ops_test
 55/107 Test #166: vm_extended_ops_test ...................................   Passed    0.00 sec
        Start 168: t81_vm_jump_flags_test
 56/107 Test #168: t81_vm_jump_flags_test .................................   Passed    0.00 sec
        Start 169: t81_vm_float_fraction_ops_test
 57/107 Test #169: t81_vm_float_fraction_ops_test .........................   Passed    0.01 sec
        Start 170: t81_vm_literal_pool_extension_test
 58/107 Test #170: t81_vm_literal_pool_extension_test .....................   Passed    0.01 sec
        Start 171: t81_vm_neg_jumps_test
 59/107 Test #171: t81_vm_neg_jumps_test ..................................   Passed    0.01 sec
        Start 172: t81_vm_print_test
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
