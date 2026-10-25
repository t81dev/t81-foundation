# PR #566 CI failure attribution

Status: BLOCKED; no merge recommendation. Evidence snapshot: 2026-10-25T10:12:43-04:00.

PR #566 head: `5bfe915531f0629b3dabb15cda6c374fe86945a0`; exact main base: `f6d4d680709c5c6c98b305628e04346f5ef61620`.
PR #565 remains at `736e4fd9754936b4851564881cfb244e359d916e`.
Neither PR, its branch, branch protection, or main was modified in this audit.
The user's untracked `build-bench/` was preserved. Research Protocol 001 remains NOT READY.

## Baseline method and limits

Inspected live GitHub checks and exact failed-job logs before changes. PR #566 changes only documentation and the documentation-lint job. Parsed YAML comparison proves all other job definitions, including build-and-test and tritwise-determinism, are identical to main. Build sources, tests, CMake/dependency definitions and fixture requirements are unchanged. This alone was not treated as native reproduction evidence.

Main scheduled run [40175545412](https://github.com/t81dev/t81-foundation/actions/runs/40175545412) checked out exact main but skipped relevant jobs after documentation lint failed. It is not a passing or equivalent native baseline.

Created preserved audit-only branch `audit/pr566-main-ci-baseline`, never proposed for merge. Its dispatch harness copies the existing job steps, commands, matrices and runner labels; selects the affected lanes; prefixes names with `audit baseline`; removes preflight dependencies solely to observe isolated jobs; and pins every checkout to exact main. Production/default-branch and PR workflows/checks are unchanged. No quality-gate result is emitted by this observational harness.

- Harness commit `2a8f2fd81982ac97236369256070282f360927cc`, [run 40199911123](https://github.com/t81dev/t81-foundation/actions/runs/40199911123): Windows MSVC, Windows clang-cl, AVX2/ASan.
- Harness commit `568844091fa603c5e375f7ddb34462409d9ca252`, [run 40200045548](https://github.com/t81dev/t81-foundation/actions/runs/40200045548): macOS ARM64.

All four native baseline jobs checked out main, independently failed with identical compiler/CMake error diagnostics, and used the same runner image releases and compiler versions as their PR counterparts. Commands were verified equal programmatically. Cache keys differ by audit ref; Windows uses the existing forced-clean configure and AVX2 has no build cache. Images/compiler identity and failure fingerprints are recorded in evidence JSON. The baseline is equivalent for attributing these failure signatures, not a complete equivalent CI suite.

## Every completed failed job at this snapshot

| PR job | Classification | Evidence and actual stage |
| --- | --- | --- |
| [Windows MSVC 121186452330](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121186452330) | Pre-existing on main; native reproduced | [main job 121188699385](https://github.com/t81dev/t81-foundation/actions/runs/40199911123/job/121188699385): CMake generation fails at src/direct_api/CMakeLists.txt:26 because development_tests/test_real_direct_api.cpp is absent from sparse checkout. |
| [Windows clang-cl 121186452359](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121186452359) | Pre-existing on main; native reproduced | [main job 121188699549](https://github.com/t81dev/t81-foundation/actions/runs/40199911123/job/121188699549): identical missing-source/No SOURCES diagnostics. |
| [AVX2/ASan 121185892781](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121185892781) | Pre-existing on main; native reproduced | [main job 121188699790](https://github.com/t81dev/t81-foundation/actions/runs/40199911123/job/121188699790): GCC 15.2.0 compilation of Google Benchmark v1.9.5 benchmark_register.cc emits six libstdc++ std_function.h maybe-uninitialized diagnostics promoted by -Werror. Equivalence and benchmark smoke tests never execute. This is not a demonstrated runtime ASan failure or determinism divergence. |
| [macOS ARM64 121186452339](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121186452339) | Pre-existing on main; native reproduced | [main job 121189107288](https://github.com/t81dev/t81-foundation/actions/runs/40200045548/job/121189107288): radix_comparison target compiles without a -std= option; AppleClang 15 rejects constexpr at radix_comparison.hpp:43 and reports 20 errors. Target does not declare a language-standard requirement or inherit one by linking a feature-bearing target. |
| [Linux x86 GCC 121186452373](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121186452373) | Pre-existing fixture precondition on main; not a full native baseline replay | Build succeeds; model-artifact-review smoke exits 1 because models/tiny-random-llama/model.safetensors is missing. Exact main script's preflight reproduces locally. |
| [Linux ARM64 Clang 121186452387](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121186452387) | Pre-existing fixture precondition on main; not a full native baseline replay | Build succeeds; only reported CTest failure is t81_cli_contract_test_runner (395/396 pass), requiring models/tiny-random-llama-exported.safetensors at cli_contract_test.cpp:490-491. |
| [Linux x86 Clang 121186452399](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121186452399) | Pre-existing fixture precondition on main; not a full native baseline replay | Only reported CTest failure is the same missing-file assertion (396/397 pass). |
| [C++23 ON 121185892646](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121185892646) | Pre-existing fixture precondition on main; not a full native baseline replay | Same sole failed CLI-contract check (396/397 pass). |
| [C++23 OFF 121185892728](https://github.com/t81dev/t81-foundation/actions/runs/40198913954/job/121185892728) | Pre-existing fixture precondition on main; not a full native baseline replay | Same missing exported fixture assertion. Full native C++20 suite was not independently replayed. |

Windows root cause is the unchanged non-cone sparse list excluding /development_tests/** while the unchanged Direct API CMake file unconditionally references its test source. The source exists in the full main tree. Windows images match exactly: windows-2025-vs2026 20261021.278.1; MSVC 19.51.36260.0 and clang-cl 20.1.8.

AVX2 image matches exactly: ubuntu-26.04 20261012.163.1; GNU C++ 15.2.0. Do not generalize this failure to every GCC, sanitizer, platform or determinism test.

macOS image and compiler match exactly: macos-14-arm64 20260831.0302.1, AppleClang 15.0.0.15000309. This experiment target's configuration failure is outside Research Protocol 001 and was not corrected here.

The two model fixture paths do not exist in a fresh main checkout, are untracked, and are excluded by .gitignore:136. Relevant test/script/job definitions are identical. This establishes the same deterministic broken clean-checkout precondition on main; it does not claim all native suite results are reproduced. For the smoke preflight, a temporary /usr/bin/false executable sentinel satisfied the binary-exists guard; the script then exited with the identical missing-source-fixture message before any binary invocation. The initial attempt without this sentinel stopped at the missing-binary guard and is preserved but not counted as equivalent evidence.

No completed failure in this snapshot is attributed to #566. Independent complete-native-suite behavior beyond the preconditions and any still-running jobs remains undetermined. Expected negative-test diagnostics elsewhere in logs were not mistaken for failed CTests; the actual final CTest failure lists were used.

## Protected checks and merge readiness

PR #566 documentation lint genuinely passes its lychee and DeepWiki steps; this does not make the entire policy-audit job trustworthy. The separate [masking report](POLICY_AUDIT_PIPELINE_MASKING.md) explains demonstrated false-green audit steps.

| PR | Exact required check | Observed result |
| --- | --- | --- |
| pr566 | quality gate / required | not yet reported; prerequisites running |
| pr566 | Analyze (cpp) | in_progress / None |
| pr565 | quality gate / required | completed / failure |
| pr565 | Analyze (cpp) | completed / success |

Neither PR is merge-ready. A required check that is absent or in progress has not passed. PR #566 has multiple failed required dependencies, even though its aggregate check is not yet posted. PR #565's existing CodeQL success applies only to its current head, not to #566 or a future refreshed security head. No recommendation to merge either PR is made.

### Recommended integration sequence

1. Keep #566 focused and #565 unchanged. Prepare baseline build/fixture corrections and policy-audit exit propagation in separate, explicitly reviewed work; do not weaken warnings, sanitizer coverage, tests, required checks or policy thresholds. These are outstanding defects, not completed fixes.
2. Validate the combined candidate on an isolated integration checkout. Main's documentation gate currently fails, so baseline repairs cannot be called green merely because jobs were skipped; account explicitly for dependency on #566 when preparing independent corrections. Do not bypass protection to break that integration dependency.
3. Re-evaluate #566 after baseline corrections are integrated or an explicitly approved combined integration plan is validated. Require both actual protected contexts to complete successfully on the exact candidate before a merge recommendation, then obtain explicit user merge authorization. Existing masked policy failures must be addressed before trusting a green gate.
4. Refresh #565 against the resulting main using ordinary, non-force history updates. Record its new merge base/SHA; rerun all required checks and the three registered native Linux authorization/integration tests. Only after genuine success request separate explicit authorization to merge #565.

Thus the intended order is baseline CI repairs / verified controls, then #566, then refreshed #565; the exact integration plan must resolve the documentation-preflight dependency without waiving checks. Never silently merge #566 through another integration PR without approval covering that incorporation.

No merges, closures, auto-merge, branch deletion, force push, production fixes, frozen ISA edits or Research Protocol 001 implementation occurred. Audit branches and old evidence remain preserved.

## Reproduction and evidence commands

```sh
gh workflow run ci.yml --repo t81dev/t81-foundation --ref audit/pr566-main-ci-baseline
gh api repos/t81dev/t81-foundation/actions/runs/40199911123/jobs
gh api repos/t81dev/t81-foundation/actions/runs/40200045548/jobs
gh api repos/t81dev/t81-foundation/actions/jobs/121186452330/logs
gh api repos/t81dev/t81-foundation/actions/jobs/121186452359/logs
gh api repos/t81dev/t81-foundation/actions/jobs/121185892781/logs
gh api repos/t81dev/t81-foundation/actions/jobs/121186452339/logs
gh api repos/t81dev/t81-foundation/actions/jobs/121188699385/logs
gh api repos/t81dev/t81-foundation/actions/jobs/121188699549/logs
gh api repos/t81dev/t81-foundation/actions/jobs/121188699790/logs
gh api repos/t81dev/t81-foundation/actions/jobs/121189107288/logs
git diff --exit-code f6d4d680709c5c6c98b305628e04346f5ef61620 5bfe915531f0629b3dabb15cda6c374fe86945a0 -- CMakeLists.txt cmake src include benchmarks tests development_tests experiments/radix_comparison
git ls-tree -r --name-only f6d4d680709c5c6c98b305628e04346f5ef61620 models
git show f6d4d680709c5c6c98b305628e04346f5ef61620:.github/workflows/ci.yml
```

The two dispatch commands used the two harness revisions listed above respectively; source checkout in each was pinned to exact main, not the harness commit. Workflow commands in the harness are unchanged and versioned for review. Complete logs and JSON snapshots are under evidence/ with a SHA-256 manifest. Log bytes were not whitespace-normalized; .gz files decompress to the hashed original downloaded bytes. Baseline harness manifests document allowed orchestration changes and job-command equality. Native environment and error comparisons assert identical image/compiler metadata and error diagnostics.

Local fixture preflight setup, in the audit worktree with main script/test bytes:

```sh
mkdir -p build
ln -s /usr/bin/false build/t81
bash scripts/ci/run_model_artifact_review_smoke.sh
```

The sentinel is an untracked local test input only; no CLI code executes because the script rejects the missing fixture first. No fixture, model or production source was fabricated. The early no-binary attempt and the later missing-fixture attempt are both preserved with different filenames.

## Final refresh — 2026-10-25 10:15 EDT

The same nine completed failed jobs remain. Required `quality gate / required` is not yet reported; `Analyze (cpp)` remains in progress with null conclusion. Neither has genuinely passed on #566. Still-running jobs in this workflow: build / macos-x86_64 / gcc. The separate general `build / sanitizers` job has now completed with conclusion `cancelled`; this does not substitute for the failed AVX2/ASan lane. Final job/check snapshots are preserved alongside the earlier snapshot. Branch protection is unchanged, both PRs remain draft/open at their original heads, and auto_merge is null for each. No merge recommendation is made.
