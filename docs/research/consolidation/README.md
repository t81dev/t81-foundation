# Research evidence ledger

**Decision proposed for review: prioritize A, the governed deterministic runtime;
use C, independently reusable components, as its integration discipline. Freeze B,
the universal computational-object direction, pending a concrete unmet consumer
need.** The strongest result is an observed authorization defect followed by a
small production correction. The descriptor experiment did not demonstrate an
integration advantage sufficient to justify its additional contract.

Read this ledger, the [architecture decision](ARCHITECTURE_DECISION.md), and the
[maintenance assessment](MAINTENANCE.md). They consolidate the five PRs; historical
reports and negative tests remain authoritative for their exact boundaries.
These are evidence-based recommendations, not amendments to public APIs or RFCs.

## Repository and CI snapshot

Audit date: October 21, 2026. Exact observed time, PR state, open PR heads, CI job
URLs and evidence hashes are in [repository-snapshot.json](repository-snapshot.json).
The initial API read found #541 open/draft at `b4860a8a...` main. During the audit,
#541 was merged externally. The pinned consolidation baseline is therefore
**`6977f49b3a2eba54d9776361a6026971ee529bfd`**, containing all five PRs.
This consolidation did not merge #541 or any other PR. Its code is now merged
**experiment-local code**, not a promoted production interface.

