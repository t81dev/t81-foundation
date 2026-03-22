# Behavioral Conformance Expansion Phase 3

Date: 2026-02-26
Last Updated: 2026-02-28
Status: Slice 1 complete — Slice 2 active
Baseline: `d1cef94c`

## Objective

Scale conformance from representative matrix slices toward subsystem-complete invariants while continuing VM integration-layer concentration reduction.

## Slice 1 — Completed 2026-02-28

Completed items:

1. Added deterministic VM trap-family matrix coverage:
   - `tests/cpp/vm_fault_family_determinism_matrix_test.cpp`
   - Families: `DivisionFault`, `BoundsFault`, `TypeFault`, `ShapeFault`, `SecurityFault`
   - Verifies run-to-run trap and signature stability.
2. Added CanonFS read-verify environment contract coverage:
   - `tests/cpp/canonfs_read_verify_env_contract_test.cpp`
   - Verifies env contract across `unset`, `0`, `false`, `OFF`, `1`, and non-empty unknown values.
3. Wired both suites into `CMakeLists.txt` test inventory.
4. Added deterministic `TLOADHASH` decode-fault matrix coverage:
   - `tests/cpp/vm_tloadhash_decodefault_determinism_matrix_test.cpp`
   - Covers malformed CanonFS tensor objects yielding `DecodeFault`.
5. Added mixed workload conformance matrix coverage:
   - `tests/cpp/vm_mixed_workload_conformance_matrix_test.cpp`
   - Combines policy + tensor + memory + branch + sum-type surfaces in one signature-hardened workload.
6. Continued VM extraction on trace/log boundaries:
   - `core/vm/internal/policy_trace_bridge.hpp`
   - `core/vm/policy_trace_bridge.cpp`
   - `vm/vm.cpp`
   - Memory-segment and bounds-fault logging now route through bridge helpers.
7. Expanded `TLOADHASH` deterministic classification matrix:
   - `tests/cpp/vm_tloadhash_decodefault_determinism_matrix_test.cpp`
   - Added explicit deterministic classification coverage for:
     - invalid hash string -> `DecodeFault`
     - CanonFS miss -> `BoundsFault` + `canonfs_miss` reason
     - malformed object decode faults -> `DecodeFault`
8. Expanded mixed workload conformance with deterministic deny-path branch coverage:
   - `tests/cpp/vm_mixed_workload_conformance_matrix_test.cpp`
   - Added low instruction-budget policy case to force deterministic deny during branch loop.
9. Expanded Axion clause-combination conformance invariants:
   - `tests/cpp/axion_policy_conformance_matrix_test.cpp`
   - Added clause-ordering equivalence checks for allow and deny policy sets.
   - Added deterministic Axion-event signature checks to detect order-dependent drift.
10. Reduced VM dispatch concentration for Axion-report/check opcode family:
   - `vm/vm.cpp`
   - Extracted `AxCheck`/`AxReport`/blocked-privileged-Axion handling into dedicated dispatch lambdas.
   - Preserved fail-closed trap and event semantics while reducing switch-body control density.
11. Reduced VM dispatch concentration for blocked-neural and bitwise opcode families:
   - `vm/vm.cpp`
   - Extracted `TNeuralFwd/TNeuralBwd` blocked-op handling and `BitAnd/BitOr/BitXor/BitNot/BitShl/BitShr/BitUShr` execution into dedicated dispatch lambdas.
   - Preserved decode-fault guards, fail-closed neural security traps, and bitwise result/tag/flag semantics.

## Parallel Work (2026-02-26..28, Outside Phase 3 Scope)

The following implementation work landed in the same window but is tracked
separately from Phase 3 and does not count toward Phase 3 slice progress:

- T81Graph lowered to VM native opcodes (PR #424): new `T81Graph` opcodes
  emitted by IR generator; lang-side canonical serialization gap remains open
  (BG-09 in `docs/status/T81LANG_ENGINEERING_BACKLOG_2026-03.md`).
- Core data types determinism audit and remediation (PRs #414, #415).
- Language surface hardening and stress test suite (PRs #420, #404).

These are tracked in `docs/status/PROJECT_CONTROL_CENTER.md` section 4.2.

## Slice 2 — Current Active Slice (March 2026)

Status: Not yet started.

Planned work:

1. Continue VM integration extraction with additional opcode-family dispatch
   splitting to reduce `vm/vm.cpp` control concentration.
2. Expand workload-level determinism/conformance cases toward longer multi-op
   mixes with bounded fault-injection checkpoints.
3. Expand Axion conformance matrices from clause-ordering checks into
   multi-requirement interaction invariants (segment + axion-event + alignment
   combinations).

Exit criteria: Slice 2 items above complete; `cmake --build` clean;
`ctest` full suite green; updated baseline SHA recorded here.
