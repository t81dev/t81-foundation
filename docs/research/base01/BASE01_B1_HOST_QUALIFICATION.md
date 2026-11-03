# BASE-01 B1 host qualification — proposed design v0.1

**Status: design only; no host is qualified by this document.**
No timer calibration, warm-up, pilot, timing, resource or footprint collection
was performed for this task. Requirements are proposed acceptance rules, not
observed capabilities or claims that every Linux system can satisfy them.
Use the [measurement contract](BASE01_B1_MEASUREMENT_CONTRACT.md) and
[readiness matrix](BASE01_B1_READINESS_MATRIX.md). Source/Phase A/B0 pins in that
contract apply unchanged. Neither integration PR is modified or presumed merged.

## Candidate profile and mandatory static requirements

Initial candidate: a dedicated bare-metal Linux/x86-64 host. A container may
package tools but does not establish host ownership/isolation; do not use a shared
VM/cloud runner as an equivalent control. A different profile requires an explicit
protocol amendment. The earlier macOS ARM64 B0 pass does not qualify its host
for B1. Linux execution remains unverified and Windows unsupported in this packet.

The operator must record availability and readback for every mandatory item.
Unsupported controls, denied observations or missing evidence block qualification;
they must not be replaced by an environment assertion. Configuration changes
need operator authority and must be recorded/restored outside measured intervals.
This design provides no commands to enable measurement or alter host controls.

| Mandatory requirement | Required evidence and acceptance condition |
| --- | --- |
| Identity | CPU vendor/model/stepping, microcode, instruction features, physical/logical topology, SMT sibling sets, core type on hybrid CPUs, cache sharing, NUMA layout, RAM, motherboard/firmware, OS/kernel, boot ID, virtualization state and kernel command line. Pin tool versions and raw outputs; CPU ID alone is insufficient. |
| Core ownership | Reserve one physical core for the workload; offline its SMT sibling or reserve the entire sibling set without other work. Choose the logical CPU from topology, not a hardcoded core 2. Keep launcher, monitoring, interrupts and other user work on housekeeping cores to the extent the OS allows; record residual noise. |
| Effective affinity | Verify actual scheduler affinity for every worker thread before and after the trial, effective cpuset CPU/memory-node masks and observed placement. One workload thread; unexpected delegated computation/threads block the profile. No migration/hotplug permitted. A successful taskset request alone is not isolation evidence. |
| Isolation policy | Use an exclusive cpuset/partition or an equivalent documented reservation with readback and interference checks. Document IRQ/workqueue routing and shared-cache neighbors. No co-running CI, build, backup, indexing or user job on the reserved physical core/sibling. |
| Frequency/boost | Register CPU scaling driver, governor, limits, affected CPUs and firmware/OS boost policy. Initial profile uses performance policy with boost disabled and stable configured limits. Read back actual controls before/after; record supported runtime telemetry and its meaning. No silent fallback if control cannot be verified. |
| Thermal/power | AC power, fixed power policy and adequate cooling; identify meaningful package/core sensor units and applicable throttle/trip reference. Register a temperature band with upper bound at least 10 C below that reference, maximum spread 5 C, and telemetry cadence. No observed throttle event or band violation; absent usable sensors blocks this initial profile. |
| Load | Before session launch and after one minute of quiescence, retain at least three observations spaced 10 seconds apart with 1-minute global load <=0.10. During trials monitor background work separately; the active worker itself legitimately raises load. Do not enforce an idle global threshold on an intentionally busy core. |
| Memory | At least 8 GiB physical RAM, 4 GiB available at launch, fixed swap/THP/NUMA/allocator policy, no swapping or major faults in API intervals. Record minor faults rather than deleting affected trials. First-touch/prefault policy for harness buffers must match across trials. Intrinsic API allocation remains measured and may fault. |
| Budget | Record the contract's one-child/resource/time/evidence ceilings, preflight available memory/disk, and checks able to stop on violation. Sampling RSS is not a hard per-process RSS cap. An OS/cgroup limit has its own scope; do not claim it enforces exact process peak RSS. |
| Toolchain/build | Exact source and runner commits, dependency hashes, compiler executable/version, target triplet, linker/standard-library/allocator versions, CMake cache, full compile/link commands, architecture flags, binary/archive hashes and environment. No binary from a different build directory/revision is silently reused. |