| PR | Research head SHA | Merge SHA on main |
| --- | --- | --- |
| [#537](https://github.com/t81dev/t81-foundation/pull/537) | `d8ad35254a1e35ca7374a4f57f7a870d568521fa` | `634e0dc69a69cb148f81159ac8c7fc3225582149` |
| [#538](https://github.com/t81dev/t81-foundation/pull/538) | `504c1c5728bf903b729cc2767a29a1cb737d41a3` | `dec74cfa4d7be321e0c28adb276a8dc2d278c502` |
| [#539](https://github.com/t81dev/t81-foundation/pull/539) | `e04b95f12798be7d85d5b6b1d3645a5c677c2a4e` | `8a6dce4831bd84e23d499fa71900e959ba940875` |
| [#540](https://github.com/t81dev/t81-foundation/pull/540) | `448edf49be6116c08fa5ec20641c447f53b934f0` | `b4860a8a3065a679c2f5f12896cb1990d1633470` |
| [#541](https://github.com/t81dev/t81-foundation/pull/541) | `1a110b07792770e3e720c449c558830f4313731a` | `6977f49b3a2eba54d9776361a6026971ee529bfd` |

At the recorded snapshot: #537's head had successful format and failed AI CI;
#538–#541 heads each had both successful checks. The #537 failure was stale status
documentation before compilation, documented in #538, not its H1 test outcome.
#538's merge had a Documentation search-index failure caused by missing npm-cache
lockfiles; #539 removed that cache setting. These old failures remain evidence,
not reasons to mutate historical runs or widen this cleanup.

At the final baseline refresh, all seven reported checks succeeded (AI, format,
build, build-status, EFI boot, deploy and badge refresh). The snapshot records
the refresh time separately from the initial PR/open-list capture. CI state is
time-bound; see the saved job URLs before treating it as current. An empty legacy status list is not a CI pass.
The remote workflows do not enable the four dedicated research flags; green AI
CI does **not** establish that the research suites ran.

Open unmerged work at capture: #503, #504, #507, #508, #513, #514, #516, #523,
#524, #525, #526, #527, #528, #534, #536. Their head SHAs are saved in the
snapshot. In particular #526's OS-first direction, #528's real-model claims,
#534's protocol claims, and #536's arithmetic/runtime fixes are not evidence of
merged production behavior in this baseline. No open PR is silently imported.

## Hypothesis ledger

All five recorded experiments used macOS 26.5.2 ARM64, Apple Clang 21, Release
builds and Python 3.14.3; #537/#538 also used OpenSSL 3.6.2/Ed25519. Their results
are bounded trusted-host observations. Remote CI is separately described above;
it does not supply cross-platform experimental replay. Counts below come from
recorded research, not newly repeated 30-repetition measurements.

| Hypothesis and original claim | Tested boundary / artifacts and reproduction | Observed result | Remaining limits / production distinction | Decision |
| --- | --- | --- | --- | --- |
| **H1:** identity and policy prevent unauthorized result creation before the side effect | #537 six scenarios × three arms × 30 fresh stores; #538 four-arm repeat. [Original report](../../../examples/governance/accountable-computation/REPORT.md), [hardening report](../../../examples/governance/accountable-computation/PRODUCTION_HARDENING.md), `results.json`, `hardening-results.json`, signed receipts. Reproduce R1/R2 below. | Historical B created 90 unauthorized objects / 150 negative requests; C experiment integration and conventional A created 0/150. Corrected production D created 0/150 and 30/30 correct authorized outputs. | Production `final_authorization`, fail-closed parse/evaluation and current-attempt evidence are real #538 changes. C's synthetic Halt was never the production repair. Protected effect is output object creation, not all computation. Bootstrap and trusted callback removal remain; no malicious-host or whole-system guarantee. | **Continue**, narrowed to configured, externally serialized production boundaries; reject the historical broad reading. |
| **H1-concurrency:** completed revocation prevents stale later mutation and future admission | #539 matched file/map/RAM-block endpoints, two ownership models, controlled thread handshakes: 64 cases × 30 = 1,920 initial operations. [Report](../../../examples/governance/accountable-computation/CONCURRENT_REVOCATION.md), `concurrent-results.json`, `concurrent-summary.json`. R3. | 240 snapshot counterexamples; zero prohibited commits in 960 shared-owner operations; post-revocation new requests deny. Conventional and T81 outcomes match under equal ownership. | The mutex/live-authority flag belong to the experiment's trusted owner. Production raw drivers still require external serialization. Draining is not preemption; no multi-process authority, crash durability, fairness or reentrancy result. Global trace state also needs shared ownership. | **Freeze** the tested draining-owner reference; no production concurrency expansion without a selected caller contract. |
| **Common computational-object interface:** existing APIs improve integration across representations | #540 TISC execution and SafeTensors intake/inspection; 36 T81/conventional cases. [Report](../../../examples/research/object-interface/REPORT.md), `results.json`. R4. | Shared byte identity, integrity, storage and storage authorization demonstrated; two provider branches remain. No common operation-discovery or useful-invocation contract advantage demonstrated. | Production storage tags do not validate formats. Caller adapters supply operation checks and records. Model inspection is not inference or program execution. Original conventional arithmetic uses Python source; no engine/performance equivalence follows. | **Revise** to shared governed byte storage; freeze universal-interface implementation. |
| **H2:** a validated descriptor reduces caller checks, representation confusion and unsupported invocation | #541 same two canonical payloads; 96 descriptor cases plus 36 baseline cases. [Report](../../../examples/research/validated-descriptors/REPORT.md), [encoding](../../../examples/research/validated-descriptors/ENCODING.md), `results.json`. R5. | Caller dispatch decisions 1→0, adapters 2→2, existing validators removed 0; 450 new executable research lines. Each descriptor arm rejects 44 cases with zero attempted provider invocations. Conventional checks match. Recorded conclusion: Unsupported. | Exact-fixture admission, compiled operation allowlist and independently pinned hashes are experiment-local. Those checks cause early rejection; the descriptor alone does not. No measured multi-caller duplication benefit. This does not falsify every possible descriptor design. Baseline counts completed operations, not attempted wrong-loader calls. | **Reject** adoption of this descriptor; retain the negative experiment frozen. |
| **Provenance / independent verification:** evidence permits detached checking of identity, authority and result | #537/#538 receipts plus detached verifier attacks; #539 detached order/state checker; #540 false/incomplete stored records. [Receipt verifier](../../../examples/governance/accountable-computation/verify.py), [schedule checker](../../../examples/governance/accountable-computation/verify_concurrent.py), R2/R3/R4. | Signatures, caller-pinned inputs, bounded arithmetic replay, hashes, declared order and observed artifacts are checked; re-signed false results reject. Storage accepts integrity-valid false/incomplete provenance; semantic checker rejects it. | Production CanonFS supplies immutable byte identity, not truth. Signing/key trust, bounded replay and receipt semantics are harness contracts. Schedule/RAM statements trust the producer; independent filesystem observation is stronger only within the recorded host boundary. No remote attestation, proof of unreported-write absence, RFC-00D4 interoperability or universal verifier. | **Continue** bounded detached checking; **Revise** any claim that hashes/signatures establish causality or truth. |

## Reproduction map

The [single regression command](../../../scripts/ci/run_research_regressions.py)
configures all four opt-in flags, builds eight named targets and runs eight
existing CTests with their existing attack cases. It does not rerun historical
measurements or silently execute old B/C using repaired code.

```sh
python3 scripts/ci/run_research_regressions.py --build-dir /tmp/t81-research-build
```

Requirements: CMake, Ninja, C++23 toolchain, Python >=3.10, OpenSSL >=3 with
Ed25519, and the existing Asio test dependency. For offline configure append
`--asio-source /path/to/asio-1-30-2`. Use a separate build directory. The script
uses argv-based subprocesses and CTest target discovery; no shell expansion,
POSIX-only path convention, producer/verifier merge or production change.
Windows needs the compiler developer environment; CMake handles `.exe` paths.
Do not use Python `-O`: the original #540 verifier uses assertions.

The following commands assume that build directory and its binaries. On Windows,
append `.exe` to direct binary paths. All measurement output directories must be
new. Exact historical build instructions remain in the linked reports.

**R1 — original H1, preserving its failure:**

```sh
git worktree add --detach /tmp/t81-h1-original d8ad35254a1e35ca7374a4f57f7a870d568521fa
```

Use the original checkout's README configure/build instructions, then run its
`accountable_computation_experiment` CTest. Its green outcome detects the known
B failure; it does not mean H1 passed. Do not use today's binary as original B/C.

**R2 — corrected H1 and detached evidence:**

```sh
python3 examples/governance/accountable-computation/run.py \
  --runner /tmp/t81-research-build/t81_accountable_experiment \
  --baseline-runner /tmp/t81-h1-original-build/t81_accountable_experiment \
  --production --repetitions 30 --output /tmp/t81-h1-measured
python3 examples/governance/accountable-computation/verify.py \
  examples/governance/accountable-computation/hardening-observed/D-authorized.json \
  --public-key examples/governance/accountable-computation/hardening-observed/public.pem \
  --input examples/governance/accountable-computation/hardening-observed/input \
  --policy examples/governance/accountable-computation/hardening-observed/allow.apl
```

**R3 — concurrent ordering:**

```sh
python3 examples/governance/accountable-computation/measure_concurrent.py \
  --runner /tmp/t81-research-build/t81_concurrent_revocation \
  --repetitions 30 --output /tmp/t81-owner-measured
python3 examples/governance/accountable-computation/verify_concurrent.py \
  /tmp/t81-owner-measured/results.json --artifacts /tmp/t81-owner-measured
```

**R4 — representation comparison:**

```sh
python3 examples/research/object-interface/compare.py \
  --runner /tmp/t81-research-build/t81_object_comparison --out /tmp/t81-object-measured
```

**R5 — descriptor falsification:**

```sh
python3 examples/research/validated-descriptors/compare.py \
  --runner /tmp/t81-research-build/t81_descriptor_comparison \
  --baseline-runner /tmp/t81-research-build/t81_object_comparison \
  --out /tmp/t81-descriptor-measured
```

To reproduce the exact recorded #539/#540/#541 source versions rather than current
regressions, use detached worktrees at their full research-head SHAs above and the
corresponding reports. Retain each new run separately; never overwrite checked-in
historical results or compare startup-heavy timings as architectural rankings.
