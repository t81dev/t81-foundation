# Determinism Audit Log

Last Updated: 2026-03-19
Owner: @t81dev

Chronological record of determinism audits: what was audited, what failed,
what was patched, what remains open. This is long-term credibility capital.

Entries are append-only. Do not edit past entries.

---

## Log

### 2026-03-19 — Axion Epoch Scheduler / Audit Parity Promotion

**Scope:** Operationalize RFC-0043 / RFC-0046 proof obligations for the
experimental TernaryOS + DPE kernel lane.

**Audited:**
- `userland/experimental/tests/epoch_submission_test.cpp`
- `userland/experimental/tests/epoch_audit_test.cpp`
- `.github/workflows/ci.yml` (`axion-epoch-determinism`)
- status / governance surfaces that describe the Axion/TernaryOS boundary

**Findings:**

| Surface | Finding | State |
| :--- | :--- | :--- |
| Kernel epoch execution | Bounded thread-pool and unbounded kernel submit paths now produce identical commit-state results across flat, fan-out, overlap, and diamond epoch shapes | Closed |
| Kernel epoch audit | Success, timeout, policy-fault, and task-fault audit outcomes now match across pooled and unbounded scheduler paths | Closed |
| CI enforcement | Dedicated `axion-epoch-determinism` gate now hard-fails divergence in the experimental kernel lane | Closed |
| Operator-facing status docs | Boundary and status artifacts were lagging the new proof lane and needed refresh | Closed |

**Patches Applied:**
- Added scheduler-parity proofs in `epoch_submission_test.cpp`
- Added audit-parity proofs in `epoch_audit_test.cpp`
- Added `axion-epoch-determinism` job to `.github/workflows/ci.yml`
- Refreshed governance and status docs so the new proof lane is visible in the
  registry, enforcement matrix, control center, CI status, extension profile,
  implementation matrix, and drift records

**Remaining Open:**
- Axion/TernaryOS remains governed non-DCP and experimental at the broader
  runtime boundary.
- Kernel fault-path integration and persistent runtime-state convergence remain
  open in `DRIFT_DECOMPOSITION.md`.

### 2026-03-05 — Status Reconciliation Follow-Up

**Scope:** Reconcile historical open findings with current hardening backlog closures.

**Findings:**

| Prior Open Item | Prior State | Current State | Source of Closure |
| :--- | :--- | :--- | :--- |
| BG-06 (collection determinism tests) | Open | **Closed** | `docs/status/HARDENING_BACKLOG.md` |
| BG-08 (T81Complex binary serialization) | Open | **Closed** | `docs/status/HARDENING_BACKLOG.md` |
| BG-09 (lang-side canonical serialization wiring) | Open | **Closed** | `docs/status/HARDENING_BACKLOG.md` |
| AX-M5 / AX-M6 / AX-M7 (Axion evidence) | Open | **Closed** | `docs/status/HARDENING_BACKLOG.md` |
| T3K-S1 (quantization spec gap) | Open | **Closed** | `docs/status/HARDENING_BACKLOG.md` |

**Remaining Open:** None. BG-07 and FW-02 were both closed on 2026-03-05 in `HARDENING_BACKLOG.md`.

---

### 2026-02-27 — Data Types Determinism Audit

**Scope:** Full audit of core data types for signed-overflow UB, signed-zero
canonicalization, and type-enforcement gaps.

**Audited:**
- `Cell` (signed arithmetic)
- `T81Float` (signed-zero handling)
- `T81Map`, `T81Set` (frontend type enforcement)

**Failures Found:**

| Component | Failure | Severity |
| :--- | :--- | :--- |
| `Cell` | Signed-overflow UB in arithmetic operations — undefined behavior under C++ strict rules | High |
| `T81Float` | Signed-zero not canonicalized — `-0.0` and `+0.0` compared unequal in some paths | Medium |
| `T81Map`/`T81Set` | Frontend type enforcement incomplete — wrong runtime type accepted without error | Medium |