Performance governors and boost behavior depend on the scaling driver; a governor
name or requested frequency is not proof of constant achieved frequency. Treat
`scaling_cur_freq` according to the driver's documented semantics. The mandatory
condition is a verified, reproducible control policy plus qualified stability;
no exact per-cycle frequency claim is made without evidence. No PMU, MSR event
sampling or cycle-based inference is used. See
[Linux CPUFreq](https://docs.kernel.org/admin-guide/pm/cpufreq.html).

The reservation requirement limits interference, not all kernel noise.
[Linux CPU isolation](https://docs.kernel.org/admin-guide/cpu-isolation.html)
describes housekeeping tradeoffs and available isolation mechanisms;
[cgroup v2](https://docs.kernel.org/admin-guide/cgroup-v2.html) documents effective
cpuset and memory-controller scope. Select the documented mechanism supported
by the actual kernel; reading a setting is not evidence it had the intended effect.
Attribution of frequency/thermal noise requires raw observations, not speculation.

## Reproducible build profile

Freeze the final profile before any campaign permit. Start from the audited
project's Release/C++23 configuration, benchmarks/TUI/examples/fuzz/llama/DPE/
TernaryOS disabled, deterministic-float setting on and tritwise profiling off.
Keep source, build, harness and output trees separate. Use one compiler/standard
library for project and harness. Reference optimization is `-O3 -DNDEBUG`, LTO
off, sanitizers/profilers/coverage off, without `-ffast-math`. Use a named x86-64
ISA target supported by the chosen CPU and record all source-specific flags
(including any audited AVX2/FMA objects). Do not call the entire binary generic
x86-64 if an input archive requires more features. Missing required CPU features
blocks the build, rather than selecting a different source implementation.

If `-march=native` is later proposed, resolve and record its effective target and
require an explicit profile amendment; it cannot be pooled with the reference.
Preserve audited global flags even if duplicated in CMake output. Review actual
commands, not just the user-supplied cache. Two clean builds must retain identical
configuration/path/environment records; compare whole-file hashes and native
section inventories. Explain nondeterminism before claiming bit reproducibility.
A source/configuration mismatch blocks eligibility; historical B0 artifacts do
not substitute for the Linux build. No build or new size observation is run here.

## Desirable controls, not hidden prerequisites

A dedicated NUMA node/cache domain, IRQ-offload tuning, full tick isolation,
disabling unrelated devices/services, a second independently operated matched
host and additional matched sessions would strengthen interpretation. Record
whether each was applied; do not require realtime scheduling, custom kernels,
PMU access, disabled mitigations or memory locking as implicit prerequisites.
Changes that affect semantics, security posture or costs must be reviewed and
recorded as a different profile. Desirable controls cannot compensate for a
failed mandatory affinity, identity, thermal or timer gate.

## Dynamic qualification — future authorization required

Even empty timer probes, stability checks, warm-ups and pilots are measurement
work. None may run under this design task. Static inspection alone can establish
only a candidate host report. A signed **qualification-only** permit is required
before the following future sequence; it does not authorize the 420-child campaign.

1. Validate inputs/correctness, required configuration and bounded allocation
   plans without timing. Check explicit VM halt and software-counter semantics
   before considering that cell. Missing workload prerequisites remain blocked;
   do not launch its pilot just to see what happens.
2. Identify the actual `steady_clock` backend and its monotonic contract; on Linux
   document its mapping before using `clock_getres` for that clock. Record nominal
   period and OS-reported resolution separately from empirical resolution.
   [Linux clock_gettime/clock_getres](https://man7.org/linux/man-pages/man2/clock_gettime.2.html)
   distinguish clock identity, resolution and behavior; nanosecond storage does
   not establish nanosecond accuracy. UTC/NTP is not the elapsed-time source.
3. Under that permit only, acquire 10,000 back-to-back empty interval pairs on the
   reserved core using the exact wrapper/compiler fences. Preserve every raw
   duration, zero/negative values and positive increment; define `q` as the larger
   of nominal/OS resolution (if mapped) and minimum observed positive increment.
   Define `e99` as the inclusive-linear 99th percentile of nonnegative empty
   wrapper durations, retaining zero values. No positive increments, backward
   elapsed values, unreliable mapping or unavailable clock evidence blocks timing.
4. Run at most three pilots per eligible cell within the global qualification
   budget and predetermined deterministic count-doubling rule. For every pilot
   require total raw elapsed `T >= 1000*q` and `B*e99/T <= 0.01`, with `B` the
   number of interval pairs. These are proposed conservative policy thresholds,
   not proven error bounds. Empty wrappers cannot model all syscall/scheduler
   effects. No overhead subtraction; preserve pilot failures and assembly evidence.
5. Final pilot must satisfy all gates and memory/output budgets; freeze its batch
   count and VM chunk partition without selecting a favorable speed. If the base
   count passes, keep it. A failed attempt can only trigger doubling for insufficient
   duration/overhead; correctness, host or accounting failure stops qualification.
   If the next doubling exceeds a bound or the third attempt fails, the cell is
   inconclusive/blocked for campaign, pending an explicit amended plan.
6. Qualify accounting using worker self values and exact waited PID, process
   lifecycle, raw units, fixed descendant policy and normalization fixtures.
   Qualify artifact tools and output retention. The orchestrator smoke check from
   B0 is not child-accounting qualification or an independent memory oracle.
7. An authorized operator reviews retained evidence and records pass/fail for
   each mandatory host gate and each cell. A campaign permit must subsequently
   bind that reviewed report and final batching plan. A runner cannot approve
   itself by setting a `qualified` JSON field.

During qualification/campaign, capture topology/control readback and host state
at start/end of every child, and temperature/frequency/load/fault observations
at the registered cadence from housekeeping CPUs. Monitoring overhead and any
shared-cache interference must be recorded and held fixed. Choose cadence and
temperature band before probes; absent validated settings blocks qualification.
Read CPU usage/interrupt/fault telemetry through OS interfaces only, with no PMU.
Any affinity/control/thermal violation stops the run and retains the record.
Unexplained timing CV or order drift is inconclusive even without detected host
violations; a clean telemetry trace is not proof of no interference.

## Report identity, lifetime and limits

The host report must identify operator, host and boot, protocol/source/runner/
binary/input hashes, exact control policy, tool outputs/errors, timer-wrapper
identity and calibration samples, pilot counts/results, accounting scope,
thermal/load observations and all limits. Bind the complete report by SHA-256
and the maintainer-approved authorization process. Record evidence provenance;
a digest alone does not certify truthful observation.

The report expires on reboot, microcode/firmware/kernel/toolchain changes, core/
NUMA/affinity/frequency/boost/security-policy drift, changed binaries or input
identities, invalidated timer/accounting data, or a detected violation. A new
permit/report is needed; do not silently resume a partial campaign. Mere elapsed
wall time is not requalification. The permit also has a finite explicit expiry
and revocation check; no universal trusted clock is assumed without review.

No suitable Linux/x86-64 host is demonstrated by this packet. macOS remains
B0-only, Linux execution unverified and Windows unsupported. Exact qualification
thresholds are a proposal for human review, not an assertion that the current
Mac, a general CI machine or every bare-metal Linux host meets them.
