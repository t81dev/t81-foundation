# Decision Log

Status: Active
Last Updated: 2026-03-05
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
- `docs/status/T81LANG_PROMOTION_GATE.md` (full gate evidence)
- `docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md`
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
- `docs/status/GOVERNED_AGI_PROMOTION_PIPELINE.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
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
