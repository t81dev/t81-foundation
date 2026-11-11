# Post-Merge Baseline Verification and SCG-08 Gate 1 Readiness Review

**Document ID:** `docs/research/continuity/POST_MERGE_BASELINE_AND_SCG08_GATE1_REVIEW_20261111.md`
**Date:** 2026-11-11
**Author:** Jules (T81 Research Continuity Assistant)
**Parent Charter:** `docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md`
**Associated PRs:** PR #770, PR #771, PR #772 (integrated); PR #747, PR #769 (superseded); PR #748, PR #750 (open)

---

## 1. Executive Summary & Authoritative Dispositions

Following the integration of Pull Requests **#770**, **#771**, and **#772** into `origin/main`, an exhaustive, verification-first engineering and methodological assessment was executed against the updated codebase baseline. The evaluation rigorously inspected the semantic analyzer scope-lifetime changes, cross-platform CI portability behavior across Linux, macOS, and Windows runners, release-active VM decoder assertions and DPE parallel execution harnesses, and the mathematical and experimental validity of the SCG-08 Phase 1 Gate 1 arithmetic harness.

### Formal Subsystem Dispositions

| Evaluated Dimension | Status Classification | Bounding Assessment & Justification |
|:---|:---:|:---|
| **Integrated Engineering Baseline** | **VERIFIED FOR EXECUTED LOCAL TESTS** | All local targeted CTest suites (semantic analyzer, frontend, VM decoder, DPE epoch DAG/parallel/thread pool, SCG-08 Gate 1) compiled cleanly and passed 100% (24/24 frontend/semantic tests, 35/35 DPE parallel tests, 30/30 DPE pool tests, 20/20 DPE DAG tests, decoder tests, and 5-width arithmetic tests). |
| **Cross-Platform Hosted CI Matrix** | **PARTIALLY VERIFIED — WINDOWS CTEST FAILURES REMAIN** | Linux (GCC/Clang, sanitizers, determinism slices, AVX2-ASan) and macOS runners pass all build, configuration, and determinism gates. Windows runners (`build / windows-x86_64 / msvc` and `build / windows-x86_64 / clang-cl`) successfully configure CMake and compile all targets, but exhibit remaining CTest execution failures with exit code 8 (3 failed under MSVC, 8 failed under clang-cl in check suite `43070255392`). |
| **Semantic Analyzer Scope Lifetime** | **VERIFIED (DEFENSIVE STABILITY IMPROVEMENT)** | Switching `_scopes` from `std::vector<Scope>` to `std::deque<Scope>` prevents reallocation of the outer scope container during deep nesting. Because individual symbols are stored in `std::unordered_map` whose node addresses are node-allocated and remain stable across map operations, no prior bug of symbol pointer corruption was observed; the change is classified as a defensive scope-container stability improvement. |
| **SCG-08 Phase 1 Gate 1 Readiness** | **ACCEPTANCE PENDING INDEPENDENT REVIEW** | Available local verification supports readiness for independent technical review. Full compliance and formal acceptance remain unestablished pending that review. |
| **SCG-08 Phase 2 (Experimental Campaign)** | **STRICT NO-GO — NOT AUTHORIZED** | Large-scale statistical benchmarks, carry-propagation timing campaigns, and hardware timing remain strictly withheld. Progression to Gate 2 requires a separate, dated maintainer authorization record. |

---

## 2. Authoritative Baseline, Provenance & Merge Ancestry

### 2.1 Baseline Identification
- **Target Repository:** `https://github.com/t81dev/t81-foundation`
- **Authoritative HEAD SHA:** `89223724038ff7dec349343469df4ea8389ede26`
- **Commit Date:** `Wed Nov 11 07:43:57 2026 -0500`
- **Commit Subject:** `Merge pull request #772 from t81dev/fix/ci-backlog-portability-7023967684116356460`

### 2.2 Ancestry & Relationship to Preceding PRs
The lineage of recent pull requests integrated into `main` demonstrates complete representation of intended corrections:

