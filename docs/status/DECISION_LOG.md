# Decision Log

Status: Active
Last Updated: 2026-03-19
Owner: Project Management / Governance
Version: 1.0.0

## Purpose

Provide a canonical, chronological record of key program decisions: what was
decided, by whom, when, what alternatives were considered, and the rationale.
This log is the authoritative source for decision provenance.

## Decision Format

Each entry records:
- **Decision ID** — sequential, prefix `DEC-`
- **Date (UTC)** — when the decision was made
- **Approver** — who stamped the decision
- **Category** — Governance / Architecture / Release / Implementation / Operational
- **Decision** — the specific action or ruling
- **Alternatives Considered** — what other paths were evaluated
- **Rationale** — why this path was chosen
- **References** — related artifacts

---

## Decisions

---

### DEC-001 — TISC ISA Frozen at v1

**Date (UTC):** Pre-2026 (historical)
**Approver:** @t81dev
**Category:** Architecture

**Decision:** The TISC ISA (`spec/tisc-spec.md`) is frozen at v1. No normative
changes to opcode semantics, encoding, or behavior may be made without a major
version bump.

**Alternatives Considered:**
- Rolling versioned spec with backward-compatible amendments.
- Separate ISA versions for experimental and production paths.

**Rationale:** Bit-exact reproducibility requires a stable ISA substrate. Any
opcode change invalidates existing canonically reproducible programs. Freeze
provides the foundation for the Deterministic Core Profile (DCP) guarantee.

**References:**
- `spec/tisc-spec.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`

---

### DEC-004 — RFC-0026 Phase-1 Opcode Set Extended: WLOAD, GATHER, SCATTER

**Date (UTC):** 2026-03-07
**Approver:** @t81dev
**Category:** Implementation

**Decision:** Extend the RFC-0026 AI-Native Inference Opcodes phase-1 implementation to
include the three remaining opcodes: `WLOAD` (weight tensor load with Axion policy gate),
`GATHER` (sparse row-gather from rank-2 tensor), and `SCATTER` (non-mutating scatter-add
of rank-1 source into rank-2 destination). All six phase-1 opcodes (`ATTN`, `QMATMUL`,
`EMBED`, `WLOAD`, `GATHER`, `SCATTER`) now have full VM dispatch, tensor kernel
implementations, checked wrappers, contract predicates, and dedicated conformance tests
with deterministic output hashes.

**Alternatives Considered:**
- Defer WLOAD/GATHER/SCATTER to a separate RFC-0026 phase-2 milestone.
- Implement only WLOAD and leave gather/scatter for later.

**Rationale:** RFC-0026 explicitly specifies all six opcodes as the phase-1 set. Delivering
them together ensures the conformance suite is complete, the Axion policy event path is
exercised for all AI opcodes, and downstream work (T81Lang lowering in AI-M6) can proceed
with a full target opcode surface.

**References:**

- `spec/rfcs/RFC-0026-ai-native-inference-opcodes.md`
- `include/t81/isa/opcodes.hpp` — `WLOAD`, `GATHER`, `SCATTER` enum entries
- `include/t81/tensor/llama.hpp` — `t81::ops::wload`, `gather`, `scatter_add` kernels
- `include/t81/tensor/contracts.hpp` — `wload_compatible`, `gather_compatible`, `scatter_compatible`
- `core/vm/tensor_helpers.cpp` — `tensor_wload_checked`, `tensor_gather_checked`, `tensor_scatter_checked`
- `core/vm/vm.cpp` — VM dispatch for WLOAD/GATHER/SCATTER
- `tests/cpp/vm_ai_phase1_wload_conformance_test.cpp`
- `tests/cpp/vm_ai_phase1_gather_conformance_test.cpp`
- `tests/cpp/vm_ai_phase1_scatter_conformance_test.cpp`
- `docs/status/AI_RFC_BACKLOG.md`

---

### DEC-003 — Documentation Content-Based Reorganization

**Date (UTC):** 2026-03-06 21:00:00Z
**Approver:** @t81dev
**Category:** Operational

**Decision:** Reorganize `/docs/` directory structure based on actual content rather than legacy naming conventions. Create `user-guide/`, `developer-guide/`, and `process/` directories to improve navigation and maintainability.

