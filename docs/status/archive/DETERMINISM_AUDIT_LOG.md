# Determinism Audit Log

Last Updated: 2026-03-22
Owner: @t81dev

Chronological record of determinism audits: what was audited, what failed,
what was patched, what remains open. This is long-term credibility capital.

Entries are append-only. Do not edit past entries.

---

## Log

### 2026-03-22 — VM Fuzzer: Three-Wave int-Narrowing OOB Security Fix

**Scope:** libFuzzer-driven security audit of `fuzz_vm` and `fuzz_parser` harnesses.
Found and closed three separate `int`-narrowing OOB vulnerabilities in `vm/vm.cpp`.

**Audited:**

- `vm/vm.cpp` — all `reg_ok`, `mem_ok`, and `check_mem` bounds-checking lambdas
- `vm/internal/memory_segments.hpp` + `vm/memory_segments.cpp` — `t81::vm::internal::mem_ok` signature
- `tests/fuzz/fuzz_vm.cpp` / `tests/fuzz/fuzz_parser.cpp` — harness instruction encoding
- Crash corpus: 9 minimised inputs across three fault classes

**Findings:**

| Location | Vulnerability | Class |
| :--- | :--- | :--- |
| `step()` → `reg_ok(int r)` (line 607) | `insn.b` (int64_t) truncated before bounds check; passes 0-based check, indexes with original int64 → OOB heap access | CWE-190 (int truncation) → OOB |
| `step()` → `mem_ok(int addr)` / `check_mem(int addr)` (lines 610–618) + `t81::vm::internal::mem_ok(int addr)` | Same truncation in Load/Store handler memory-address path | CWE-190 → OOB |
| `handle_axread/axset/axverify/axcheck/axreport` → local `reg_ok(int r)` (5 sites, lines 7162–7357) | Axion opcode handlers each had their own `int`-typed lambda; AxCheck also indexed `ctx.registers[insn.b]` with unguarded int64_t after the narrowed guard passed | CWE-190 → OOB |

**Patches Applied:**

- `vm/vm.cpp` — widened `reg_ok`, `mem_ok`, `check_mem` lambdas in `step()` (wave 1 + 2)
- `vm/vm.cpp` — widened 5 local `reg_ok` lambdas in Axion handlers + explicit `static_cast<std::size_t>` on `insn.a/b` register-index accesses in `handle_axcheck` (wave 3)
- `vm/internal/memory_segments.hpp` — `mem_ok` declaration: `int addr` → `std::int64_t addr`
- `vm/memory_segments.cpp` — `mem_ok` implementation signature widened to match

**Verification:**

- All 9 crash inputs verified: `fuzz_vm <crash> → exit 0` post-fix
- 406/406 main tests passing (AArch64, 2026-03-22)
- 120-second post-fix fuzz run: 113,027 executions, no new crashes
- Fuzz corpus: 79 minimised VM seed inputs committed to `tests/fuzz/corpus/vm/`

**Evidence:**

- `tests/fuzz/crashes/vm/` — 9 archived crash inputs
- `tests/fuzz/corpus/vm/` — 79-entry minimised corpus

---

### 2026-03-22 — Axion AgentInvoke Runtime Integration Evidence

**Scope:** Close the policy-enforcement evidence gap at the AgentInvoke VM boundary
identified in `DRIFT_DECOMPOSITION.md`. The existing `agent_constructs_test.cpp`
([RFC-0015-01..09]) verified compilation correctness but did not prove the Axion policy
engine is wired to the per-instruction `eval_axion_call` gate for AgentInvoke dispatch.

**Audited:**

- `vm/vm.cpp` — per-instruction `eval_axion_call(kStep)` gate and AgentInvoke audit path
- `include/t81/axion/policy_engine.hpp` / `kernel/axion/policy_engine.cpp` — `LimitInstructions` bytecode op; `evaluate_internal` deny condition (`instruction_count > max_instructions`)
- `include/t81/vm/vm.hpp` — `make_interpreter_vm(engine)` injection API
- `include/t81/isa/program.hpp` — `axion_policy_text` field and `load_program()` policy install path
- `tests/cpp/axion_agent_invoke_policy_test.cpp` — new integration evidence test

**Findings:**

| Surface | Finding | State |
| :--- | :--- | :--- |
| AgentInvoke audit event | Logged with `reason="agent-invoke"` on every invocation | Confirmed present |
| Policy-text path (`axion_policy_text`) | `load_program()` parses policy and installs `PolicyEngine`; per-instruction gate enforces `max-instructions` | Confirmed wired |
| Engine-injection path (`make_interpreter_vm(engine)`) | Externally supplied engine replaces default allow-all; same `eval_axion_call` gate used | Confirmed wired |
| Deny verdict produces SecurityFault | When `instruction_count > max_instructions` the VM returns `Trap::SecurityFault`, not `Halt` | Confirmed |

**Patches Applied:**

- Added `tests/cpp/axion_agent_invoke_policy_test.cpp` — 5 test functions, 9 assertions ([AI-01..05])
- Registered in `CMakeLists.txt` as `axion_agent_invoke_policy_test` linked against `t81_vm t81_axion t81_core`

