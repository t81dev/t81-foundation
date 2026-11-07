# Architecture decision: governed runtime before object generalization

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Architecture decision: governed runtime before object generalization](#architecture-decision-governed-runtime-before-object-generalization)
  - [Decision and source evidence](#decision-and-source-evidence)
  - [Comparison](#comparison)
  - [Ambitions not currently justified](#ambitions-not-currently-justified)
  - [Prioritized 30-day objective](#prioritized-30-day-objective)

<!-- T81-TOC:END -->


Status: **Proposed for maintainer review**. Date: October 21, 2026.
Baseline: `6977f49b3a2eba54d9776361a6026971ee529bfd`.
Evidence: [ledger](README.md), [exact snapshot](repository-snapshot.json).
This record proposes work allocation; it does not change API or RFC authority.

## Decision and source evidence

Rank **A first, C second, B third**. Invest in the governed deterministic runtime.
Use components through their existing explicit interfaces and integrate only
where the benefit is demonstrated. Do not split the repository or create a new
component framework to implement C. Freeze implementation work on a universal
object interface and reject adopting #541's private descriptor.

This ranking comes from executable behavior and source inspection:

- #537 observed unauthorized persistent results; #538 corrected
  [the production callback](../../../kernel/axion/canonfs_hook.cpp) and
  [final authorization](../../../include/t81/axion/context.hpp). Production
  [negative tests](../../../tests/cpp/canonfs_policy_boundary_test.cpp) inspect
  filesystem changes and device writes rather than only return codes. The same
  measurement gate moved from B's 90/150 failures to D's 0/150 failures without
  another architectural layer. That is the strongest causal engineering result.
- #539 used the actual drivers and a matched conventional comparator. Shared
  ownership orders writes/revocation; both stacks need it under the stronger
  contract. This supports explicit owner responsibility, not T81-specific locking
  superiority or introducing a transaction layer. The [Driver contract](../../../include/t81/canonfs/canon_driver.hpp)
  continues to require external serialization.
- #540 actually executes a TISC program and loads a SafeTensors artifact. Shared
  identity/storage work, but their operations and provider paths stay distinct.
  Model inspection does not establish interchangeable execution.
- #541 validates only two exact fixtures. Its catalog removes one caller dispatch
  branch while retaining two adapters and all provider validators. The standard
  library control achieves the same rejection/discovery predicates. This is
  evidence against adopting **that descriptor**, with insufficient coverage to
  prove every future abstraction pointless.
- Source audit confirms a live gap: [block-backed storage](../../../fs/block_backed_driver.cpp)
  calls the hook for read/write, while capability publish/revoke and parity repair
  do not; reads/writes do not consult its capability map. Persistent/in-memory
  behavior is different. #539 already records this correction to #538's broader
  wording. Do not advertise uniform driver enforcement from the common C++ type.

## Comparison

Ranks in parentheses are review judgments, with **1 best** for that criterion;
they are not measured scores and are not summed. The experiments supply no
external adoption study or fair architectural performance ranking.

| Criterion | A: governed runtime | B: universal object substrate | C: reusable capabilities, selective integration |
| --- | --- | --- | --- |
| Existing implementation evidence | (1) Actual VM/CanonFS/Axion path, repaired production boundary and retained regression tests | (3) Common byte identity works; operation-interface advantage not demonstrated | (2) Existing separate APIs are real; independent packaging/consumer success not measured |
| Demonstrated correctness | (1) Correct bounded production D plus explicit ownership reference; limits testable | (3) Local descriptor rejection works but adds no established general correctness guarantee | (2) Retains component checks, but separate deployment boundaries were not tested |
| Original research potential | (2) Falsifiable policy/side-effect and evidence boundaries; useful next measurements | (1) Broad open question, but theoretical potential alone cannot fund implementation | (3) Smaller integration questions; reduced conceptual novelty |
| Engineering complexity | (2) Harden existing paths and explicit contracts; known synchronization/evidence gaps | (3) Additional representation, provider, trust and operation contracts without a demonstrated consumer | (1) Reuse explicit APIs where sufficient; this rank excludes a speculative repository/package split |
| Maintenance burden | (1) Concentrate on existing regression surfaces and one runtime story | (3) More adapters/catalogs and contract versions; #541 removes no provider validation | (2) Avoid universal glue, but multiple component contracts/packaging could duplicate integration work |
| External adoption feasibility | (2) Concrete CLI and reproducible bounded example; actual adoption unmeasured | (3) Unproven abstraction plus substantial prerequisites; adoption unmeasured | (1) Smaller entry points may lower commitment; this is an inference, not consumer evidence |

A wins the near-term allocation because the highest-confidence result was an
existing-path repair with observable before/after effects. C constrains that work:
keep identities, storage permissions, representation validation and operation
authorization distinct. Its potential adoption advantage does not justify a
premature component split. B's conceptual novelty does not outweigh its negative
integration evidence and added trust/maintenance burden.

## Ambitions not currently justified

The evidence does not justify a universal execution API, provider/plugin registry,
logical identity across representations, interchangeable model/program execution,
new VM opcodes, or generalized object capability declarations. Nor does it justify
OS-first product claims, a full network stack, a new transaction/epoch service,
preemptive or multi-process revocation, automatic crash-safe publication, generic
semantic provenance verification, remote attestation, or whole-system security.
Hash identity and signatures cannot supply those guarantees by themselves.

No measured throughput, memory, complexity-multiple or cross-platform determinism
advantage over conventional deployments follows. The standard-library controls
are bounded comparators, not full OPA/Postgres/Wasmtime deployment studies. Those
larger protocol arms remain unperformed. The current fixture-based H2 result is
too narrow to justify either universal adoption or universal impossibility.

## Prioritized 30-day objective

**One objective:** make the existing configured, externally serialized result-write
boundary reproducible and independently checkable on Linux x86_64, Windows x86_64
and macOS ARM64, without adding an interface or changing the tested authorization
semantics. Extend the evidence coverage of H1 rather than propose H3.

| Period | Priority / concrete output |
| --- | --- |
| Days 1–7 | Pin the existing eight-test slice, toolchain/dependency versions and boundary map. Confirm owner scope includes the global trace and distinguish block-backed gaps. Retain all current negative cases and historical controls. |
| Days 8–14 | Run the same tests and existing A/D fresh-store measurements on the three platforms. Save per-platform source/binary fingerprints, CTest logs, receipts and actual persistent payloads outside historical evidence paths. No mandatory model downloads. |
| Days 15–21 | Verify receipts with caller-pinned public keys, inputs and policies on a separate process/host; rerun existing signature, false-result, completeness and filesystem-tamper attacks. Compare content/result predicates rather than nondeterministic timing/signature values. |
| Days 22–30 | Have a contributor unfamiliar with the five PRs reproduce the slice from the ledger. Publish a small claim/coverage summary with explicit failures and decide which narrow production gap, if any, merits the next fix. |

Success criteria:

- All eight named CTests actually register and pass on the three named platforms;
  each platform repeats the existing six A/D scenarios 30 times in fresh stores.
- D has zero prohibited result objects across 150 negative requests per platform
  and 30/30 correct authorized outputs. Detached verification accepts valid records
  and rejects the existing tampering/re-signed-invalid-record cases.
- The existing ownership test retains zero prohibited shared-owner commits and
  retains snapshot counterexamples; green tests must not hide known weaker-model
  behavior. No claim of native concurrent revocation is added.
- The contributor reaches the existing regression command and correctly identifies
  the H1 repair, the descriptor rejection and the remaining trust boundary within
  a 30-minute walkthrough. Record actual completion/time; it is currently unmeasured.
- Results preserve original evidence, identify exact commits/environments and keep
  startup-heavy timing samples out of superiority claims.

Stopping criteria:

- Any unauthorized D object, accepted false evidence or missing negative case stops
  expansion; isolate a concrete defect before adding more scope. Do not relax a
  predicate or erase the historical failure to obtain a pass.
- If a platform cannot execute by day 14, record the concrete blocker and prioritize
  the smallest portability repair; do not claim portable evidence until it runs.
- A requirement for preemption, untrusted callbacks, multiple independent processes
  or crash durability stops that subtask pending a separately specified boundary.
  Do not smuggle it into the draining-owner result.
- At day 30, conclude with the attained platform/claim matrix even if incomplete.
  Freeze further object-generalization work unless a named consumer exposes a
  concrete unsupported operation and a measurable improvement target that explicit
  existing APIs cannot reasonably meet.

Revisit this decision only with new evidence: an actual reusable consumer, reduced
validation duplication across independent callers, or a specified boundary the
existing mechanisms fail. Documentation ambition alone is not a reopening trigger.