**Alternatives Considered:**
- Keep existing directory structure with only link fixes
- Minimal reorganization with only top-level directories
- Complete flattening of all documentation into single level

**Rationale:** Content-based organization improves user experience by separating user-facing documentation from developer internals. Reduces cognitive overhead and makes documentation more discoverable. Legacy structure mixed user tutorials with developer guides and process documents, creating confusion.

**References:**
- `docs/REORGANIZATION_SUMMARY.md`
- `docs/README_LINK_VERIFICATION.md`
- `docs/STATUS_FILES_AUDIT_SUMMARY.md`

---

### DEC-002 — T81Lang Implementation Maturity Promoted to Beta

**Date (UTC):** 2026-02-26 15:05:00Z
**Approver:** @t81dev
**Category:** Governance / Implementation

**Decision:** T81Lang implementation maturity is promoted to Beta. The spec
authority remains Draft (`spec/t81lang-spec.md`). This is an implementation
posture change only; it does not modify normative authority, freeze policy, or
determinism registry boundaries.

**Alternatives Considered:**
- Remain at Alpha/Experimental until spec authority also reaches Beta.
- Promote both spec and implementation simultaneously.

**Rationale:** Gate criteria TG-01 through TG-06 were all passing. BG-01
through BG-05 backlog items were closed. Determinism and conformance slices were
green. Separating implementation maturity from spec authority allows engineering
velocity while spec review continues.

**References:**
- `docs/records/status-history/T81LANG_PROMOTION_GATE.md` (full gate evidence)
- `docs/records/status-history/T81LANG_PROMOTION_GATE_SNAPSHOT.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`

---

### DEC-003 — llama.cpp Integration Classified as Governed Non-DCP

**Date (UTC):** 2026-02-26
**Approver:** @t81dev
**Category:** Architecture / Governance

**Decision:** The llama.cpp integration path (`t81_llama_adapter`,
`tooling/model/llama_cpp_adapter.cpp`, CLI `llama-run`) is classified as
**governed non-DCP**. Determinism claims are bounded to practical reproducibility
evidence only and do not imply registry-Verified deterministic guarantees.
Promotion beyond governed non-DCP requires the governed AGI promotion pipeline.

**Alternatives Considered:**
- Full DCP exclusion with no reproducibility claims.
- Experimental DCP scoping with explicit reproducibility caveats.

**Rationale:** AGI-facing inference surfaces carry non-determinism risk from
model sampling, hardware FP variation, and library version churn. Classifying as
governed non-DCP maintains safety: reproducibility evidence is captured, but DCP
guarantees are not extended. Upgrade path is explicit via the governed AGI
promotion pipeline.

**References:**
- `docs/records/audits/RELEASE_READINESS_PACKET_2026-03.md` (§ AGI-Facing Boundary Classification)
- `docs/status/EXTENSION_PROFILE.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md`
- `CMakeLists.txt` (`T81_EXPORT_LLAMA_ADAPTER` guard)

---

### DEC-004 — t81_llama_adapter Remains Internal / Build-Only

**Date (UTC):** 2026-02-26
**Approver:** @t81dev
**Category:** Architecture / Release

**Decision:** `t81_llama_adapter` is not exported as a package target. It
remains internal and build-only, guarded by `T81_EXPORT_LLAMA_ADAPTER` in
`CMakeLists.txt` with an explicit unsupported-error path.

**Alternatives Considered:**
- Export as an optional package target with governed-non-DCP labeling.
- Ship as a separate add-on package.

**Rationale:** Exporting a non-DCP adapter as a first-class package target risks
downstream consumers treating its outputs as DCP-guaranteed. Build-only
classification prevents this and simplifies the public API contract.

**References:**
- `CMakeLists.txt` (`T81_EXPORT_LLAMA_ADAPTER`)
- `docs/status/TASKS.md` (§ Governed llama.cpp Integration — Packaging Decision)

---

### DEC-005 — Jekyll Pages Build Failure Classified as Deferred

**Date (UTC):** 2026-02-28
**Approver:** PM / @t81dev
**Category:** Release / CI

