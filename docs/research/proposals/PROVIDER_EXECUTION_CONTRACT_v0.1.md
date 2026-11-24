# T81 Provider Execution Contract (M1-A / M1-B Draft Specification)

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

1. **Preflight Validation:** Verify provider capability match (`operation`, `media_type`), strict integer/non-negative `ResourceBudget`, `seed` presence for `seeded_probabilistic`, and explicit `user_consent_verified` whenever `capabilities.requires_user_consent == true`.
2. **Pre-Execution Cancellation Check:** If `CancellationToken::is_cancelled()` is already true, fail closed (`Cancelled`, `trit: -1`, `provider_invoked: false`).
3. **Axion Authorization Gate (Provider-Independent):** Build `t81::axion::SyscallContext` with `caller = principal_id`, `syscall = "provider:" + operation`, `payload = artifact.sha3_512`, and `final_authorization = true`. Evaluate via `t81::axion::PolicyEngine::evaluate()`.
   - Any verdict other than `VerdictKind::Allow` (`Deny`, `Defer`, `Warn`, `Quarantine`) or a null policy fails closed (`PolicyDenied`, `trit: -1`, `provider_invoked: false`).
   - Provider `execute()` **MUST NOT** be invoked prior to `Allow`.
4. **Governed Execution:** Invoke `IAiProvider::execute(request, context, cancel)` under budget (`max_steps`, `max_output_tokens`, `max_memory_bytes`, `timeout_ms`) and `CancellationToken`.
   - Exceeding budget (`BudgetExhausted`), timeout (`TimedOut`), cancellation (`Cancelled`), or deterministic fallback violation (`DeterminismViolation`) fails closed (`trit: -1`) and suppresses unverified output release.
5. **Canonical Evidence Sealing & CanonFS Persistence:**
   - Serialize canonical sorted-key `t81.provider.evidence.v1` JSON (zero wall-clock timestamps inside the hashed core so deterministic replays produce bit-identical `CanonRef`s).
   - Compute `evidence_sha3_512`.
   - Persist into `t81::canonfs::Driver` under mutex serialization (`ObjectType::RawBlock`) and verify read-back `SHA3-512` identity.
   - If persistence or read-back verification fails, fail closed (`EvidencePersistError`, `trit: -1`).

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
