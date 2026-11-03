# Measurement-readiness dependencies and acceptance gates

**PROPOSED — NOT APPROVED.** Snapshot and source index: [REPORT.md](REPORT.md).
Classifications concern the inspected BASE-01 call paths, not every repository
subsystem. VERIFIED means the explicitly scoped property was checked in this
audit; IMPLEMENTED BUT UNVERIFIED means source exists without the relevant new
execution evidence. INCOMPLETE means only part of the B1 requirement exists;
ABSENT means no implementation found in the inspected paths. NOT INSPECTED is
reserved for items outside this bounded review. Historical logs are not new runs.

## Implementation assessment

All source links below pin `a98b22fb65c80f4225151cbcd5c6934ae076cbe1`.

| Dependency | Classification | Evidence and practical limit |
| --- | --- | --- |
| Timed-runner qualification | INCOMPLETE | [measurement.hpp:8–21](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_lane_a/measurement.hpp#L8-L21) contains an unused interval template. No activated timed dispatcher, wrapper qualification, Linux clock mapping, raw timer/pilot evidence or approved lifetime plan was established. A steady-clock static assertion is not qualification. |
| Pre-execution authorization | INCOMPLETE | [lane_a.py:280–298](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_lane_a/lane_a.py#L280-L298), [harness.py:237–245](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_b1_foundation/harness.py#L237-L245) and native gates deny measurement unconditionally; no constructible allow decision. This protects the freeze but cannot validate future authority. |
| Signed permit validation | ABSENT | [AUTHORIZATION_INTERFACE.md](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_b1_foundation/AUTHORIZATION_INTERFACE.md) proposes the interface. No B1 reader, signature verifier, trust store, revocation freshness, nonce/trial consumption or signed-byte parser found. Unrelated signing mechanisms were not audited or assumed reusable. |
| Live resource budget enforcement | ABSENT | [harness.py:174–187](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_b1_foundation/harness.py#L174-L187) checks output size after `child.wait`; logical capacity checks do not enforce live RSS/disk/time. [collector.py:125–206](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_resource_accounting/collector.py#L125-L206) observes fixed fixtures; no campaign controller or OS resource-limit setup. |
| Watchdog termination | ABSENT | Foundation waits without timeout; collector waits on exact PID without watchdog. Fixture exit7/SIGTERM tests are expected termination cases, not deadline-enforced killing. No B1 watchdog/signal escalation/reap path found. |
| Artifact identity verification | IMPLEMENTED BUT UNVERIFIED | [harness.py:48–65,90–92,163–173,198–227](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_b1_foundation/harness.py#L48-L227) checks source blobs, preflight compiled identity, inventory hashes and source recheck. The mocked stale-identity rejection passed here; no native executable/link/archive qualification ran. Compiled source identity is not full executable/library identity or TOCTOU protection. |
| Measurement denial before side effects | VERIFIED, narrow Python scope | `BoundaryTests.test_python_rejects_all_measurement_requests_before_work` passed with validate/source/Popen/run mocked and asserted uncalled, including environment overrides. Native gates [runner.cpp:45–48](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_lane_a/runner.cpp#L45-L48), [native.cpp:201–225](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/experiments/base01_b1_foundation/native.cpp#L201-L225) were inspected but not executed here: IMPLEMENTED BUT UNVERIFIED for native scope. Parser stderr is an intentional denial output. This is rejection of current entry points, not proof of future permit denial ordering. |
| Raw evidence preservation | INCOMPLETE for B1 | Foundation preserves raw success outputs, lifecycle, stderr, source/build records and SHA-256 index; partial files survive failures, but no complete failure journal/sealing transaction exists for arbitrary early exceptions or interrupted campaign workers. No B1 append-only campaign recorder/live quota path. Existing [integration evidence](https://github.com/t81dev/t81-foundation/tree/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/docs/research/base01/evidence) supplies historical files, not a future retention guarantee. |
| Physical host qualification | ABSENT | [physical runbook:1–12](https://github.com/t81dev/t81-foundation/blob/a98b22fb65c80f4225151cbcd5c6934ae076cbe1/docs/runbooks/BASE01_PHYSICAL_LINUX_CORRECTNESS.md#L1-L12) explicitly gives unexecuted correctness-only instructions. Proposed host requirements are not a passing host/boot report. QEMU Linux collector evidence is not physical-host qualification. No suitable host report found in the inspected packet. |
| Worker accounting integration (supporting dependency) | INCOMPLETE | Exact-PID fixed-fixture collector exists in #692; real Linux/macOS receipts are retained but not rerun here. Foundation records no real accounting samples and has no connection to that collector. Correct accounting and live enforcement are different properties. |
| R75 denominator independent acceptance (supporting dependency) | INCOMPLETE | #690 has source ledger, exact-builder guards and author-run correctness evidence. Foundation keeps PROVISIONAL status. This audit did not independently execute or fully rederive the VM/JIT ledger; it cannot promote it to independent qualification. Do not convert dispatch ticks to hardware instructions. |

## Required acceptance tests before future qualification

These are requirements, not newly implemented or executed tests. Correctness and
synthetic fault-injection tests can precede measurements; tests involving real
timer/noise/pilot observations need their own bounded qualification-only permit.

1. **Contract admission:** reject the original 100-live 1 MiB plan before launch;
   calculate overflow-safe counts/capacity/container bounds, include next-result
   admission, detect duplicate copies and unsupported ABI. Do not substitute
   RSS for output capacity. Reject 200 writes on one store (1/200) and two-store
   restarts under the fixed-lifetime profile even though their ratio is 1%.
2. **Selected lifetime profile:** if B is approved, a deterministic event ledger
   must show exactly 10 intervals * 10 reads, 100 verified outputs, all ten live
   through each interval end, no prior chunk payload surviving, and destruction
   only after full verification. Inject failure in every chunk position, wrong
   bytes/hash, allocation failure and oversized capacity; retain the partial
   record and stop. Inspect separately compiled optimized code to confirm results
   escape; tests mirroring loop constants alone do not establish retention.
3. **Actual API and store behavior:** preserve payload hashes, sizes/fill, enabled
   CanonHash verification, bootstrap policy profile and public allocating copy
   path. Independent specification-compatible CanonHash/byte oracle for every
   output. Ledger initial absence and only first write insertion without adding
   existence checks to timed calls. Check source-derived store/parity/transient
   bounds and absence of persistent driver/I/O workload substitution.
4. **Arithmetic and aggregation:** labeled synthetic intervals/counts check units,
   sum-before-normalize, ten boundaries for B, 30 trial-level summaries, unchanged
   CV/IQR/drift rules and invalid/nonpositive data rejection. No measured values
   should be invented to validate formulas. Preserve every failure/outlier.
5. **Permit/security boundary:** malformed/duplicate/noncanonical bytes, wrong
   signature/key, caller-supplied trust root, unknown action, expiry/revocation,
   unavailable/stale revocation, source/runner/binary/input/protocol/host/boot/
   budget/output mismatch, replay, concurrent nonce reservation and duplicate
   trial IDs must deny. Test both launcher and worker, tracing that no timer,
   warm-up, worker launch or overwrite occurs before authorization. A bounded
   denial record is permitted. A qualification permit cannot launch a campaign.
6. **Resource/watchdog failures:** reviewed fault fixtures exercise memory/output/
   evidence/time admission, live disk exhaustion, descendant prohibition, stuck
   worker, watchdog escalation, reaping, signal/exit identity and interrupted
   sealing. Distinguish sampled process RSS from OS/cgroup scope and known
   detection latency. A post-exit check cannot be accepted as live enforcement.
7. **Identity and evidence:** wrong source bytes, stale builds, swapped binary or
   archive, inconsistent link commands, missing samples, output reuse, corrupted
   index and crash at each recording phase must fail closed with partial raw
   evidence retained. Test actual native paths, not just mocked Python calls.
8. **Host and timer, separately permitted:** actual physical Linux build/boot,
   effective affinity/isolation/control readback, timer backend/resolution,
   wrapper distribution, bounded pilots and all unchanged overhead/stability/
   thermal rules. Any unsupported control blocks the initial profile; no relaxed
   threshold or quiet sample selection. Independently review R75 scope and observer
   correctness on this build before any affected cell can qualify.

## Dependency-ordered checklist

- [ ] Human decision on intended lifetime, insertion scope, B1 approval status
  and accounting definition; record an approved protocol digest without rewriting
  original records. An unresolved contradiction blocks the affected cell and
  therefore the unchanged complete campaign.
- [ ] Separate implementation authorization for the chosen bounded profile;
  complete reviewed correctness, allocator retention, independent oracle, VM
  denominator, observer and artifact-identity engineering. Preserve all failure
  evidence and platform limits (macOS ARM64 evidence is not Linux qualification;
  Windows unsupported; no general ARM64/Linux campaign claim).
- [ ] Human trust-root/permit/revocation decisions, reviewed launcher/worker
  enforcement, nonce reservation, resource controller, watchdog and raw-evidence
  recorder. Pass negative tests without enabling ordinary CI measurements.
- [ ] Assign physical Linux x86-64 host/operator and correctness-only scope;
  execute the existing runbook only under that assignment, preserve restricted
  raw inventory and independent review. This yields a candidate, not qualification.
- [ ] Approve static host/control report and grant a separately signed, bounded
  qualification-only permit tied to reviewed code/protocol/host/budgets.
- [ ] Under that permit only, run timer/stability/eligible-cell qualification;
  retain unsuccessful pilots, verify memory/overhead limits, freeze the exact
  plan and obtain independent human review of host and workload reports.
- [ ] Grant a distinct campaign permit binding the approved qualification digest,
  all 14 eligible timed cells, fixed schedule/counts, budgets and evidence path.
  No automatic promotion from a passing test or qualification permit.
- [ ] At any future campaign, recheck launcher/worker authority and host identities
  per trial, enforce limits, abort on hard violations, seal incomplete evidence,
  and review results under the original statistical rules. This audit stops before
  all execution steps and grants none of these permissions.