```
*   89223724 (HEAD -> main, origin/main) Merge PR #772 (Portability Reconciled)
|\
| *   8d05e51e Merge branch 'main' into fix/ci-backlog-portability-7023967684116356460
| |\
| |/
|/|
* | b96022c6 Merge PR #771: fix(frontend): reconcile PR #769 against current main baseline
| * eba3b54b Merge remote-tracking branch 'origin/main' into fix/ci-backlog-portability
|/
* a4684582 Merge PR #770: docs(scg08): publish statistical reconciliation and prospective RFC-00E1
* c297125d Revise README.zh-CN.md for clarity and links
* 55bd0aba Update README.ru.md
* c111d083 Revise headings and enhance readability in README.pt-BR
* c768bb9a Update README with language links and content clarity
* e5f58626 Revise links and enhance clarity in README.es.md
* 86983a47 Revise ROADMAP.md with updates and clarifications
* 36b3849a Merge PR #765: fix(scg08): resolve Codex review findings on Gate 1 arithmetic harness
* dbf107fd Merge PR #763: feat/scg08-phase1-gate1-arithmetic-harness
```

### 2.3 Status of Open & Superseded Pull Requests
1. **PR #747 (`fix/ci-backlog-portability`):**
   - **Status:** Open (`merged: false`).
   - **Head SHA:** `b18075ad6de93091f9aea30707ef5237d479c9d1`.
   - **Representation:** All 26 files and functional portability changes from PR #747 were reconciled with `main` and merged via PR #772 (`89223724`).
   - **Recommendation:** Recommend closure of PR #747 with comment: *"Superseded by PR #772 (`89223724`), which reconciled all portability changes onto the current `main` baseline."*
2. **PR #769 (`fix/semantic-analyzer-scope-lifetime-10179265985774715172`):**
   - **Status:** Open (`merged: false`).
   - **Head SHA:** `7c52be13dc0c3fb175271d63f1a8ba9aaeee170c`.
   - **Representation:** The complete 4-file change scope (`CMakeLists.txt`, `PUBLIC_API_LOCK.json`, `include/t81/frontend/semantic_analyzer.hpp`, `tests/cpp/semantic_analyzer_scope_lifetime_test.cpp`) was reconciled and merged via PR #771 (`b96022c6`).
   - **Recommendation:** Recommend closure of PR #769 with comment: *"Superseded by PR #771 (`b96022c6`), which cleanly applied and verified the scope lifetime test and deque container change onto `main`."*
3. **PR #748 (`fix/policy-block-metadata-reconciliation`):**
   - **Status:** Open (`merged: false`).
   - **Head SHA:** `bee490f5c53a97a9479e578d0db65e19f812768f`.
   - **Evaluation:** Independent CanonFS block authorization fix; not superseded by PR #770, #771, or #772. Remains open for separate review.
4. **PR #750 (`fix/cli-execution-policy-reconciliation`):**
   - **Status:** Open (`merged: false`).
   - **Head SHA:** `2695d2f81a5db4c209b6fc1d64bd1cd75bb6e2c5`.
   - **Evaluation:** CLI explicit execution policy enforcement; independent of PRs #770–#772. Remains open for separate review.

---

## 3. Semantic Analyzer Correctness & Scope Stability Assessment

### 3.1 Pointer Stability vs Defensive Container Improvement
PR #771 modified `SemanticAnalyzer` in `include/t81/frontend/semantic_analyzer.hpp`:
```cpp
<<<<<<< PREVIOUS
  using Scope = std::unordered_map<std::string, SemanticSymbol>;
  std::vector<Scope> _scopes;
=======
  using Scope = std::unordered_map<std::string, SemanticSymbol>;
  std::deque<Scope> _scopes;
>>>>>>> INTEGRATED
```
- **Heap Node Stability in `std::unordered_map`:** In standard C++ library implementations, `std::unordered_map` stores key-value pairs (`SemanticSymbol`) in individually allocated bucket nodes. Growing or rehashing a hash map alters the bucket pointers, but individual element addresses remain stable. Furthermore, growing an outer container (`std::vector<Scope>`) reallocates the `Scope` objects (which are the maps themselves), but does **not** reallocate or move the heap-allocated nodes of each `unordered_map`.
- **Enclosing Scope Container Stability:** Moving from `std::vector<Scope>` to `std::deque<Scope>` ensures that references or raw pointers to an entire `Scope` instance (`std::unordered_map<std::string, SemanticSymbol>&`) are never invalidated when `_scopes.emplace_back()` or `_scopes.pop_back()` is executed during deep scope entry and exit.
- **Classification:** Because existing frontend operations did not demonstrate dangling symbol reference corruptions in test suites, this modification is accurately classified as a **defensive scope-container stability improvement** rather than the resolution of a demonstrated runtime crash.