**Remaining Open:**

- `eval_axion_call` is called before the AgentInvoke body enters; the audit event inside the
  `AgentInvoke` case itself uses a hardcoded `Allow` verdict rather than calling `eval_axion_call`.
  This is correct by design (RFC-0015 §3.2 audit-only semantics for the dispatch record); the
  per-instruction gate fires separately. No change needed.
- TernaryOS kernel integration remains experimental and deferred.

**Evidence:**

- `tests/cpp/axion_agent_invoke_policy_test.cpp`
- 406/406 tests passing on AArch64 (2026-03-22)

---

### 2026-03-22 — HostFloat Result Representation Fix (Track K) + Matmul Fast Path (Track L)

**Scope:** Correct numeric-class tagging on native unary fast paths and remove
O(N) DFixed canonical-fixed cache builds from HostFloat tensor construction in
the ternary-native inference path.

**Audited:**

- `vm/tensor_helpers.cpp` — `native_tensor_unary_exp_direct`, `_silu_direct`, `_softmax_direct`
- `include/t81/tensor/matmul.hpp` — `ops::matmul`, `ops::qmatmul`
- `include/t81/tensor/reduce.hpp` — `contract_dot`
- `include/t81/tensor/unary.hpp` — `exp`, `sqrt`, `log`
- `include/t81/tensor/llama.hpp` — `silu`, `softmax`, attention block
- `tests/cpp/vm_tensor_test.cpp` — TExp/TSiLU/TSoftmax numeric-class assertions
- `benchmarks/BM_NativeWeightsExecution.cpp` — chained TExp→TMatMul benchmark

**Findings:**

| Surface | Finding | State |
| :--- | :--- | :--- |
| `native_tensor_unary_exp_direct` / `_silu_direct` / `_softmax_direct` | Tagged output tensor `ExactInt` instead of `HostFloat` — semantically wrong for transcendental float results; caused downstream matmul to treat the tensor as strict-core eligible, triggering O(N) DFixed builds | Closed |
| `ops::matmul` / `contract_dot` / `unary::exp` / `silu` / `softmax` / attention | `has_canonical_fixed_data()` called before `strict_core_eligible()` — cache built eagerly even for HostFloat operands whose DFixed data is never used | Closed |
| `ops::matmul` scalar path | Used `deterministic_fma` (3× T81Float round-trip) for HostFloat result tensors — semantically incorrect and ~570 000× slower than IEEE float multiply | Closed |
| Result tensor construction in `ops::matmul` | Two-argument `T729TensorBase` constructor calls `initialize_canonical_storage_mode_()` which eagerly builds DFixed cache on ALL output elements regardless of class | Closed |

**Patches Applied:**

- Changed `ExactInt` → `HostFloat` in three `vm/tensor_helpers.cpp` native unary fast paths
- Added `strict_core_eligible()` outer guard before `has_canonical_fixed_data()` in 7 locations across `matmul.hpp`, `reduce.hpp`, `unary.hpp`, `llama.hpp`
- Added `result_class == HostFloat` branch in `ops::matmul` scalar path (IEEE float multiply, no `deterministic_fma`)
- Replaced `T729DynamicTensor({m,n}, std::move(c))` with `T729DynamicTensor::from_host_float_data({m,n}, std::move(c), result_class)` in matmul result construction — skips eager DFixed cache build entirely
- Updated 3 test assertions in `vm_tensor_test.cpp` (TExp/TSiLU/TSoftmax: `ExactInt` → `HostFloat`)
- Added `BM_NativeWeightsExpThenMatMul_T81Native` benchmark (chained WeightsLoad→TExp→TMatMul)
- RFC-00BB §6.3 updated with execution evidence and benchmark numbers

**Performance impact:** Chained `WeightsLoad → TExp → TMatMul` path: ~4 160 ms/iter → **0.0073 ms/iter** at 64 elements; 10–873× faster than binary BigInt reference at sizes 64–4096.

**Remaining Open:**

- `deterministic_fma` still used for ExactTrit×ExactTrit paths — correct behaviour, no change needed.
- `experimental_native → native_supported` progression for dense decoder families unblocked; remaining gates are per RFC-00BB §7 (GGUF schema coverage, multi-head attention profile).

**Evidence:**

- `docs/records/status-history/TRACK_K_RESULT_REPRESENTATION_EVIDENCE_2026-03-22.md`
- `docs/records/status-history/TRACK_L_HOSTFLOAT_MATMUL_EVIDENCE_2026-03-22.md`
- Commit: `ffe867ff`

---

### 2026-03-19 — Axion Epoch Scheduler / Audit Parity Promotion

**Scope:** Operationalize RFC-0043 / RFC-0046 proof obligations for the
experimental TernaryOS + DPE kernel lane.

**Audited:**
- `experimental/ternaryos/tests/epoch_submission_test.cpp`
- `experimental/ternaryos/tests/epoch_audit_test.cpp`
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
| Dependency firewall | **PASS** | Controlled waiver existed at the time (`core/vm/vm.cpp:24`) and was documented |
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
  `core/types/`, `lang/frontend/`, or any type header in `include/t81/`

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