**Patches Applied:**
- `Cell::safe_add` introduced; all arithmetic paths now use checked arithmetic (PR #414)
- `T81Float` signed-zero canonicalization enforced at construction (PR #415)
- `T81Map`/`T81Set` type-enforcement hardened in frontend paths (PR #415)

**Remaining Open:** None from this audit.

**Evidence:** `docs/records/archive/project-reports/determinism_types_audit.md`

---

### 2026-02-28 — Language Surface Determinism Verification

**Scope:** Determinism of newly exposed language types (List, Map, Set, Tree,
Quaternion, Prob, Qutrit, Uint) and canonical serialization coverage.

**Audited:**
- Canonical serialization on 10 types
- Language frontend exposure for collection types
- Empty vs. populated constructor determinism
- DecodeFault behavior under canonical decode paths

**Findings:**

| Component | Finding | State |
| :--- | :--- | :--- |
| List, Map, Set, Tree | `serialize_canonical` added to C++ headers; NOT invoked from language runtime | Open (BG-09) |
| List, Map, Set, Tree | Empty constructors verified deterministic; populated constructors not yet tested | Open (BG-06) |
| T81Quaternion, T81Prob, T81Qutrit, T81Uint | Determinism tests added; fixture conformance suite passing | Closed |
| DecodeFault paths | No faults observed in current corpus | Closed |
| Collection polyfills | List/Map/Set/Tree use `Vector` polyfills — O(N) iterator ops; no correctness failure but algorithmic gap | Open (BG-06) |

**Patches Applied:**
- `serialize_canonical` added to `T81List`, `T81Set`, `T81Tree`, `T81Complex`,
  `T81Symbolic`, `T81Polynomial`, `T81Time`, `T81Entropy`, `T81Promise`, `T81Agent`
- BG-10 closed: determinism tests added for `T81Quaternion`, `T81Prob`, `T81Qutrit`, `T81Uint`
- `t81lang_surface_gate_test` added; AST/IR repro hash baseline refreshed

**Remaining Open:** BG-06 (collection determinism tests), BG-09 (runtime serialization wiring)

**Evidence:** `docs/status/DRIFT_DECOMPOSITION.md` (Language Surface rows)

---

### 2026-02-26 — Structural Integrity Check

**Scope:** Dependency firewall, public API boundary, experimental containment,
legacy path references, build and test pass status.

**Findings:**

| Check | Result | Notes |
| :--- | :--- | :--- |
| Dependency firewall | **PASS** | Controlled waiver existed at the time (`vm/vm.cpp:24`) and was documented |
| Public API boundary (`include/t81/`) | **PASS** | No violations |
| Experimental containment | **PASS** | 1 controlled include waiver at audit time |
| Legacy path references | **PASS** | No stale paths |
| Build (CMake + CTest) | **PASS** | 247/247 tests passing |

**Remaining Open:** None. Historical waiver retired on 2026-03-05.

---

### 2026-02-25 — Verified Surface Audit (Baseline)

**Scope:** Traceability of all six Determinism Surface Registry surfaces.

**Findings:**

| Surface | Traceability | Notes |
| :--- | :--- | :--- |
| TISC Opcode Semantics | **Verified** | Full test coverage; CI enforced |
| VM Interpreter Execution | **Verified** | Full test coverage; CI enforced |
| Data Type Canonical Encoding | **Verified** | Full test coverage; CI enforced |
| Soft-Float Deterministic Math | **Verified** | Full test coverage; CI enforced |
| Compiler Bytecode Emission | **Partial** | Spec gap in `t81lang-spec.md` §5 addressed 2026-02-25 — bounded deterministic compile-profile trace language present; fixture-bounded only |
| T3K Quantization | **Partial** (→ Verified post-gate) | Spec gap: no `spec/t3k-quantization-spec.md` exists; repro gate passes |

**Remaining Open:**
- T3K spec document (`spec/t3k-quantization-spec.md`) has not been authored.
- Compiler Bytecode full spec-section traceability gap remains open (see `DRIFT_DECOMPOSITION.md`).

---

### 2026-02-26 — CodeQL Push Trigger Gap

**Scope:** CI gate — `Analyze (cpp)` not populated on `main` push events.

**Finding:** `.github/workflows/codeql.yml` lacked a `push` trigger on `main`,
causing `Analyze (cpp)` to not run after merges, blocking required-context
satisfaction.

**Patch:** `ad6c2777` — push trigger added to `codeql.yml`.

**Remaining Open:** None.

---

## Audit Cadence

- Full structural integrity check: each release candidate
- Verified surface audit: each monthly governance cadence
- Data type / language surface audits: triggered by implementation changes to
  `include/t81/types/`, `lang/frontend/`, or any type header in `include/t81/`

## Adding Entries

New audit entries must be added at the **top** of the Log section with:
1. Date
2. Scope (what was audited)
3. Failures Found (table)
4. Patches Applied
5. Remaining Open items

---

## Cross-References

- `docs/status/FROZEN_CORE_PROFILE.md`
- `docs/status/DRIFT_DECOMPOSITION.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/records/archive/project-reports/determinism_types_audit.md`
