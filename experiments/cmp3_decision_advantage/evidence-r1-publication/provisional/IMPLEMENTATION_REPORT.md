# EXP-T81-COMP3-01/r1 provisional implementation report

**Final disposition: IMPLEMENTATION PARTIAL — BLOCKED**

1. **Source and patch identity.** Original source/unchanged repository HEAD:
`988b18ca9ebdb4a6fb76fe0be30115470e5637f0`, existing PR #637 branch
`research/exp-t81-comp3-01-implementation-2641169195584772356`.
All 36 original experiment files were verified byte-for-byte against that commit
and preserved under `original/`. Repairs exist only in `source/`, outside the
checkout. No new commit exists. Patch SHA-256:
`1123bd8f4d52ce212b69ce796cc6c806d87cfbf11b3bfd4342c8323a2d2889f3`. `git apply --check PROVISIONAL.patch` passes against the pinned
checkout. Repository status still contains only the pre-existing untracked
`build-bench/`. No commit, push, remote PR edit, review resolution, merge,
governance change, activation or experimental performance collection occurred.

2. **Authorization.** All five packet SHA-256 values match the user's pins.
`/private/tmp/t81-comp3-amendment-review/FINAL_APPROVAL_RECORD.md` is unsigned:
M1–M4 remain NOT APPROVED / NOT AUTHORIZED; root/operator/host/key configuration is
UNCONFIGURED. This request authorizes provisional local repair/testing only.
Original #635 commit/blob remain
`b37005ceda6a3f4349ec9027b2a9be84ea176aa9` /
`0fbe0b4da1614867bf51b1c3eaf200f82d789a65`. Frozen protocol bytes and earlier
validation artifacts are untouched. Content hashes are not approval signatures.

3. **Changed files.** All 26 paths are under
`experiments/cmp3_decision_advantage/` (M=modified, A=added, D=deleted):

```text
M README.md
M analyze.cpp
A analyze_r1.py
M baseline1.cpp
M baseline2.cpp
M baseline3.cpp
M baseline4.cpp
M benchmark.cpp
M candidate.cpp
M corpus.hpp
M dispatch.hpp
A fixtures/bootstrap_cases.json
A fixtures/corpus_0_first8.csv
A fixtures/corpus_1_first8.csv
A fixtures/corpus_2_first8.csv
A fixtures/statistical_cases.json
M inspect_assembly.py
A r1_analysis.py
A resource_budget.hpp
A runner_r1.py
D statistics.hpp
D test_analysis.py
D test_classification.cpp
M test_corpus.py
A test_r1.py
M validate.py
```

The old double-based analysis/test path is replaced with experiment-local Python
integer/Fraction analysis to avoid a new C++ big-integer dependency. Its legacy
executable rejects every invocation; the original implementations remain in the
preserved snapshot. This is not a T81 runtime refactor.

4. **Reconciliation against original findings 1–27.** “Implemented” here refers
to the provisional proposed-r1 target, not formal acceptance or #635 equivalence.
Line anchors below refer to the immutable source copy accompanying this report.