### 3.2 Regression Verification & Test Coverage
The regression suite `t81_semantic_analyzer_scope_lifetime_test` added in PR #771 exercises:
1. Symbol address stability across 256 nested scopes.
2. 100 levels of nested lexical block statement traversal.
3. Function declarations (`FunctionStmt`) and recursive function invocations (`RecurseStmt`).
4. Diagnostic preservation and parameter count error validation.
5. Release-mode assertion activity.

**Test Execution:**
```bash
./build/t81_semantic_analyzer_scope_lifetime_test
```
**Output:**
```
Running semantic analyzer scope lifetime regression tests...
All semantic analyzer scope lifetime regression tests passed!
```
All 24 frontend CTest targets in the build system passed 100%.

---

## 4. Hosted CI Matrix & Windows Portability Audit

### 4.1 Hosted GitHub Actions Check Suite Audit
#### Baseline Commit `89223724038ff7dec349343469df4ea8389ede26` (Check Suite `43022879680`)
All 28 jobs in GitHub Actions check suite `43022879680` were audited:

| Matrix Job Name | Target Platform / Toolchain | Status | Conclusion | Details / Failing Step |
|:---|:---|:---:|:---:|:---|
| `build / linux-x86_64 / clang` | Ubuntu 24.04 (Clang 18) | Completed | **Success** | Built and tested clean |
| `build / linux-x86_64 / gcc` | Ubuntu 24.04 (GCC 14) | Completed | **Success** | Built and tested clean |
| `build / linux-arm64 / clang` | Ubuntu 24.04 ARM64 | Completed | **Success** | Built and tested clean |
| `build / macos-arm64 / clang` | macOS 14 ARM64 (AppleClang) | Completed | **Success** | Runner configuration resolved; all tests pass |
| `build / macos-x86_64 / gcc` | macOS 13 x86_64 (GCC 14) | Completed | **Success** | Dedicated GTest toolchain linking passes |
| `build / sanitizers` | Linux ASan/UBSan | Completed | **Success** | Sanitizer runtime checks pass |
| `gate / tritwise-determinism / avx2-asan` | Linux AVX2 ASan | Completed | **Success** | `-DBENCHMARK_ENABLE_WERROR=OFF` resolved warning promotion |
| `gate / tritwise-determinism / no-simd` | Linux No-SIMD | Completed | **Success** | Deterministic baseline |
| `gate / determinism slice / linux-x86_64` | Linux Clang | Completed | **Success** | Determinism slice verified |
| `gate / determinism repeatability` | Linux Clang | Completed | **Success** | Repeatability verified |
| `gate / axion epoch determinism` | Linux Clang | Completed | **Success** | Axion epoch hash determinism |
| `cxx-std / linux-x86_64 / clang / OFF` | Linux C++20 | Completed | **Success** | C++20 standard build passes |
| `cxx-std / linux-x86_64 / clang / ON` | Linux C++23 | Completed | **Success** | C++23 standard build passes |
| `build / windows-x86_64 / msvc` | Windows Server 2022 (MSVC) | Completed | **Failure** | **Step 25 ('Test'): CTest Exit Code 8** |
| `build / windows-x86_64 / clang-cl` | Windows Server 2022 (Clang-cl) | Completed | **Failure** | **Step 25 ('Test'): CTest Exit Code 8** |

