# T1–T8 Trusted-Runner Readiness Matrix (EXP-T81-COMP3-01/r1)

**Date:** 2026-10-29
**Protocol Revision:** EXP-T81-COMP3-01/r1
**Overall Readiness Status:** **TRUSTED RUNNER INCOMPLETE — MEASUREMENT BLOCKED**

---

## Requirements Compliance Matrix

| ID | Requirement Name | Protocol Requirement | Implementation Path | Missing Functionality / Trust Gap | Verification Method | Status |
|:---|:---|:---|:---|:---|:---|:---|
| **T1** | **Process Identity & Key Custody** | Isolated process execution with Ed25519 key custody separate from build workspace. | `runner_r1.py` `verify_signature`, `verify_test_bundle` | Production Ed25519 signing keys and secure enclave/HSM key custody unprovisioned. Synthetic test keys isolated. | `test_r1.py` (`test_wrong_root_and_signature_role`, `test_test_trust_isolation`) | **PASS (Test Scope) / BLOCKED (Production Keys)** |
| **T2** | **Executable & Dependency Binding** | SHA-256 digest binding across source manifest, executable binary, inputs, and environment. | `runner_r1.py` `verify_test_bundle` artifact digest mapping | Production release key sign-off and signed production manifest missing. Local artifact hashing verified. | `test_r1.py` (`test_modified_csv_and_environment`, `test_signed_csv_order_must_match_manifest`) | **PASS (Local Hash) / BLOCKED (Production Attestation)** |
| **T3** | **Single-Process Enforcement** | Guaranteed single-process run with CPU pinning and frequency control. | `benchmark.cpp` `setup_cpu_affinity`, `FrequencyControls` | Frequency governor scaling restriction in unprivileged container handled via explicit fallback (`GOVERNOR_PINNING_SKIPPED_UNPRIVILEGED`). | `validate.py` output check (`CPU_CORE=...`) | **PASS (Unprivileged Fallback)** |
| **T4** | **CPU & RSS Resource Limits** | Strict runtime CPU time (<= 600s) and RSS memory limit (<= 500MB) checks. | `resource_budget.hpp` `check_resource_budget`, `runner_r1.py` `wait4_linux_native` | Full Linux `wait4` native integer rusage accounting verified on x86_64 Linux. | `test_r1.py` (`test_budget_equal_and_exceeded`, `test_linux_native_integer_resources`) | **PASS** |
| **T5** | **Authoritative Terminal Accounting** | Terminal process status accounting with raw wait status verification and exit code verification. | `runner_r1.py` `decode_status`, `check_terminal` | None for test harness. Synthetic diagnostic verified. | `test_r1.py` (`test_real_exit_and_signal_status`, `test_contradictory_and_incomplete_terminal`) | **PASS** |
| **T6** | **Atomic Evidence Capture & Publication** | Atomic record writing, strict JSON canonicalization, and SHA-256 hash preservation. | `runner_r1.py` `canonical`, `strict_json`, `signed_record` | Production publication server / evidence ledger endpoint unconfigured. | `test_r1.py` (`test_noncanonical_json_and_duplicate_keys`, `test_exact_uint64_preservation`) | **PASS (Local Engine) / BLOCKED (Ledger Node)** |
| **T7** | **Crash-Durable Protected Journal** | `fcntl` file locking (`LOCK_EX`), permission isolation (`0600`), attempt limit tracking, and replay detection. | `runner_r1.py` `locked_journal`, `reserve_test_run`, `admit_test_diagnostic` | Production journal daemon unprovisioned. | `test_r1.py` (`test_replay_and_idempotent_read`, `test_reservation_attempt_limit_and_unreserved_id`) | **PASS (Local Journal) / BLOCKED (Production Daemon)** |
| **T8** | **Signature Verification & Replay Rejection** | Strict Ed25519 framed signature verification, role isolation, delegation revocation check, and replay rejection. | `runner_r1.py` `verify_test_bundle` | Production root trust anchor unconfigured. | `test_r1.py` (`test_revoked_test_delegation`, `test_signed_protocol_scope_and_run_order_failures`) | **PASS (Test Engine) / BLOCKED (Production Anchor)** |

---

## Deficiencies & Remaining Production Trust Dependencies

1. **Production Ed25519 Signing Keys:** No production private keys are present or provisioned in this environment. All T1–T8 signature checks pass strictly for `synthetic_diagnostic` test objects and synthetic test keys (`kind: test_only_trust`).
2. **Production Attestation Anchor:** Production signing endpoints (`verify_measurement_bundle`) are explicitly locked and throw `MEASUREMENT_NOT_AUTHORIZED_RUNNER_INCOMPLETE`.
3. **Hardware PMU Requirement:** As documented in `PMU_CAPABILITY_REPORT.md`, hardware PMU instruction and branch-miss counters remain restricted in this container environment.

---

## Conclusion

The T1–T8 verification engine and local resource supervision are **fully implemented and passing 100% of non-performance regression tests**. However, because production cryptographic credentials and hardware PMU counters are unconfigured, production measurement remains blocked.

Final Disposition: **IMPLEMENTATION PARTIAL — MEASUREMENT BLOCKED.**