**Decision:** The `build` (GitHub Pages / Jekyll) workflow failure on candidate
`1ec312e3` is classified as **Deferred** and does not block C++ release. Root
cause is Liquid `{{ }}` syntax in `third_party/llama.cpp` modelcard templates
causing Jekyll rendering errors.

**Alternatives Considered:**
- Block release until Pages build is green.
- Remove third_party/llama.cpp from Pages scope immediately.

**Rationale:** The Pages build failure has no impact on C++ release artifacts,
determinism guarantees, or DCP claims. The `build` workflow is non-required in
branch protection. Immediate scope-exclusion fix would require changes to
`_config.yml` or Pages configuration outside the release window. Deferring is
lower-risk than a last-minute Pages configuration change.

**Follow-Up:** Implemented root Jekyll scope control via repository `_config.yml`
to exclude `third_party/`; pending CI verification before closing risk tracking.

**References:**
- `docs/records/audits/RELEASE_READINESS_PACKET_2026-03.md` (§ Non-required failure)
- `docs/status/CI_GATE_STATUS.md`

---

### DEC-006 — March 2026 Release Candidate: GO on `1ec312e3`

**Date (UTC):** 2026-02-28
**Approver:** PM / @t81dev
**Category:** Release / Governance

**Decision:** The March 2026 release packet is stamped **GO** on candidate SHA
`1ec312e3`. Both required branch-protection contexts (`quality gate / required`,
`Analyze (cpp)`) are `completed` + `success`. Pending final @t81dev approval
stamp to convert recommendation to formal decision.

**Alternatives Considered:**
- Wait for Pages build fix before GO stamp.
- Select a different post-fix main candidate SHA.

**Rationale:** Required contexts are fully satisfied. Non-required failure
(Pages) is classified as Deferred per DEC-005. All determinism gates,
cross-arch bit-identity gates, sanitizer builds, fuzz, formal verification, and
governance-metric checks passed. Waiting for Pages fix would delay the release
window without benefit to release quality.

**References:**
- `docs/records/audits/RELEASE_READINESS_PACKET_2026-03.md`
- `docs/status/PROJECT_CONTROL_CENTER.md` (§ 4.1)

---

### DEC-007 — Test Suite Deduplication (Sprint Close 2026-02-28)

**Date (UTC):** 2026-02-28 (commit `7724578e`)
**Approver:** @t81dev
**Category:** Implementation

**Decision:** Consolidate 10 per-module `cli_std_*_fixtures_test.cpp` files into
a single parameterized `cli_stdlib_fixtures_test.cpp`. 16 files removed,
−1,780 LOC. New stdlib modules require a one-line `STDLIB_MODULES` entry.
Shared utilities extracted to `test_sig_util.hpp` and `test_fixture_util.hpp`.

**Alternatives Considered:**
- Keep per-module test files, enforce a template/generator for new modules.
- Remove duplication selectively rather than wholesale consolidation.

**Rationale:** Per-module files introduced copy-paste drift risk and made adding
new modules high-friction. A single parameterized harness with a one-line
registration point scales better and reduces maintenance surface. 285/285 tests
continue passing post-consolidation.

**References:**
- `docs/records/audits/TEST_SUITE_DEDUP_AUDIT_2026-02-28.md`
- `tests/cpp/cli_stdlib_fixtures_test.cpp`

---

### DEC-008 — CodeQL Push Trigger Added to `main`

**Date (UTC):** 2026-02-26 (commit `ad6c2777`)
**Approver:** @t81dev
**Category:** CI / Governance

**Decision:** Add `push` trigger on `main` to `.github/workflows/codeql.yml` so
that `Analyze (cpp)` runs on every merge to `main`, satisfying the required
branch-protection context.

**Alternatives Considered:**
- Run CodeQL only on schedule and PR.
- Promote a CodeQL-excluded candidate SHA.

**Rationale:** The required context `Analyze (cpp)` was not populated on `main`
push events because the workflow lacked a `push` trigger. Adding the trigger is
the minimal correct fix; alternatives would either leave the required context
unsatisfied or degrade security posture.

**References:**
- `.github/workflows/codeql.yml`
- `docs/records/audits/RELEASE_READINESS_PACKET_2026-03.md`

---

### DEC-009 — Restore `expected.hpp` Fallback and Harden Format Diff Logic