### 4.2 Windows CI Breakdown: Resolved vs Remaining Failures
The fixes integrated in PR #772 successfully addressed several previous blocking issues on Windows:
- **CMake Configuration Unblocked:** Windows sparse-checkout was updated to include `/development_tests/**` and the five required AI-OS contract documents (`STABLE_BASELINE_CONTRACT.md`, `AI_OS_OBJECT_*.md`, etc.), successfully resolving the fatal CMake generation failure at `src/direct_api/CMakeLists.txt:26`.
- **Docs-Only Compliance Check:** The step `Verify stable baseline docs and platform test surfaces` now properly passes with `--docs-only` on Windows (step 22), avoiding execution of non-portable fork/wait test runners.
- **Compilation Clean:** Both MSVC and `clang-cl` successfully compiled all libraries and test executables (step 16).

### 4.3 Windows CTest Execution Analysis (Check Suite `43070255392`)
In check suite `43070255392` (PR #773 reviewed commit `bbfb74cc46`), CTest executed in Step 25:
```bash
ctest --test-dir build --output-on-failure --verbose --output-junit build/ctest-results.xml
```
Process completed with **exit code 8**. The actual executed test suites show distinct failure sets:

#### 1. MSVC Runner (`build / windows-x86_64 / msvc`)
- **Total Tests:** 401
- **Passed:** 398 (99.25%)
- **Failed:** 3 tests
  - Test #280: `vm_ffi_bridge_test` (Exit code `0xc0000409`)
  - Test #306: `t81_cli_repro_hash_test` (Failed)
  - Test #307: `canonfs_cli_portable_storage_test` (Failed)

#### 2. Clang-cl Runner (`build / windows-x86_64 / clang-cl`)
- **Total Tests:** 401
- **Passed:** 393 (98.00%)
- **Failed:** 8 tests
  - Test #50: `t81_bigint_properties_test` (Failed)
  - Test #53: `t81_bigint_division_edge_properties_test` (Exit code `0xc0000409`)
  - Test #57: `fuzz_bigint_libfuzzer` (Exit code `0xc0000409`)
  - Test #58: `t81_fraction_properties_test` (Exit code `0xc0000409`)
  - Test #280: `vm_ffi_bridge_test` (Exit code `0xc0000409`)
  - Test #306: `t81_cli_repro_hash_test` (Failed)
  - Test #307: `canonfs_cli_portable_storage_test` (Failed)
  - Test #345: `t81_std_test` (Failed)

#### 3. Classification of Unresolved Windows Failures
- **Status Classification:** **`WINDOWS TEST FAILURES — ROOT CAUSE NOT YET ESTABLISHED`**
- **Available Diagnostics:** Multiple failures terminate with status code `0xc0000409` (`STATUS_STACK_BUFFER_OVERRUN` / fast-fail assertion), while CLI/CanonFS harness tests report assertion mismatches. Root causes have not been independently diagnosed or established under hosted Windows execution.
- **Evidentiary Boundary:** **No causal connection between the identified Windows test failures and the bounded arithmetic or cross-profile execution-equivalence tests has been demonstrated.**

#### 4. Historical Observations (Check Suite `43022879680` and Prior Runs)
Earlier diagnostic investigations recorded failures in `cli_contract_test` (missing safetensors fixture / backslash path matching) and `check_root_structure` (raw `python3` command invocation without `${Python3_EXECUTABLE}`). These remain preserved as historical diagnostic records from prior runs and must not be conflated with the latest test results in check suite `43070255392`.

### 4.4 Hosted CI Status & Historical Infrastructure Incidents
#### Latest Check Suite `43070255392` Results
- **Full Pass Across Non-Windows Targets:** Linux x86_64 (Clang 18 & GCC 14), Linux ARM64, macOS 14 ARM64, macOS 13 x86_64, ASan/UBSan sanitizers, AVX2-ASan, No-SIMD determinism, Axion epoch determinism, determinism slice & repeatability, frontend fuzzing, static analysis (clang-tidy), VM workload benchmarks, and C++20/C++23 conformance all completed with **Success**.
- **Windows CTest Failures:** MSVC (3/401 failed) and Clang-cl (8/401 failed) failed in CTest step 25 as documented above.
- **Final Matrix Disposition:** **`PARTIALLY VERIFIED — WINDOWS CTEST FAILURES REMAIN`**.

#### Historical Package-Mirror DNS Incident (Check Suites `43045048496` and `43047703843`)
During intermediate verification runs, an external Azure package-mirror resolution outage caused jobs to abort during `sudo apt-get update` with exit code 100 (`Temporary failure resolving azure.archive.ubuntu.com`). This incident was a transient infrastructure provisioning failure; unprovisioned jobs aborted prior to test execution and were not treated as passing. The incident has since fully cleared as evidenced by check suite `43070255392`.

---

## 5. VM and DPE Regression Findings

The VM decoding and DPE parallel execution engines were re-verified against the authoritative baseline:

| Target Name | Total Tests | Passed | Failed | Status |
|:---|:---:|:---:|:---:|:---:|
| `t81_vm_decoder_test` | Exhaustive | All assertions active | 0 | **PASS** |
| `t81_dpe_epoch_dag_test` | 20 | 20 | 0 | **PASS** |
| `t81_dpe_epoch_parallel_test` | 35 | 35 | 0 | **PASS** |
| `t81_dpe_thread_pool_test` | 30 | 30 | 0 | **PASS** |

### Decoder Release-Active Assertions & Greedy Decoding
- `tests/cpp/vm_decoder_test.cpp` enforces Release-mode active assertions (`T81_ASSERT_RELEASE` / `T81_TEST_EXPECT`) to ensure validation cannot be compiled out by `-DNDEBUG`.
- Full-vocabulary greedy decoding, register bounds enforcement, and prefix table decoding remain fully functional without regressions.

---

## 6. SCG-08 Phase 1 Gate 1 Comprehensive Audit

### 6.1 Multi-Precision Arithmetic Representation & Accumulator Sizing
Evaluated across all five signed information capacity bands $B \in \{64, 128, 256, 512, 1024\}$ bits:
$$\mathcal{D}_{\text{signed},B} = \left[-2^{B-1}, \, 2^{B-1} - 1\right]$$

The implementation in `experiments/scg08_arithmetic_work/` strictly conforms to the exact minimum trit requirements:
- $B=64$: 41 op trits, 42 add trits (2 limbs), 81 mul trits (3 limbs).
- $B=128$: 81 op trits, 82 add trits (4 limbs), 161 mul trits (6 limbs).
- $B=256$: 162 op trits, 163 add trits (7 limbs), 323 mul trits (12 limbs).
- $B=512$: 324 op trits, 324 add trits (12 limbs), 646 mul trits (24 limbs).
- $B=1024$: 647 op trits, 647 add trits (24 limbs), 1292 mul trits (48 limbs).

*Mathematical Note:* For $B=512$ and $B=1024$, addition does not require an additional trit because $\lceil \log_3(2^{B+1}+1) \rceil = \lceil \log_3(2^B+1) \rceil$.

### 6.2 Oracle Independence & Numerical Correctness
- `ExactBigIntOracle` (`experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp`) implements an independent, decoupled arbitrary-precision base-$10^9$ integer engine using `std::vector<std::uint32_t>`.
- Multi-precision ternary trits and binary words evaluate to identical integer values in the oracle across all 5 widths.
- Digit canonicalization ($d_i \in \{-1, 0, 1\}$), sign symmetry, asymmetric domain boundaries (e.g. $-2^{B-1}$ and $2^{B-1}-1$), and overflow classifications are verified fail-closed.

### 6.3 Operation Accounting & Weighted Metric ($W_{\text{ops}}$)
The frozen weighted operation metric is defined as:
$$W_{\text{ops}} = \text{OP}_{\text{ALU\_ADD\_SUB}} + 3 \cdot \text{OP}_{\text{ALU\_MUL}} + \text{OP}_{\text{TABLE\_LOOKUP}} + \text{OP}_{\text{BRANCH\_COND}} + \text{OP}_{\text{NORM\_PASS}}$$
- **Ternary Addition:** Each call to `T81Limb27::addc()` performs exactly 38 table lookups (9 in `ADD_TABLE`, 20 in `COMPOSITION_TABLE`, 9 in `CARRY_FROM_ZERO`).
- **Binary Multiplication:** `mul64()` primitive accurately accounts for 4 primitive multiplications and 3 additions; accumulator additions in `mul_binary()` account for 4 additions per inner step.
- **Normalization Costs:** Normalization passes are counted under `op_norm_pass`. Primitive normalization counters are tracked as non-aggregating diagnostic counters and do not alter $W_{\text{ops}}$.
- **Workload Equivalence:** Ternary and binary harnesses execute identical operand pairs generated from deterministic PRNG seed `0x5C608001ULL`.

### 6.4 Falsification Criteria vs Execution Authorization (Separation of Boundaries)
- **Authoritative Falsification Source:** Defined in `SCG08_PHASE0_RESEARCH_CHARTER.md` (§10.5 line 506 and §9 Table):
  *"If balanced-ternary carry normalization overhead exceeds total binary carry resolution time by > 2.0x for all operand widths N in [64, 1024] bits, H02 is FALSIFIED."*
- **Strict Governance Boundary:** Preregistered scientific falsification criteria define stopping conditions under which a hypothesis is rejected. **Falsification conditions do not confer authorization to begin testing or execute Gate 2.**
- **Missing Gate 2 Pre-Requisites:** Gate 2 execution remains **STRICT NO-GO** because the following required pre-requisites are currently missing:
  1. No dated maintainer authorization record approving Gate 2 entry.
  2. No ratified benchmark stopping rules or trial ceilings for large-scale runs.
  3. No ratified protocol amendment choosing between contiguous streams and clustered sequences.
  4. No independent maintainer signature accepting Phase 1 Gate 1.

### 6.5 Statistical Protocol Identity: Contiguous Markov Stream vs Clustered Design
A rigorous distinction must be maintained between the two competing experimental protocols:
1. **Original Preregistered Protocol (Charter §5.3, §10.3):**
   - Single contiguous Markov stream of $10^7$ steps ($10,000,000$ steps).
   - Single initial boundary condition $c_0 = 0$, evaluating asymptotic stationary mixing with eigenvalue $\lambda_3 = 1/9$.
   - Effective sample size $M_{\text{eff}} \approx 0.8M$.
2. **Unratified Proposed Clustered Design (Addendum 03 / Report):**
   - Stratified sequence clusters across 6 length bands totaling $K = 136,500$ clusters ($48,128,000$ digits).
   - Boundary condition $c_0 = 0$ is reset $136,500$ times, inducing a cumulative carry deficit of $\approx -4,265.6$ active carries and systematically suppressing early carries ($\mathbb{E}[\hat{R}_b] < 1/4$).
   - Uses within-stratum centered variance estimation.
- **Protocol Boundary Decision:** Because these protocols measure fundamentally different regimes (asymptotic stationary mixing vs transient-dominated mixtures), **they must not be combined or conflated**.
- **Recommendation:** The proposed 136,500-cluster design should receive its own separate experiment identifier (e.g. `EXP-SCG08-02-CLUSTERED`) and dedicated preregistration prior to any execution.

---

## 7. SCG-08 Authorization & Provenance Decision Matrix

To ensure historical fidelity without backdating, authorization dimensions are formally classified below:

| Dimension | Classification | Authoritative Evidence / Basis | Minimum Acceptance Requirement |
|:---|:---:|:---|:---|
| **Historical Gate 1 Authorization** | **`NOT VERIFIED`** | No dated maintainer signature, PR approval, or commit SHA exists granting Gate 1 implementation authorization in the historical archive. | Retain `NOT VERIFIED`. Do not manufacture or backdate approval. |
| **Historical Addendum 06 Authority** | **`NOT VERIFIED`** | Physical specification document text is absent from git tree; cited only as an external string. | Retained as `NOT VERIFIED`; requirements prospectively formalized under RFC-00E1. |
| **H02 Falsification Threshold ($W_{\text{ops}} > 2.0\times$)** | **`PREREGISTERED`** | Charter §10.5 line 506 and §9 Table. Immediate hypothesis rejection rule. | Established in Charter; does not grant Gate 2 execution permission. |
| **Gate 1 Trial Ceiling ($\le 1,000$)** | **`IMPLEMENTED / HISTORICAL — PREREGISTRATION NOT VERIFIED`** | Implemented in `tier2_work_harness.hpp:67` and enforced fail-closed. | Requires explicit maintainer ratification in prospective Gate 1 acceptance. |
| **Gate 1 Prospective Technical Acceptance** | **`PENDING INDEPENDENT REVIEW`** | Technical harness, 5-width arithmetic, base-$10^9$ oracle, and 12-case negative schema verification are 100% verified locally. | Signed maintainer acceptance record on RFC-00E1 and Gate 1 evidence packet. |
| **Gate 2 Benchmark Execution** | **`STRICT NO-GO / NOT AUTHORIZED`** | Prohibited by Charter §1.3 and governance protocol. | Requires complete Gate 1 sign-off, ratified protocol amendment, and signed Gate 2 authorization. |

---

## 8. Gate 1 Readiness Disposition & Next Engineering Task

### 8.1 Gate 1 Readiness
- **Status:** **`ACCEPTANCE PENDING INDEPENDENT REVIEW`**
- **Basis:** Available local verification supports readiness for independent technical review. Full compliance and formal acceptance remain unestablished pending that review. Historical Gate 1 authorization remains classified as `NOT VERIFIED`, historical Addendum 06 authority remains `NOT VERIFIED`, and prospective specification `RFC-00E1` is a draft awaiting formal ratification.

### 8.2 Gate 2 Disposition
- **Status:** **`STRICT NO-GO — NOT AUTHORIZED`**
- **Basis:** No large-scale statistical campaigns, performance timing, or microarchitectural benchmarks may be executed until Gate 1 is formally accepted by repository maintainers.

### 8.3 Next Research Milestone — Independent Review Manifest
To facilitate formal independent technical review of SCG-08 Phase 1 Gate 1, the following exact artifacts and commands constitute the review package:

1. **Normative Evaluation Contract:**
   - `spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md` (§2 operand domains, §3 $W_{\text{ops}}$ accounting model, §4 decoupled base-$10^9$ oracle, §5 budget ceiling).
2. **Implementation Sources:**
   - `experiments/scg08_arithmetic_work/tier1_markov_harness.hpp` (Markov carry-dynamics transitions).
   - `experiments/scg08_arithmetic_work/tier2_work_harness.hpp` (5-width multi-precision arithmetic, trial budget enforcer).
   - `experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp` (Decoupled base-$10^9$ `ExactBigIntOracle`).
   - `experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp` (Smoke and deterministic replay test suite).
3. **Evidence Artifacts & Schemas:**
   - `experiments/scg08_arithmetic_work/gate1_evidence_record.json` (179 executed trials).
   - `experiments/scg08_arithmetic_work/evidence_schema.json` (JSON schema `t81.scg08.carry-experiment.v1`).
   - `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` (Implementation report).
4. **Independent Validation Commands:**
   ```bash
   # Build test harness
   cmake -B build -G Ninja -DT81_BUILD_EXPERIMENTS=ON -DBENCHMARK_ENABLE_WERROR=OFF
   ninja -C build t81_exp_scg08_01_test
   # Execute arithmetic harness
   ./build/experiments/scg08_arithmetic_work/t81_exp_scg08_01_test
   # Validate schema and 12-case negative fail-closed rejection
   python3 scripts/ci/validate_scg08_gate1_evidence.py
   python3 scripts/ci/validate_scg08_gate1_evidence.py --test-negative
   ```
5. **Core Evaluation Question for Reviewers:** Determine whether prospective Gate 1 technical acceptance under `RFC-00E1` is justified on technical and mathematical merit despite the unresolved historical authorization provenance of `Addendum 06`.

### 8.4 Exactly One Recommended Next Engineering Task
**Recommendation:**
Execute an independent maintainer review of the SCG-08 Gate 1 technical package (prospective RFC-00E1, `gate1_evidence_record.json`, and harness verification) to determine formal maintainer technical acceptance of Phase 1 Gate 1.
