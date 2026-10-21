# Experiment report — October 21, 2026

**Recommendation: Revise H1.** Existing T81 components support a bounded identity,
execution, storage, and evidence workflow, but the existing CanonFS text-policy
hook fails the tested authorization gate. Experiment-local integration can close
the observed gaps. This does not establish a shared production object interface,
VM necessity, or an advantage over a conventional implementation.

## Baseline and protocol audit

Fetched `origin/main` and created the experiment branch from exact commit
`d1c24670470e74412ab96182a177a47e7127c68a`. No PR was merged. The pre-existing,
untracked `build-bench/` directory was left untouched and was not used to build
the experiment. Required handoff/project docs, research framework/charter,
Baseline-X specification, RFC-00D4, and the relevant CanonFS contracts were read.

PR state and diffs were reviewed on October 21 using GitHub API snapshots and
locally fetched PR heads:

| PR | Status/head | Relation to main; experimental impact |
| --- | --- | --- |
| [#528](https://github.com/t81dev/t81-foundation/pull/528) | Open; `29b09ec231e2f695e7c94be70569e2546688a45f` | Not merged. Adds real-model validation scripts/reports, opcode assignments/reordering, and extensive VM changes including weak experimental dispatch fallbacks. Its model download uses a mutable HF `main` URL. Do not import its ISA changes or treat its model/intake report as evidence for this experiment. No model inference or experimental dispatch is used here. |
| [#533](https://github.com/t81dev/t81-foundation/pull/533) | Merged; head `cca7f9a11eb5fa6df480500ac2410219f6b8d72a` | Merge `c9d12ba2bcebe22ed3af5f105d7b2001f5ca8e8f` is an ancestor of main. Adds charter, RFC-00D4, and Baseline-X design; those documents are not implementations. |
| [#534](https://github.com/t81dev/t81-foundation/pull/534) | Open; `5f3c8072f94bb3e5fd5eb7ceaf3af6e96e14d004` | Zero changed files; tree diff from its base is empty. Its body says an audit/protocol was completed, but no separate protocol artifact is present in that PR. Its merge base with main is `c9d12ba2...`. The existing framework at main remains the available methodology. |
| [#536](https://github.com/t81dev/t81-foundation/pull/536) | Open; `c3820c2d67040f92213adc1c7ce6915ac3b4abef` | Based on current main; not merged. Proposes BigInt normalization, INT64 division/modulo/negation fixes, float/fraction flags, tier limits, DirectAPI/backend build fixes, and skipping optional CLI fixtures. It leaves ordinary signed add/sub/mul overflow concerns relevant outside bounded inputs. None of these edits is imported; bounded positive integer multiplication avoids those paths/edges. ControlledAIBackend is not used. |

The checked-in framework specifies Python/FastAPI/Postgres/OPA, T81VM, and
Wasmtime/SQLite/Sigstore arms. This precursor deliberately does not relabel its
A/B/C as those completed deployment arms: A uses standard local mechanisms, B
connects existing T81 APIs, C tests minimal integration. Implementing the full
three-stack study would add dependencies and breadth before a basic boundary gate
passes. The original zero unauthorized-action requirement is retained; the
sub-second offline-verification target is measured; cross-platform reproducibility,
10x complexity/latency, and the original A/B/C architectural outcomes remain
**inconclusive**. No threshold is relaxed and no missing deployment arm is scored.

Protocol defects prevent claiming RFC-00D4 conformance:

- Its detached verification protocol requires ALLOW, while Baseline-X requires
  independently checkable DENY bundles. A denial record must be distinguishable
  from an authorization certificate; this experiment checks both without treating
  denial as permission.
- RFC-00D4's signature covers meaning/evidence/authority but not method,
  challengeability, or all identity metadata. This experiment signs the complete
  restricted record instead. It does not silently change the RFC.
- The draft does not settle bundle self-hash scope, a complete canonicalization
  profile, offline trust/key resolution, or authenticated execution ordering.
  Schema validity and hashes do not prove that a side effect was prevented.
- Baseline-X's illustrative predictions about Python nondeterminism, relative
  latency, and dependency LOC are assumptions, not measurements. Exact bounded
  integer arithmetic is equally replayable in the conventional arm.

## What was implemented

A single experiment with a native adapter, conventional baseline, detached
verifier, and CTest-driven adversarial suite. See [README.md](README.md) for exact
build/run/verification commands and the boundary/trust contract.

Existing capabilities: CanonHash81, CanonFS import/read and persistent writes,
TISC LoadImm/Mul/Halt, real PolicyEngine calls before VM dispatch, and a driver
policy callback before object creation. Missing integration: a universal object
execution/receipt API, production detached signing/trust, and a policy phase
contract independent of VM instruction names. The harness supplies common request
and receipt fields only; it is not a new public interface or subsystem.

The protected action is creating a result object. Direct enumeration and reading
of fresh output stores measures whether that action occurred. VM-policy call
counts instrument a delegating PolicyEngine, not a mock. Successful arithmetic
produces 25000; denied output operations may still calculate that proposal.

Two concrete implementation mismatches were observed:

1. `make_axion_policy_hook` returns an allowing callback when parsing fails,
   because the captured engine is absent.
2. Its context uses Nop; PolicyEngine checks required-event conditions only when
   `next_opcode == Halt`. An unmet required authorization event is therefore not
   enforced by that hook at the tested storage boundary.

C validates policy and evaluates it at the commit callback using the existing
Halt final-evaluation marker, tier 1 context, and explicit Allow-only handling.
No VM instruction executes in that callback. This context convention is a narrow
adapter, not a production repair or a new policy primitive. Both interpreted and
compiled policy paths use the final-instruction convention; compiling alone does
not fix the mismatch. The existing runtime is left intact to preserve a measurable
baseline and keep a future production fix separately reviewable.

Added implementation cost is reported as physical source lines (including comments
and blanks) in results.json, not estimated transitive dependency LOC. A/B/C share
the signing/orchestration/verifier code; B/C additionally use the native adapter
and existing T81 libraries. The native executable size is recorded, but is not a
like-for-like storage comparison with an installed Python interpreter. No claim of
10x lower complexity or dependency footprint follows from these counts.

## Measurement protocol

All six scenarios run in A, B, and C for 30 fresh-store repetitions each (540
requests). Full records must match across repetitions. All receipts use the same
signature format and detached verifier. Changed bytes retain the original expected
hash; the wrong-hash case supplies an all-zero digest. Revocation replaces the
active policy synchronously after calculation and before the commit callback.

`results.json` records every timing sample, medians, ranges, sample standard
deviations, verdicts, object counts, hashes, runtime versions, and source/binary
fingerprints. `observed/` includes the first signed receipt and caller input/policy
for every arm/scenario plus the run's public key; run.py retains all 540 receipts
in its output directory. Private signing material is deleted.

Timing uses Python `perf_counter_ns`. Runner timings include a C++ subprocess,
VM initialization, and CanonFS in B/C, versus in-process Python in A. Signature
creation is outside runner timings in every arm; verification includes Python and
OpenSSL process startup. These are descriptive end-to-end observations, not fair
compute microbenchmarks or evidence of architectural performance superiority.
There is no randomization or controlled CPU isolation; no confidence interval or
causal performance claim is made. Peak memory is not compared: host Python and
child VM/process memory have different accounting boundaries here.

Environment: macOS 26.5.2 (25F84), ARM64, Apple Clang 21.0.0
(`clang-2100.1.1.101`), CMake 4.3.0, Ninja 1.13.2, Python 3.14.3,
OpenSSL 3.6.2. Asio is the existing test dependency pinned to `asio-1-30-2`.
Build flags observed: `-DT81_DETERMINISTIC -O3 -DNDEBUG -std=gnu++23 -fPIE
-Wall -Wextra -Wpedantic -O3 -DNDEBUG`. No ISA changes, model downloads, network
runtime, stochastic execution, or new package dependencies are involved.

Input hex: `323520313030300a`.
CanonHash81 bytes in hex:
`24c7ef023a5b9b1ee91fec7fd453b7646889f71213013a8022a4cbe9facb9617`.
Output hex: `32353030300a`.
Output CanonHash81 bytes:
`f240cb585558f845852731ce9c30b3bb686bc7712fee6a8e7b6934351fcb2b25`.

## Observed comparison

| Scenario / gate | A conventional | B existing path | C integration |
| --- | --- | --- | --- |
| authorized | PASS (1 result objects) | PASS (1 result objects) | PASS (1 result objects) |
| changed_bytes | PASS (0 result objects) | PASS (0 result objects) | PASS (0 result objects) |
| wrong_hash | PASS (0 result objects) | PASS (0 result objects) | PASS (0 result objects) |
| denied | PASS (0 result objects) | FAIL (1 result objects) | PASS (0 result objects) |
| revoked | PASS (0 result objects) | FAIL (1 result objects) | PASS (0 result objects) |
| invalid_policy | PASS (0 result objects) | FAIL (1 result objects) | PASS (0 result objects) |
| Same record across 30 repetitions | PASS | PASS | PASS |
| Heterogeneous object interface / cross-platform replay | INCONCLUSIVE | INCONCLUSIVE | INCONCLUSIVE |
| Comparative runtime/memory advantage | INCONCLUSIVE | INCONCLUSIVE | INCONCLUSIVE |

Within this designed negative-case set, unauthorized result creation occurred in
**0/150 A requests, 90/150 B requests, and 0/150 C requests**. The B failures
repeated 30/30 times in each of denied, revoked, and invalid-policy cases.
Reproducibility includes reproducible wrong authorization behavior in B.

| Authorized case, 30 repetitions | Runner median [min, max], ms | Runner sample SD, ms | Detached verifier median / max, ms |
| --- | --- | --- | --- |
| A | 0.267 [0.226, 0.391] | 0.032 | 90.294 / 119.023 |
| B | 6.614 [6.352, 7.599] | 0.310 | 89.341 / 97.257 |
| C | 6.765 [6.366, 7.659] | 0.339 | 90.302 / 102.679 |

All detached verifier invocations, including rejections, finished within 125.144 ms
in this run (the local <1s target passed). Python startup is included.
The runner timing columns are **not comparable performance rankings**.

Implementation physical LOC: {'runner.cpp': 130, 'run.py': 154, 'verify.py': 110}; native runner: 1446712 bytes.
These counts exclude existing T81/stdlib/OpenSSL implementations and do not
establish a dependency or engineering-cost advantage.

Validation: the five targeted CTests passed from an isolated export of main plus
this patch, built in a fresh directory with no `build-bench/` inputs. The pinned
Asio checkout downloaded during initial configuration was reused as an offline
FetchContent source. The four existing tests cover CanonFS identity, persistent
read tampering, Axion trace integration, and VM predispatch denial. The new test
executes 36 real requests plus detached-verifier attacks, including re-signed
incorrect results, missing fields/objects, changed bytes/addresses, wrong key,
wrong digests, and unsigned mutation. C++ formatting and diff whitespace checks
passed. Default builds keep this research suite disabled.

The existing root-structure governance script also fails on the current baseline:
it expects missing root package.json/package-lock.json and rejects already tracked
paths such as src, PROJECT_STRUCTURE.md, and build.log. Those root paths and the
checker are unchanged by this patch; the failure is recorded rather than claiming
all repository checks pass. This is separate from the five passing targeted tests.
Existing macOS linking emits duplicate libt81_core.a warnings.

## CI audit and separate repair proposal

[October 21 Inference Benchmark run](https://github.com/t81dev/t81-foundation/actions/runs/39607952433),
job `119248960565`, completed all five benchmark families, report generation, and
artifact upload. The final push failed with GH006, "2 of 2 required status checks
are expected." This is a publication infrastructure failure, not failed benchmark
calculation. The October 14 inference run also appears among recent failures.

The latest failing Documentation run at the reviewed main history is
[38468449162](https://github.com/t81dev/t81-foundation/actions/runs/38468449162).
Job `115541065955` failed in setup-node's npm cache discovery: no root dependency
lock file was found. PDF/sidebar/TOC jobs succeeded. This is separate from runtime
correctness; no global green-CI claim is made.

Propose a separate minimal inference workflow repair: retain benchmark steps,
change contents permission to read, remove the final git commit/push step, and
include `benchmarks/results/inference_comparison.md` alongside the existing
`bench-*.json` uploaded artifact. Adjust generated wording from automatic commits
to artifact publication. Alternatively publish through a bot branch and PR.
Keep branch protection intact. For Documentation, fix setup-node's cache path to
the actual checked-in lockfile, or disable npm caching when none exists. Neither
workflow is changed in this experiment PR, and failed calculations are not masked.

## Facts, assumptions, and remaining uncertainty

Facts are restricted to observed result files, verifier outcomes, and the exact
code paths described above. Trust in the harness, signer, executable, and local
filesystem is an assumption. Signed self-reporting is not remote attestation or
proof that no unreported action happened. No unit test proves system-wide security.

No actual inventory agent, external order service, OPA, Postgres, Wasmtime,
concurrent revocation, replay across CPU architectures, delegated authority,
hardware isolation, persistent input provenance graph, or RFC-00D4 interoperability
was evaluated. The output/evidence interface covers one transformation and does
not establish heterogeneous-object generality. A future producer could lie while
signing internally consistent evidence; this verifier cannot solve that problem.

**Next experiment:** repair the existing CanonFS hook's malformed-policy handling
and define how its required-event checks apply at a storage commit. Add matching
production negative tests, rerun this unchanged zero-write gate with the repaired
path as a separately labeled arm, then test concurrent revocation against the same
side-effect service in a conventional and T81 host. Only after that passes should
the full inventory/OPA/Wasmtime protocol and cross-architecture study proceed.