**Date (UTC):** 2026-03-05
**Approver:** @t81dev
**Category:** CI / Implementation

**Decision:** Restore `include/t81/support/expected.hpp` fallback implementation
from a known-good revision and update `.github/workflows/format.yml` checkout
configuration to `fetch-depth: 2` so push diff logic can reliably evaluate
`HEAD^..HEAD`.

**Alternatives Considered:**
- Reconstruct `expected.hpp` manually from scratch.
- Disable or relax failing CI lanes temporarily.
- Run clang-format across entire repository as a one-off mitigation.

**Rationale:** Multiple CI lanes were failing due to a malformed `expected`
fallback implementation, while `Format Check` intermittently scanned the full
tree because shallow checkout lacked `HEAD^` on push. Restoring a previously
validated fallback and fixing checkout depth addressed both failure classes
without reducing gate strictness.

**References:**
- `include/t81/support/expected.hpp` (commit `57f1a96c`)
- `.github/workflows/format.yml` (commit `b20934be`)
- `https://github.com/t81dev/t81-foundation/actions/runs/22722029938`
- `https://github.com/t81dev/t81-foundation/actions/runs/22722029925`

---

### DEC-010 — Close BG-07 via Deterministic BigInt Transport and Fail-Closed Narrowing

**Date (UTC):** 2026-03-05
**Approver:** @t81dev
**Category:** Governance / Implementation

**Decision:** Mark backlog item BG-07 as closed. Oversized integer literals now
lower to `LiteralKind::BigIntHandle`, persist through deterministic binary
transport (`Program::bigint_pool`), materialize in VM as canonical
`FractionHandle` (`BigInt/1`), and no longer silently truncate. Narrowing back
to machine `Int` remains explicitly bounded and fail-closed (`Frac2I` decode
fault on non-`int64` BigInt numerators). JIT explicitly deopts `BigIntHandle`
literals to preserve interpreter precision semantics.

**Alternatives Considered:**
- Keep BG-07 open until full native arbitrary-precision arithmetic opcodes land.
- Narrow language claim to fixed-width integers only and remove BigInt transport.
- Permit best-effort narrowing/truncation for oversized values.

**Rationale:** The primary determinism and safety defect was silent precision
loss across IR->binary->VM boundaries. The implemented path eliminates silent
truncation, preserves deterministic transport/materialization semantics, and
makes precision boundaries explicit at conversion points. This satisfies BG-07
hardening intent without deferring release hygiene on speculative opcode
expansion.

**References:**
- `docs/status/HARDENING_BACKLOG.md`
- `docs/status/BG07_PHASE2_IMPLEMENTATION_NOTES.md`
- `core/vm/vm.cpp`
- `runtime/jit/jit_compiler.cpp`
- `tests/cpp/test_vm_literal_pool_extension.cpp`

---

### DEC-006 — Deterministic Math Implementation for Test Coverage

**Date (UTC):** 2026-03-06 22:50:00Z
**Approver:** @t81dev
**Category:** Implementation

**Decision:** Implement deterministic division and power operations in the T81 math library to resolve test failures and achieve 100% test coverage while maintaining deterministic guarantees.

**Problem Statement:**
Five critical tests were failing due to `T81_DETERMINISTIC` causing division and power operations to return `nae()` (Not a Number Equivalent):
- `t81_matrix_test` - Matrix inverse operations failing
- `t81_matrix_singular_test` - Singular matrix detection failing
- `t81_core_improvements_test` - Scalar operations returning NaN
- `t81_core_features_test` - Matrix multiplication failing
- `t81_cli_stdlib_fixtures_test` - `std.math.pow(1.0, 2.0)` returning NaN instead of 1

**Alternatives Considered:**
- Disable `T81_DETERMINISTIC` flag temporarily (would compromise determinism guarantees)
- Skip failing tests in CI (would hide regressions)
- Use host math fallback with warnings (would violate deterministic contract)
- Implement full deterministic math library (chosen approach)

**Rationale:**
The deterministic math implementation preserves the project's core commitment to reproducible execution while fixing all test failures. By leveraging the existing `DFixed` arithmetic and `t81_soft_math` infrastructure, we maintain consistency with the deterministic architecture and avoid introducing non-deterministic code paths.

