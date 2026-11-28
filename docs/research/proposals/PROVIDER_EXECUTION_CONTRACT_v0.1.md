# T81 Provider Execution Contract (M1-A / M1-B Draft Specification)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Provider Execution Contract (M1-A / M1-B Draft Specification)](#t81-provider-execution-contract-m1-a--m1-b-draft-specification)
  - [1. Scope and Non-Goals](#1-scope-and-non-goals)
  - [2. Three Execution Classes](#2-three-execution-classes)
  - [3. Governance & Execution Lifecycle](#3-governance-&-execution-lifecycle)
  - [4. RFC-00D4 Projection](#4-rfc-00d4-projection)
  - [5. Acceptance Tests & Falsification Criteria](#5-acceptance-tests-&-falsification-criteria)

<!-- T81-TOC:END -->


**Status:** Draft (Proposal — Not Yet Promoted to Normative Public API)  
**Schema Versions:** `t81.provider.capabilities.v1`, `t81.provider.evidence.v1`  
**Target Namespace:** `t81::provider` (staged under `src/provider/` to preserve `v2.2.0` `PUBLIC_API_LOCK.json` and frozen TISC `v1.9.0` ISA)  
**Central Principle:** *T81 owns the rules of intelligence execution, governance, and verifiable evidence—not the intelligence itself.*

---

## 1. Scope and Non-Goals

This specification defines a **provider-neutral execution contract** allowing T81 to host interchangeable deterministic and probabilistic AI implementations under a single **Axion** authorization boundary and **CanonFS** evidence persistence sink.

**Non-Goals:**
- Does **not** modify frozen TISC `v1.9.0` opcodes.
- Does **not** mutate `docs/governance/PUBLIC_API_LOCK.json` (`v2.2.0`).
- Does **not** ratify draft `RFC-00D4` as normative; `t81.provider.evidence.v1` is a smaller, deterministic, timestamp-free prerequisite that projects losslessly into `RFC-00D4`.
- Does **not** conflate host-side provider governance with bare-metal `ternaryos/` kernel transactions.

---

## 2. Three Execution Classes

Reproducibility is declared as an explicit provider capability, never a universal promise:

| Class | Schema Enum | Contract | Replay Mode |
| :--- | :--- | :--- | :--- |
| **Deterministic** | `deterministic` | Bit-identical `output_sha3_512` across repeat runs for identical `(artifact_digest, input_digest, params_digest)`. | `bit_exact_replay` (`replayable: true`) |
| **Seeded Probabilistic** | `seeded_probabilistic` | Bounded reproducibility under pinned `(seed, params, runtime_environment)`. Cross-hardware bit-identity is **not** claimed. | `seeded_bounded_attestation` (`replayable: false`) |
| **External Probabilistic** | `external_probabilistic` | Non-deterministic or remote/opaque runtime. Provides provenance, authorization, and I/O digest attestation only. | `provenance_only` (`replayable: false`) |

---

## 3. Governance & Execution Lifecycle

Every execution flows through `ProviderGovernor`:

1. **Preflight Validation & Artifact-Source Precedence (F-01):** Verify provider capability match (`operation`, `media_type`), strict integer/non-negative `ResourceBudget`, `seed` presence for `seeded_probabilistic`, explicit `user_consent_verified` whenever `capabilities.requires_user_consent == true`, and cooperative-cancellation support whenever `timeout_ms > 0`.
   - **Artifact-source precedence:**
     1. If `artifact.canonfs_ref` is supplied: read bytes from `CanonFS`; if missing/unreadable fail closed (`ArtifactDigestError`). If `capabilities.artifact_is_inline_input == true` (both `canonfs_ref` and inline executable bytes supplied), require exact byte and SHA3-512 identity between them.
     2. Else if `capabilities.artifact_is_inline_input == true`: compute `SHA3-512(input.bytes)` independently.
     3. Any mismatch against caller-supplied `artifact.sha3_512` fails closed (`ArtifactDigestError`, `provider_invoked: false`) prior to Axion/provider invocation.
2. **Pre-Execution Cancellation Check:** If `CancellationToken::is_cancelled()` is already true, fail closed (`Cancelled`, `trit: -1`, `provider_invoked: false`).
3. **Axion Authorization Gate & Policy Provenance Binding (F-02, M-03, m-01, D4):**
   - `ProviderGovernor` owns the authoritative execution-policy boundary. Compute `authoritative_policy_sha3_512` upfront from `Policy::serialize()` (canonical little-endian TLV v1) plus a deterministic canonical extension suffix covering the remaining `Policy` fields (`max_tensors`, `allowed_ternary_model_hashes`, `activation_ceiling_*`, etc.) so both preflight denials and post-authorization receipts record the exact active policy digest. Reject caller-supplied `context.policy_sha3_512` mismatches fail-closed (`PolicyDenied`, `provider_invoked: false`).
   - Build `t81::axion::SyscallContext` with `caller = principal_id`, `syscall = "provider:" + operation`, `payload = authoritative_artifact_sha3_512`, and `final_authorization = true`. Evaluate via `t81::axion::PolicyEngine::evaluate()`.
   - Any verdict other than `VerdictKind::Allow` (`Deny`, `Defer`, `Warn`, `Quarantine`) or a null policy fails closed (`PolicyDenied`, `trit: -1`, `provider_invoked: false`). VM-internal `AIHookEngine` safeguards in `T81VmProvider` cannot weaken or bypass `ProviderGovernor` authorization.
4. **Governed Execution & M1 Control Boundaries (F-03):** Invoke `IAiProvider::execute(request, context, cancel)` inside a `try/catch` boundary (mapping uncaught exceptions to `ProviderFault`, `trit: -1`, cleared output).
   - **Cooperative timeout vs. hard preemption:** In-process (`InProcessReadOnly`) timeouts are **cooperative** (providers check `CancellationToken` and `timeout_ms`); hard preemption requires `JailedProcess` isolation (out of M1 scope).
   - **Payload accounting vs. RSS isolation:** In M1, `max_memory_bytes` bounds input and output byte payload sizes (`BudgetExhausted`); it is **not** a process RSS or total-memory isolation guarantee.
5. **Canonical Evidence Sealing & Fail-Closed CanonFS Persistence (F-04, D2):**
   - Serialize canonical sorted-key `t81.provider.evidence.v1` JSON (zero wall-clock timestamps inside the hashed core so deterministic replays produce bit-identical `CanonRef`s).
   - Compute `evidence_sha3_512`, persist into `t81::canonfs::Driver` under mutex (`ObjectType::RawBlock`), and verify read-back `SHA3-512` identity.
   - If persistence or read-back verification fails, fail closed (`EvidencePersistError`, `trit: -1`), clear `output.bytes`, and clear `evidence_ref`, `evidence_json`, and `evidence_sha3_512` so no unpersisted receipt can escape.

---

## 4. RFC-00D4 Projection

`t81.provider.evidence.v1` projects cleanly to draft `RFC-00D4`:
- `meaning` ← `(execution.operation, digests.input_sha3_512, digests.params_sha3_512)`
- `evidence` ← `(digests, parent_evidence_sha3_512)`
- `method` ← `(provider, artifact)`
- `authority` ← `(authority)`
- `challengeability` ← `(challengeability)`
- `accountability` + `timestamp_utc` ← optional outer signing/audit wrapper outside the deterministic `CanonFS` core.

---

## 5. Acceptance Tests & Falsification Criteria

Verified by CTest target `t81_provider_contract_test` (`tests/cpp/provider_contract_test.cpp`):
- **AT-01:** Provider-independent fail-closed Axion denial (`provider_invoked == false`, `invocations == 0`, persisted denial receipt).
- **AT-02:** Deterministic `T81VmProvider` bit-exact repeat replay (`output_sha3_512`, `evidence_sha3_512`, and `CanonRef` bit-identical; `dualrail_fallback_count == 0`, `trit == +1`).
- **AT-03:** Probabilistic `ControlledProvider` consent gate (`ConsentRequired` when unverified) and honest `seeded_probabilistic` / `seeded_bounded_attestation` receipt when consented + seeded.
- **AT-04:** Pre-cancelled token (`Cancelled`, `invocations == 0`), step-budget exhaustion (`BudgetExhausted`), and timeout enforcement (`TimedOut`).
- **AT-05:** Offline verifier rejects 1-byte tampered `CanonFS` evidence and rejects probabilistic bundles mislabeled as `bit_exact_replay`.
- **AT-06:** Inline artifact bait-and-switch, `canonfs_ref` digest mismatch, dual `canonfs_ref` + inline mismatch, and unreadable `canonfs_ref` fail closed (`ArtifactDigestError`, `invocation_count == 0`).
- **AT-07:** Canonical `Policy::serialize()` repeat determinism (D1), caller `policy_sha3_512` mismatch rejection (`PolicyDenied`, `invocation_count == 0`), and D4 non-bypass (permissive VM-internal policy cannot weaken strict `ProviderGovernor`).
- **AT-08:** Uncaught provider exception caught into persisted `ProviderFault`, non-cancellable provider + `timeout_ms > 0` rejected (`CapabilityMismatch`), `max_memory_bytes` payload enforcement (`BudgetExhausted`), and strict integer parsing in `ControlledProvider`.
- **AT-09:** Null/failing `CanonFS` driver clears `evidence_ref`, `evidence_json`, and `evidence_sha3_512` on `EvidencePersistError` (D2).
- **AT-10 & Schema Conformance:** Structural JSON parser rejects string-injected spoofing payloads (`verify_evidence_json`), and `tests/ci/test_provider_schema_conformance.py` validates live serialized JSON against committed schemas.