| Finding | Local resolution / remaining limitation | Source |
|---|---|---|
| 1 | Provisional authority explicitly pins #635 and proposed r1; no activation. | [README.md:9](source/experiments/cmp3_decision_advantage/README.md#L9) |
| 2 | Predecessor/authority lineage clarified locally; frozen headers remain unchanged. | [README.md:12](source/experiments/cmp3_decision_advantage/README.md#L12) |
| 3 | Archived validation qualified by its original revision; no retrospective conformance claim. | [README.md:15](source/experiments/cmp3_decision_advantage/README.md#L15) |
| 4 | Proposed paired df=9 primary versus Baseline 1 implemented, without pretending it is original Welch design. | [r1_analysis.py:111](source/experiments/cmp3_decision_advantage/r1_analysis.py#L111) |
| 5 | Student-t 95%, 99% and Bonferroni family intervals reported. | [r1_analysis.py:92](source/experiments/cmp3_decision_advantage/r1_analysis.py#L92) |
| 6 | Regression takes precedence over support/inconclusive; global order explicit. | [r1_analysis.py:147](source/experiments/cmp3_decision_advantage/r1_analysis.py#L147) |
| 7 | Positive sub-15% improvement remains INCONCLUSIVE; minimum-effect status separate. | [r1_analysis.py:143](source/experiments/cmp3_decision_advantage/r1_analysis.py#L143) |
| 8 | Unrelated nonsignificance cannot hide a demonstrated regression. | [test_r1.py:94](source/experiments/cmp3_decision_advantage/test_r1.py#L94) |
| 9 | All 18 predefined contrasts use .01/18; descriptive support guards retained. | [r1_analysis.py:14](source/experiments/cmp3_decision_advantage/r1_analysis.py#L14) |
| 10 | All 64 aggregate states tested. Incomplete CSV conservatively blocks the entire diagnostic; unaffected-pattern retention is not implemented. | [test_r1.py:86](source/experiments/cmp3_decision_advantage/test_r1.py#L86) |
| 11 | Closed Pattern-B interval contract and two policy thresholds retained across five forms. | [README.md:45](source/experiments/cmp3_decision_advantage/README.md#L45) |
| 12 | Independent piecewise oracle plus endpoints, adjacent/extreme cases; raw classification and payload separated. | [corpus.hpp:21](source/experiments/cmp3_decision_advantage/corpus.hpp#L21) |
| 13 | Correct existing C++ PCG verified rather than assumed: four vectors and independent recurrence/jump tests; pcg64.hpp unchanged. | [test_r1.py:28](source/experiments/cmp3_decision_advantage/test_r1.py#L28) |
| 14 | Conversion/draw/reset order documented; 24 frozen pairs, 30,000-pair differential check and full 100,000-index identity. | [README.md:50](source/experiments/cmp3_decision_advantage/README.md#L50) |
| 15 | Lifetime user+system integer CPU budget replaces wall clock. Final production supervisor/output-path accounting still incomplete. | [resource_budget.hpp:6](source/experiments/cmp3_decision_advantage/resource_budget.hpp#L6) |
| 16 | 500,000,000-byte peak-RSS polling and exact synthetic terminal tests; Linux kernel enforcement untested. | [test_r1.py:242](source/experiments/cmp3_decision_advantage/test_r1.py#L242) |
| 17 | Authenticated test completion required before statistics; complete CSV plus failure/signal/overrun rejected. Production T1–T8 supervision unfinished. | [runner_r1.py:134](source/experiments/cmp3_decision_advantage/runner_r1.py#L134) |
| 18 | Strict integer grammar, uint64 preservation, duration limits, duplicates and complete coverage checked before inference. | [r1_analysis.py:152](source/experiments/cmp3_decision_advantage/r1_analysis.py#L152) |
| 19 | Distinct CORRECTNESS_FAILURE / PAYLOAD_PARITY tested with injected faults. | [benchmark.cpp:146](source/experiments/cmp3_decision_advantage/benchmark.cpp#L146) |
| 20 | UNRESOLVED: intended Linux affinity/SMT isolation, governor/boost distinctions, readbacks, restoration and pre/post evidence remain incomplete. | [README.md:96](source/experiments/cmp3_decision_advantage/README.md#L96) |
| 21 | Correct Linux enum PERF_COUNT_HW_BRANCH_MISSES retained; frozen spelling not rewritten. | [benchmark.cpp:35](source/experiments/cmp3_decision_advantage/benchmark.cpp#L35) |
| 22 | Operandless ret/nop counted; payload handlers now 3 instructions each. | [test_r1.py:142](source/experiments/cmp3_decision_advantage/test_r1.py#L142) |
| 23 | Per-basic-block assembly inventories and object disassembly added; Mach-O linker-hint labels no longer create false blocks. | [inspect_assembly.py:12](source/experiments/cmp3_decision_advantage/inspect_assembly.py#L12) |
| 24 | Driver invokes regressions/assembly/object checks and checks failure reasons; portable validation distinct from Linux eligibility. | [validate.py:13](source/experiments/cmp3_decision_advantage/validate.py#L13) |
| 25 | BLOCKED: GNU GCC and Linux PMU/environment validation unavailable on this macOS host; no workload or elapsed-time substitute. | [README.md:96](source/experiments/cmp3_decision_advantage/README.md#L96) |
| 26 | Actual registry reference qualified; no promotion or deterministic counter/timing claim. | [README.md:114](source/experiments/cmp3_decision_advantage/README.md#L114) |
| 27 | Historical distributions/falsifier and withdrawn physical-area claims explicitly excluded from executable conclusions; frozen history preserved. | [README.md:116](source/experiments/cmp3_decision_advantage/README.md#L116) |

5. **Executed validation.** Final retained runs:

| Run | PASS | FAIL | SKIP |
|---|---:|---:|---:|
| Clang 22 validation driver checks | 48 | 0 | 1 |
| Apple Clang 21 validation driver checks | 48 | 0 | 1 |
| Latest standalone Python suite (41 test methods) | 40 | 0 | 1 |

Driver counts are command/check counts, not assertions; nested Python methods
must not be added to them. Each compiler driver's Python invocation ran 40 methods
(39 pass, 0 fail, 1 Linux-only skip). The final standalone suite adds the signed
CSV/run-order mismatch regression and passes 40/41, with the same Linux wait4 skip.
Two driver skips cover the unavailable Linux environment group; GNU GCC validation
was not executed and is separately unavailable, not a passing compiler job.
Initial fixture/indexing errors were corrected before these final runs.

Executed cases include four PCG vectors; 24 pinned corpus pairs; all 30,000
independently generated pairs; signed integer/policy boundaries across all five
implementations; distinct injected correctness/parity failures; complete C++ and
independent-jump Python bootstrap-index identities; all 11 statistical
counterexamples (including 70/100 instructions with 11/10 misses); seven exact
bootstrap fixtures; ranks 100/9900; exact 3/20 boundary and binary64-collapse case;
zero and tiny positive variance; Student-t landmarks; all 18 isolated regression
contrasts; all 64 aggregate states; raw grammar/overflow/missing/duplicate cases;
terminal exit/signal/cap boundaries; signatures, tampering, revocation, replay,
role/trust isolation, protocol/artifact mismatches and complete-CSV failed-collector
CLI rejection; assembly operandless instructions and basic-block fixtures.
Subtests are cases inside methods, not separately counted test methods.

Logs: [Clang 22](clang22.log), [Apple Clang 21](apple-clang21.log),
[latest Python](python-tests.log). Machine-readable driver results are
[clang22/results.json](clang22/results.json) and
[apple-clang21/results.json](apple-clang21/results.json).

6. **Compiler/sanitizer and assembly evidence.** Homebrew LLVM Clang **22.1.1**
(`/opt/homebrew/Cellar/llvm/22.1.1/bin`, invoked through
`/opt/homebrew/opt/llvm/bin/clang++`) and Apple Clang **21.0.0**
(`clang-2100.1.1.101`, `/usr/bin/clang++`), target
`arm64-apple-darwin25.5.0`. Both compile every current C++ translation unit with
strict warnings and repeat optimized correctness checks under ASan/UBSan; no
sanitizer findings. GNU GCC is absent from the available toolchain; `/usr/bin/g++`
identifies Apple Clang and is not counted as GCC.

Exact driver invocations (all expanded compile/link/test commands are in logs):

```sh
PYTHONDONTWRITEBYTECODE=1 python3 /private/tmp/t81-comp3-r1-provisional/source/experiments/cmp3_decision_advantage/validate.py --compiler /opt/homebrew/opt/llvm/bin/clang++ --objdump /opt/homebrew/opt/llvm/bin/llvm-objdump --out /private/tmp/t81-comp3-r1-provisional/clang22
PYTHONDONTWRITEBYTECODE=1 python3 /private/tmp/t81-comp3-r1-provisional/source/experiments/cmp3_decision_advantage/validate.py --compiler /usr/bin/clang++ --objdump /opt/homebrew/opt/llvm/bin/llvm-objdump --out /private/tmp/t81-comp3-r1-provisional/apple-clang21
PYTHONDONTWRITEBYTECODE=1 python3 /private/tmp/t81-comp3-r1-provisional/source/experiments/cmp3_decision_advantage/test_r1.py
```

Optimized compile flags: `-std=c++20 -O3 -fomit-frame-pointer -fno-lto -Wall
-Wextra -Wpedantic -Wconversion -Wshadow -Werror`; assembly adds
`-S -fverbose-asm`. Sanitized builds append `-O1 -g -fno-omit-frame-pointer
-fsanitize=address,undefined -fno-sanitize-recover=all`, overriding O3/frame flags.
Each source compiles separately; links use `-fno-lto`, plus sanitizer flags where
applicable. All seven benchmark translation units and the disabled analyzer build
in both configurations. Python is 3.14; OpenSSL is 3.6.2 (Ed25519 CLI verification).

Both toolchains emit the same static wrapper totals:

| Form | A/C wrapper | Pattern-B wrapper |
|---|---:|---:|
| Candidate | 8 | 9 |
| Baseline 1 | 7 | 7 |
| Baseline 2 | 8 | 10 |
| Baseline 3 | 7 | 7 |
| Baseline 4 | 9 | 10 |

Each shared payload contains 3 instructions, including ret (9 total). A/C candidate
and Baseline 2 have identical instruction lowering on both compilers. Pattern-B
lowering remains asymmetric (candidate 9 vs Baseline-2 10); Baseline 1 and 3 have
equal A/C lowering, while other dispatch forms differ. Comparable wrapper/noinline
boundaries are preserved, not equal static counts. Per-block inventories and real
object disassembly are in `clang22/assembly.json` and
`apple-clang21/assembly.json`; raw `.s`/`.o` are retained in optimized directories.
Mach-O object labels can use aliases such as ltmp0; object span counts may include
padding. These are static implementation observations, not PMU measurements,
dynamic instruction estimates or evidence of ternary advantage.

7. **Linux readiness.** Host: macOS 26.5.2 build 25F84, Darwin 25.5.0, ARM64.
No Linux execution environment or GNU GCC is available in the probed PATH/tools.
The no-workload `--probe-pmu` returns exit 6 with exact PMU_UNAVAILABLE on both
optimized binaries. **MEASUREMENT BLOCKED — PMU_UNAVAILABLE.** No hardware counter
values or elapsed-time proxy results were collected. Actual Linux instructions /
branch misses, affinity readbacks, SMT isolation, governor/boost permissions and
restoration, and native integer wait4/rusage validation remain unexecuted.
Windows portability is also untested; no production support is claimed there.

8. **Trusted runner.** The synthetic verification interface is tested with
short-lived, clearly test-only Ed25519 keys generated in temporary unit-test
directories and cleaned up. It verifies role-framed canonical approval,
delegation, eligibility and completion records against an explicitly supplied
test trust store. Failed exit/signal and final CPU/RSS checks precede CSV
eligibility/inference. Test diagnostics always retain measurement BLOCKED.
Production verification and launch unconditionally reject; there is no test-key
measurement override. No production credentials or trust anchors were generated,
installed or asserted.

9. **Remaining defects and dependencies.** Production T1–T8 is not complete:
separate protected supervisor/collector identities and credentials; same-file
hash/execution and dependency binding; single-process restrictions; authoritative
output/exit accounting; atomic capture/terminal publication; protected crash-durable
journals; validated source/executable/corpus/environment binding and pre/post
control evidence are not implemented end to end. Test journals are not production
ledgers. Actual custody/isolation/supervisor-crash and intended-host launch tests
are unexecuted; signature algebra tests do not establish them. The production
verifier is an explicit disabled interface, not a completed implementation.
The inherited Linux frequency-control code lacks approved full readback,
unsupported-versus-denied distinctions and isolation evidence; PMU-open error
classification also needs intended-host review. Native Linux resource code exists
but was neither compiled nor executed here. Incomplete bundles conservatively
block globally; unaffected-pattern diagnostic retention is not implemented.
Formal signed M1–M4 approval, human-provisioned root/operator/runner/campaign
configuration, completed implementation and later intended-host eligibility remain
required; none is inferred from this report. No scientific acceptance or
measurement-readiness claim is made.

10. **Review/discard/reproduction.** [PROVISIONAL.patch](PROVISIONAL.patch),
[exact inventory and output hashes](CHANGED_FILES.json), and
[original snapshot hashes](ORIGINAL_SOURCE_MANIFEST.json) are beside this report.
`source/` is the reviewable provisional copy and `original/` is the verified
baseline. `git apply --check /private/tmp/t81-comp3-r1-provisional/PROVISIONAL.patch`
was executed successfully against the unchanged repository. Applying the patch
is not required for inspection; do not publish or activate it under this request.
Stop reached: local non-performance validation complete for available paths;
required Linux execution and production runner work remain blocked/incomplete.