**Implementation:**
1. Added `t81::core::detail::div()` function using `DFixed` arithmetic
2. Added `t81::core::math::t81_soft_math::t81_div()` and `t81_pow()` wrappers
3. Updated `T81Float::div()` and `T81Float::pow()` to use deterministic implementations
4. Added template instantiations for all T81Float variants (27,9), (72,9), (18,9)

**Outcome:**
- **Before:** 319/324 tests passing (98.5% success rate)
- **After:** 324/324 tests passing (100% success rate) ✅
- All deterministic guarantees preserved
- No performance impact on non-deterministic builds
- CI now green with perfect test coverage

**References:**
- `include/t81/math/t81_soft_math/t81_soft_math.hpp`
- `include/t81/types/detail/dmath.hpp`
- `include/t81/types/T81Float.hpp`
- `core/math/t81_soft_math/t81_soft_math.cpp`
- `docs/status/PROJECT_CONTROL_CENTER.md`
- `docs/status/ACTIVE_RISKS.md`

---

### DEC-011 — Fix CanonHash\<T81String\> Non-Determinism via serialize_canonical()

**Date (UTC):** 2026-03-07
**Approver:** @t81dev
**Category:** Implementation / Determinism

**Decision:** Add `serialize_canonical()` to `T81String` returning `storage_`
(the normalized string content), so `CanonHash<T>` dispatches through the
content-based path rather than the raw-bytes fallback.

**Problem:** `CanonHash<T81String>` had no specialization and no `serialize_canonical()` method. The generic `CanonHash<T>` fell through to `canon_hash81(reinterpret_cast<const void*>(&value), sizeof(T))`, which hashes `sizeof(std::string)` bytes including uninitialized SSO buffer bytes. Different `T81String` instances with identical content produced different hashes, causing `t81_determinism_containers_test` to fail (test 294 of 325).

**Alternatives Considered:**
- Add a `std::hash<T81String>` specialization using `CanonHash`.
- Specialize `CanonHash<T81String>` directly.
- Fix the raw-bytes fallback to zero-pad uninitialized bytes.

**Rationale:** Adding `serialize_canonical()` is the minimal correct fix that
aligns with the existing `CanonHash<T>` dispatch convention used by all other
T81 types. It is semantically correct (hash is over normalized content), does not
require changes to the hash infrastructure, and makes `T81String` consistent with
the rest of the type system.

**References:**
- `include/t81/types/T81String.hpp` (line 176)
- `include/t81/determinism/canon_hash81.hpp`
- Commit `bda2f089`

---

### DEC-012 — Restore Canonical ci.yml and Apply CI Hardening Fixes

**Date (UTC):** 2026-03-07
**Approver:** @t81dev
**Category:** CI / Operational

**Decision:** Restore `ci.yml` from commit `restore-to-main-293223a8` (the canonical version) and apply three targeted fixes on top: (1) pin lychee to v0.18.1 with absolute `--root-dir`, exclude `github.com/ggerganov/*`, remove `docs/policies/AGENTS.md` from inputs; (2) correct CLI manual path from `docs/guides/` to `docs/user-guide/reference/`; (3) add `timeout-minutes` to all jobs that were missing them.

**Problem:** Multiple CI failures observed:
- lychee defaulted to v0.23.0 which has a tokio channel panic bug (exit 101)
- `--root-dir .` rejected as non-absolute path in lychee v0.18.1
- `docs/policies/AGENTS.md` treated as a URL
- `github.com/ggerganov/*` links returned HTTP 429 (GitHub rate-limits CI runner IPs)
- CLI steps failed with `missing CLI manual: docs/guides/cli-user-manual.md`
- macOS x86_64 GCC benchmark job hung for 26+ minutes on `brew install` with no timeout

**Alternatives Considered:**
- Cherry-pick individual fixes onto the previous ci.yml without restoring canonical.
- Disable lychee entirely and rely on manual link review.
- Remove the macOS GCC matrix entry.

**Rationale:** Restoring the canonical version preserves all new jobs (determinism-repeatability, cross-compile-armv9, Windows matrix, nightly experimental arch) that were missing from the prior working version. Applying fixes as a diff makes the change reviewable and reversible. Pinning lychee to a known-good version is preferable to disabling link checking.

