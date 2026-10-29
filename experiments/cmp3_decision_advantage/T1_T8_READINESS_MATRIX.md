# T1–T8 Trusted-Runner Readiness Matrix (EXP-T81-COMP3-01/r1)

**Date:** 2026-10-29
**Protocol Revision:** EXP-T81-COMP3-01/r1
**Overall Readiness Status:** **TRUSTED RUNNER INCOMPLETE — MEASUREMENT BLOCKED**

---

## Requirements Compliance Matrix

| ID | Requirement Name | Protocol Requirement | Implementation Path | Trust Gap / Status Justification | Verification Method | Status |
|:---|:---|:---|:---|:---|:---|:---|
| **T1** | **Provisioning and key custody** | Maintainer/operator separation, out-of-band trust anchor, offline maintainer Ed25519 key, restricted runner account with runner key outside repository/sandbox, no signing socket/key/privileges exposed to collector. | `runner_r1.py` `verify_signature`, `verify_test_bundle` | Production maintainer and runner Ed25519 signing keys unprovisioned; production keys missing. Isolated synthetic test keys only. | `test_r1.py` (`test_wrong_root_and_signature_role`, `test_test_trust_isolation`) | **FAIL** |
| **T2** | **Authenticated trust bootstrap and roles** | Maintainer pins maintainer public key out-of-band before runs; maintainer signs runner delegation naming runner key, host, role, revision, packet digest, campaign ID, validity window, run scope; verifier rejects self-signed runner or unauthenticated keys; revocation checked. | `runner_r1.py` `verify_test_bundle` | Production maintainer delegation and production release key sign-off missing. Synthetic test-role delegation tested. | `test_r1.py` (`test_revoked_test_delegation`, `test_signed_protocol_scope_and_run_order_failures`) | **PARTIAL** |
| **T3** | **Exact approval representation** | Maintainer approval is separate immutable `approval.json` signed by pinned maintainer key, binding commit/blob, amendment digest, approved document SHA-256s, M1–M4 flags, campaign ID, allowed host/compiler, delegation digest, retry policy, and measurement authorization flag (defaults false). | `runner_r1.py` `verify_test_bundle` | Unsigned review candidate only. Formal production maintainer approval payload (`approval.json`) uncreated and unsigned. | `test_protocol_identity.py` (`test_prospective_approval_is_not_measurement_authorization`), `test_r1.py` (`test_missing_approval_and_eligibility`) | **PARTIAL** |
| **T4** | **Eligibility and same-object launch** | Maintainer signs eligibility payload binding approved digests, repaired commit, source-file manifest, compiler flags/versions, executable hashes, corpus hashes, assembly hashes, environment profile; supervisor hashes exact executable descriptor/inode to be launched. | `runner_r1.py` `verify_test_bundle` | Production eligibility payload uncreated and unsigned. Tested against synthetic test eligibility objects only. | `test_r1.py` (`test_missing_approval_and_eligibility`, `test_signed_csv_order_must_match_manifest`) | **PARTIAL** |
| **T5** | **Unique reservation and replay protection** | Supervisor allocates 128-bit CSPRNG nonce + campaign ID + monotonically increasing attempt number; reserves in operator-owned append-only journal under filesystem lock (`fcntl LOCK_EX`) before fork; duplicate run IDs or reused signed evidence rejected by verifier. | `runner_r1.py` `locked_journal`, `reserve_test_run` | Local journal mechanism implemented and verified for synthetic runs. Production journal daemon/node unprovisioned. | `test_r1.py` (`test_replay_and_idempotent_read`, `test_reservation_attempt_limit_and_unreserved_id`) | **PARTIAL** |
| **T6** | **Supervisor termination and budgets** | Supervisor forks single-process collector, waits using native Linux `wait4`, captures raw status, decodes WIFEXITED/WEXITSTATUS/WIFSIGNALED/WTERMSIG, records microsecond CPU time (ru_utime+ru_stime) and peak RSS (`ru_maxrss * 1024`), enforces 600s CPU / 500MB RSS limits. | `resource_budget.hpp`, `runner_r1.py` `wait4_linux_native`, `decode_status` | Local single-process wait4 resource accounting fully implemented and verified. Production publication ledger endpoint unconfigured. | `test_r1.py` (`test_budget_equal_and_exceeded`, `test_real_exit_and_signal_status`, `test_linux_native_integer_resources`) | **PARTIAL** |
| **T7** | **Bound terminal manifest and decision order** | RUN payload binds measurement role/schema, run/campaign/attempt IDs, approved protocol/packet/approval/delegation/eligibility digests, source/executable/corpus/assembly hashes, full CSV/stderr hashes, N=10M/R=10, 150 block records, environment profile, supervisor start/exit/raw status, exact CPU/RSS, global/pattern failure status; signed by runner. | `runner_r1.py` `signed_record`, `check_terminal` | Local terminal manifest writing and canonical JSON verification implemented. Production runner signing daemon unprovisioned. | `test_r1.py` (`test_noncanonical_json_and_duplicate_keys`, `test_contradictory_and_incomplete_terminal`) | **PARTIAL** |
| **T8** | **Synthetic/measurement isolation** | Test keys restricted to `test` role and `artifact_kind=synthetic`; measurement verifier accepts only production runner delegation, `artifact_kind=measurement`, and valid measurement authorization; changing kind invalidates signature; signing measurement with test key fails trust check. | `runner_r1.py` `verify_test_bundle` | Test/measurement isolation logic fully implemented and passing. Strictly valid for current tested isolation boundary (`kind: test_only_trust`). Production root trust anchor unconfigured. | `test_r1.py` (`test_test_trust_isolation`, `test_wrong_root_and_signature_role`) | **PASS (for current tested isolation boundary only)** |

---

## Audit Correction Record (2026-10-29 Post-Merge Pass)

This document was updated following post-merge audit finding F2 to reconcile requirement statuses with the frozen protocol amendment (`protocol/r1/PROPOSED_PROTOCOL_AMENDMENT.md` Section 10 M4):

1. **Requirement Identities Realigned:** Restored the frozen requirement definitions and names (T1: Provisioning and key custody, T2: Authenticated trust bootstrap and roles, T3: Exact approval representation, T4: Eligibility and same-object launch, T5: Unique reservation and replay protection, T6: Supervisor termination and budgets, T7: Bound terminal manifest and decision order, T8: Synthetic/measurement isolation).
2. **Accurate Requirement Statuses Restored:** Replaced misleading "PASS" claims in production scope with accurate statuses reflecting implementation gaps (T1: FAIL, T2–T7: PARTIAL, T8: PASS for current tested isolation boundary only).
3. **Removed Unsupported Claims:** Removed claims of production completeness, nonexistent hardware affinity guarantees, and invented daemon/server/HSM requirements not specified in the frozen protocol.

---

## Summary of Production Trust Dependencies

1. **Production Ed25519 Signing Keys:** No production private keys are present or provisioned in this repository or environment. All T1–T8 signature checks pass strictly for `synthetic_diagnostic` test objects using test keys (`kind: test_only_trust`).
2. **Production Attestation Anchor:** Production signing entry points (`verify_measurement_bundle`, `run_production`) are locked and throw `MEASUREMENT_NOT_AUTHORIZED_RUNNER_INCOMPLETE`.
3. **Hardware PMU Requirement:** Hardware PMU instruction and branch-miss counters remain restricted in this environment.

Final Disposition: **IMPLEMENTATION PARTIAL — MEASUREMENT BLOCKED.**
