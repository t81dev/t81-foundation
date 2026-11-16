# Minimum future target evidence — requirements only

No implementation, host acquisition or probe is supplied or authorized. First
resolve the admission-proof decision in ASSESSMENT.md. These are evidence
categories, not a claim that a finite suite proves universal timing. Every row
is currently NOT VERIFIED. An admitted candidate needs exact source/tool/kernel/
manager/host and human authorization before any observation may be collected.

## Preconditions and measurement discipline

Pin the actual kernel build/config/boot, CPU/architecture/hypervisor and load
scope, manager binary/version/effective unit properties, cgroup mounts and FD/
inode identities, controller chain, OOM policy, credentials/capabilities/namespaces,
filesystem/storage/durability behavior and guard/controller/helper sources.
No substitute host, current/latest image, unobserved merge or configuration-only
receipt is evidence. Linux x86_64 and ARM64 need separate qualification scopes.

Every record needs same-clock T0, Ta, phase/FINAL absolute cutoffs, pre-origin
fresh-D usage and subsequent D/cpu.stat samples, sample begin/end timestamps,
trigger/delivery/signal/exit/wait/population observations, PID/start/pidfd and
cgroup identities, limits/events/peak counters, source/config hashes and errors.
Report uncertainty, sampling interval, counter visibility and observer overhead;
no cross-clock subtraction without a validated mapping. Record actual sampled
facts, not inferred signal/exit timestamps. Signals and reaping need separate
receipts, including exit status and whether a survivor or zombie remained.

All custom instrumentation/load/fault helpers must remain in D and share its
one CPU,256MiB aggregate,64MiB controller/128MiB helper and zero swap. No test
observer outside D or independent per-row allowance is implicit. A new guard/
tool/probe requires source review, not implementation during a host run.

## Necessary evidence categories

| ID | Minimum scenario or source obligation | Required retained evidence / falsifier |
|---|---|---|
| G00 admission | Read-back full topology/rights/OOM/unit/clock/configuration and immutable identities before release | Any missing primitive, unknown behavior, dirty identity or unsupported shared-failure dependency blocks before work |
| G01 accounting | Guard/controller startup, sequential helper/exec and bounded descendants, plus finalization | Absolute fresh-D totals include all custom work; no post-gate baseline, migration escape, refreshed RLIMIT assumption or missing observer cost |
| G02 deadlines | Phase prefix/entry bounds,43/44 work transitions,47/53 cutoffs and48/54 quiescence | Raw observation/decision/signal/exit/reap timing; any overrun fails that phase; late detection reduces remaining FINAL, never resets it |
| G03 C failure | C dies or stops progressing while a bounded R/descendant is still alive | Actual G/manager authority and reaper ownership; no success from main PID disappearance or expected PDEATHSIG alone |
| G04 shared memory | Actual configured S-local OOM/reclaim and relevant D/ancestor effects | memory.events/OOM victim/unit policy and G progress; no survival claim from process separation, unused D headroom or a configuration string |
| G05 guard/common-mode loss | G unavailable while C remains alive/nonresponsive; failure of the cumulative CPU path | Independently justified remaining cutoff before50 CPU seconds; a later55s runtime stop is not a passing CPU result |
| G06 scheduler/CPU | Allowed one-CPU contention and D/ancestor throttle conditions within the reviewed profile | All control overhead and wakeup/read/decision/kill delays; relative weight or intended poll interval alone fails the evidence requirement |
| G07 blocked paths | Controller-only interruptible stall; separately justified kernel/storage dependencies | Proof of G dependency independence and bounded target exit; a pipe-stall test is not evidence about arbitrary uninterruptible device waits |
| G08 authority | Actual reviewed credentials/delegation and controlled loss/denial of each required signaling/kill/IPC right | errno/identity/authority and remaining admitted path; pidfd stability or root-owned labeling does not establish permission |
| G09 containment/reaping | Bounded descendant with changed session plus controller/reaper loss case | No escaped process, full exit/wait status and population observations; wait timeout/population0 alone cannot assert all children reaped |
| G10 manager fallback | Actual startup and runtime-expiry stop state paths, extensions disabled and no hooks/restart | Ta-T0<=1, expiry55–56, total expiry-to-quiescence<=2 incl delivery/reaping; no timing inference from TimeoutStopSec or unit-state label |
| G11 evidence death | Capacity/write/flush failure and termination before/within FINAL seal | Prior confirmed checkpoints/partial tails preserved, original cause retained; no invented receipt or successful post-kill persistence |

G00/G01/G03/G08/G09 establish capability and scope; G02/G06/G10 measure timing
only for the tested configuration. G04/G05/G07 expose failure-model gaps that
cannot be waived because benign cases pass. A host test cannot prove that every
kernel/device wait will return or every future scheduling interval will be bounded.
If that guarantee remains required, the design argument is a prerequisite and
finite test success does not discharge it.

## Safety, bounds and evidence handling

This table is not a schedule. A55s fallback case can consume most of a60s campaign;
all rows are not claimed to fit one allowance. A separate reviewed qualification
protocol must select/order cases and allocate setup, faults, cleanup, observers,
states/observations and artifacts without exceeding existing60s wall/50s CPU,
256MiB/one CPU/zero swap/25MiB and any applicable49,408 observation ceiling.
Existing1GiB provisioning storage is not a new probe artifact allowance. No row
receives an automatic fresh budget, retry or second host. No A–F code/control/
fixture is a qualification helper. Actual fit is unknown and can block the plan.

Do not induce host-wide OOM, uncontrolled fork/RT load, stuck devices, new kernel
modules or unrecoverable I/O on a shared host. If a named fault cannot be safely
contained under separate authorization, retain NOT VERIFIED and STOP for that
claim. Test-only independent safety bounds may be necessary, but record them:
a helper stopped by its own safety limit does not demonstrate that G stopped it.
No source, runtime policy, external observer or extra CPU can be smuggled in as
measurement infrastructure. Simulating EPERM or an interruptible wait does not
establish actual credential-loss or uninterruptible-exit behavior.

Reserve terminal/cleanup evidence space before a case. Keep raw counter/time/
status/error records and confirmed checkpoint hashes within the25MiB cap; record
sampling gaps, write/flush scope, trace truncation and uncertainty. If the unit
is killed, do not start a replacement finalizer. Later authorized export of an
existing manager record labels its time/scope and does not retrofit a successful
operation. Missing signal-to-exit/reaping/persistence evidence is incomplete,
not a latency of zero or an omitted failed sample.

An eventual result report must publish configuration, raw samples, maxima,
uncertainty, case coverage and exclusions, including negative/inconclusive results.
Do not turn the largest observed delay into an asserted worst-case bound. Without
an admitted deterministic dependency argument, no sample count establishes the
unchanged1s/2s guarantees. No observations or results are present in this packet.