**References:**
- `.github/workflows/ci.yml`
- PR #448 (merged `b566bff8`)
- DEC-011 (T81String fix applied in same window)

---

---

### DEC-013 — Extract IRGenerator to .cpp and Fix SA Dispatch Ordering

**Date (UTC):** 2026-03-08
**Approver:** @t81dev
**Category:** Architecture / Implementation

**Decision:** Extract all `IRGenerator` method implementations from `ir_generator.hpp` into a new `lang/frontend/ir_generator.cpp` translation unit. Simultaneously fix the semantic analyzer's builtin dispatch ordering: move the table-driven (`kBuiltinTable`) fallback block to *after* all custom per-function SA handlers, so custom type-checking code always runs first.

**Alternatives Considered:**
- Keep IRGen as header-only (rejected — excessive compile-time cost, poor separation of concerns).
- Set `needs_custom_sa_check = true` per-function in the registry to suppress early table dispatch (rejected — fragile; requires per-function flag maintenance and still allows ordering bugs to reappear).

**Rationale:** Placing the table-driven dispatch before custom handlers silently bypassed argument-type validation (e.g., `std.math.cos("oops")` passed SA without error). Moving it to a fallback is architecturally correct: custom handlers own their type semantics; the table handles anything not covered. The `needs_custom_sa_check` field is now advisory documentation, not operative gating logic.

**References:**
- `lang/frontend/ir_generator.cpp` (new file, ~5,500 lines)
- `lang/frontend/semantic_analyzer.cpp` (table-driven fallback after custom handlers)
- commit `ccc93563`

---

### DEC-014 — TLOADHASH Null-CanonFS: set_canonfs_root() API + Explicit Driver Attachment

**Date (UTC):** 2026-03-08
**Approver:** @t81dev
**Category:** Architecture / Implementation

**Decision:** Fix the TLOADHASH null-`canonfs_driver_` SEGFAULT by adding a null-guard with hash-format validation in the VM dispatcher. Introduce `set_canonfs_root()` as a new virtual method on `IVirtualMachine` so tests and harnesses can explicitly attach a CanonFS driver without relying on env-var or CWD ambient state.

Trap taxonomy: invalid hash format → `DecodeFault`; valid hash with no driver (canonical miss) → `BoundsFault` + miss event in axion log.

**Alternatives Considered:**
- CWD auto-discovery (fall back to `.t81_canonfs` in current directory) — rejected; broke `t81_vm_ai_phase1_wload_canonfs_audit_test` when the build directory contained a stale `.t81_canonfs` from prior test runs.
- Env-var-only initialization (status quo) — insufficient; tests cannot set env vars without risking cross-test pollution.

**Rationale:** Explicit `set_canonfs_root()` avoids all ambient-state problems. Tests declare exactly what CanonFS driver they use; the VM default (no driver) is well-defined and safe. Correctly distinguishes program errors (bad hash format) from runtime misses (valid hash, no content), matching the AI-M4 contract.

**References:**
- `include/t81/vm/vm.hpp` — `set_canonfs_root()` virtual method
- `core/vm/vm.cpp` — null-guard + `Interpreter::set_canonfs_root()` implementation
- `tests/cpp/vm_tloadhash_conformance_test.cpp`, `vm_tloadhash_canonical_fixed_test.cpp`, `vm_tloadhash_decodefault_determinism_matrix_test.cpp`
- commit `7d92bb09`

---

## Log Maintenance

- New decisions must be logged within the same PR or governance window in which
  they are made.
- Decision entries are append-only; do not edit the rationale or alternatives
  after the decision is stamped. Amendments are new entries referencing the
  original.
- Each entry must have an approver and a UTC date/time.

## Cross-References

- `docs/status/PROJECT_CONTROL_CENTER.md`
- `docs/status/RISK_REGISTER.md`
- `docs/records/audits/RELEASE_READINESS_PACKET_2026-03.md`
- `docs/records/audits/2026-03-governance-review.md`

## Versioning Statement

This log is an operational governance artifact. Entries do not override `/spec`,
freeze policy, or determinism registry boundaries.
