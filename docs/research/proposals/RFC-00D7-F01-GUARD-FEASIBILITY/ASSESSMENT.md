# F01 independent guard feasibility assessment

**Recommendation: UNSUPPORTED for the complete unchanged failure-path contract.**
This is a documentation/source feasibility finding, not an observed enforcement
failure, a proof that no conceivable system can satisfy it, or a governance
change. Narrow, explicitly scoped behavior may be conditionally testable, but
that does not satisfy the accepted contract or authorize its relaxation.

Assessment baseline: PR #811 `d0edeb486993860d733abc04b2ce740b35442874`.
Supervisor: PR #809 `0afbc339288b44cc7536af6c227f957257bb8ff6`.
Input profiles: PR #810 `0a369b8c7ce00979c970389e02e0da8d89d6ab0a`.
Protocol SHA-256: `d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3`.
Codex authored earlier source/plans too; this is not independent human/scientific
validation. No guard, interpreter or probe was implemented/run, no host observed,
no dependency installed or lock changed. The three PR branches are untouched.
H07 is context only; canonical H01–H08 and Hanoi/CWEB continuity remain intact.

## Decision scope and frozen obligations

Let C be the controller/finalizer, G the independent custom guard, R the helper
subtree, S the64MiB controller leaf containing C/G, and D the256MiB aggregate.
R is128MiB, swap0, one logical CPU; all custom work is charged to D. No separate
leaf/CPU/privilege/accounting exception is assumed.25MiB evidence and the separate
1GiB provisioning volume remain unchanged. linux_relock_required stays true.

The accepted design requires P0–P4<=44s wall/43s CPU, FINAL<=10s/5s, quiescence
by54s/48s, and termination within1s wall/1s charged CPU after the53s/47s final
write cutoff (or earlier entry-relative bounds). Manager expiry is Ta+55, with
0<=Ta-T0<=1; its required2s expiry-to-quiescence bound gives latest58s. Outer
ceilings remain60s wall/50s cumulative CPU. Every inequality includes custom
guard cost, startup, signaling, reaping and finalization. None is newly measured.

Three terms must remain distinct:

- Best effort: the mechanism attempts progress; no applicable response maximum.
- Bounded observed latency: maxima and uncertainty from a pinned finite test set;
  no claim outside those inputs/configuration/platform/load/fault conditions.
- Guaranteed deadline: an argument covering every admitted fault and dependency,
  including observation, dispatch, termination and reaping. A configured timer
  or a finite passing test set is insufficient for that claim.

The first two can be researched without changing numbers. Silently substituting
them for the third, or excluding named OOM/I/O/rights faults, changes the claim
and needs a separate program decision, not an implementation convenience.

## CPU observation, bandwidth and stopping

D/cpu.stat usage_usec provides hierarchical observation, not a cumulative kill
threshold. cpu.max regulates periodic bandwidth; CPU affinity does not reserve
execution for G. Neither interface supplies a50s lifetime cutoff. The kernel
interface description is a mechanism reference, not a target-host observation.
[Kernel cgroup CPU interfaces](https://docs.kernel.org/admin-guide/cgroup-v2.html#cpu-interface-files).

Reference Linux v6.8 rstat code flushes statistics before producing non-root
CPU usage. This rebuts the idea that users must simply add all per-process
snapshots, but supplies no numeric bound on read completion, observation-to-kill
latency or admission control. See cgroup_base_stat_cputime_show and flush paths:
[rstat source](https://github.com/torvalds/linux/blob/v6.8/kernel/cgroup/rstat.c).
Do not assert arbitrary permanent staleness either; inspect the actual kernel.

RLIMIT_CPU is process-scoped and inherited; a sequential/forking workload does
not become a single lifetime budget. A proposed bound from per-process limits
would need a proved complete process/exec/descendant model, which this plan does
not supply. [getrlimit](https://man7.org/linux/man-pages/man2/getrlimit.2.html).

A necessary paper condition is:

    U_stop + E_visibility + U_observe/dispatch/terminate/reap <= U_ceiling

For planned quiescence,47+remaining<=48; for the outer ceiling, remaining<=3
from47 to50. The accepted inner guard separately permits at most1 CPU second
and1 wall second. CPU consumed by G itself is included. No uniform bound on
those remaining terms is established here. Waking every millisecond does not
bound time spent unscheduled, stalled, in kernel code or waiting for authority.

If G is unavailable and C cannot progress while R remains runnable, a one-CPU
workload can consume50 CPU seconds before the55–56s runtime timer expires.
This is a possible missing-control path in the proposed provisioning design,
not a reproduced run or an assertion that #809 lacks its44/45 runner backstop.
The accepted source limits remain unchanged; no complete provisioning-helper
process/limit model has been supplied. Earlier OOM or
main-process-exit policy may stop a particular case; that must be established,
not presumed. The late wall fallback is not a substitute cumulative CPU guard.

## Feasibility matrix

Entries assess the complete contract, not whether an API exists. Conditional
means a useful component/subset, never acceptance of the unchanged whole design.

| Candidate | Supported role | Limit/failure dependency | Full contract |
|---|---|---|---|
| G polls D/cpu.stat from S, absolute timers | Custom threshold observation and decision | Shares S/D memory, scheduling and authority; threshold handling is not a kernel quota | UNSUPPORTED alone |
| cpu.max plus cpuset/affinity and weights | Bandwidth/placement and relative shares | No lifetime CPU budget or1s guard-service guarantee | UNSUPPORTED alone |
| Tighter periodic quota plus an independently proved lifetime bound | Potential CPU envelope below50 without a CPU watcher | Requires a proved end time and period/burst/in-flight slack; does not itself guarantee lifetime,1s response or reaping | CONDITIONAL calculation only |
| RLIMIT_CPU on helpers/controller | Kernel per-process backstop | Fork/exec/accounting topology and survivors must be proved; does not guarantee reaping | UNSUPPORTED alone |
| Separate G process in existing S | May survive C-only crash or interruptible wait | Same leaf OOM/reclaim/CPU fate; G failure is not C failure | CONDITIONAL component |
| memory.oom.group / OOMPolicy stop or kill | Fail-stop policy and manager reaction | Actual hierarchy/scope/default matters; no numeric response/reaping guarantee | CONDITIONAL component |
| pidfds plus R/cgroup.kill and subreaper | Stable targeting, subtree stop and child collection | Rights, containment, kernel progress and living reaper remain dependencies | CONDITIONAL component |
| Existing systemd unit runtime/kill policy | Trusted OS wall fallback already allowed by #811 | Expiry after active-enter; manager scheduling/stop paths; no stock cumulative threshold | CONDITIONAL component |
| A new external host-managed custom watcher | Can leave S's direct resource failure domain | Its custom CPU/memory/I/O are outside D; ordinary labels/chargeback do not repair this | NOT ADMISSIBLE |
| New G sibling/subgroup inside D | Could improve isolation and keep D charging | Changes the accepted shared64MiB leaf/two-leaf topology; still no hard real-time exit guarantee | NOT ADMISSIBLE as unchanged design |
| SCHED_FIFO/DEADLINE, RT kernel, mlock or OOM adjustment | Possible selected scheduling/memory mitigations | New privileges/config/admission/trust assumptions; not end-to-end I/O/exit proof; not current fair profile | NO ACCEPTED DESIGN |
| Kernel/BPF/hardware watchdog or manager extension | Possible research alternatives | No implementation, attribution, privilege, kernel-memory or end-to-end bound supplied; not a silent replacement | SPECULATIVE / UNSUPPORTED |

A stricter quota is worth separating from a lifetime kill threshold: with a
proved end-time bound, one could derive an upper CPU envelope from bandwidth,
period/burst/in-flight slack and elapsed time. No such worst-case kernel/host
argument is supplied here. Assuming the very bounded termination being assessed
would be circular; this alternative also leaves the required1s response/reaping
claim open. It is not applied or proposed as a silent resource revision.

Scheduler policy APIs and SCHED_DEADLINE describe resource/admission models, not
a generic deadline on arbitrary blocking code and process teardown. Nothing in
this assessment authorizes changing those policies or adding capabilities.
[Scheduling](https://man7.org/linux/man-pages/man7/sched.7.html),
[deadline scheduler](https://docs.kernel.org/scheduler/sched-deadline.html).

## Shared-leaf fault analysis

| Named fault | What S sharing permits | Effect on the claim / required evidence |
|---|---|---|
| S-local or D/global memory pressure/OOM | C and G share charge/reclaim pressure; either can lose progress or be selected for termination | Separate process identity is not memory-failure independence; exact OOM policy and ancestor behavior are missing |
| CPU starvation or quota throttling | G shares one CPU and D/ancestor bandwidth with the work it observes | Relative weight is not reserved response time; all dispatch/timer delays count, including manager delay |
| C blocked in I/O | A separate G might still run if G's path avoids that dependency | Useful subset only; G can itself page fault/reclaim/block, and a signaled target may not exit promptly |
| C death | G may survive; existing manager may also react to its actual main PID | Must prove G's parent-death/exec lifetime, main-PID/ExitType policy and owner for reaping; #809's runner parent-death behavior is not a guard-survival design |
| Lost signaling/delegation rights | pidfd identity may remain while signal permission fails; cgroup/IPC rights are separate | Need independently admitted remaining authority, not assumed privilege escalation or an untested D-Bus fallback |
| G death or C/G common-mode loss | An otherwise live service/helper can lose cumulative enforcement | Verify actual OOM/main-PID policy; existing runtime expiry is too late to prove the CPU cutoff |

Group OOM settings can select a group fate; they are not a reserved G allocation.
OOM immunity can affect victim choice, not guarantee successful allocation or
scheduling. Do not assume memory.min/low protects one process from its own shared
hard limit. [Kernel memory interface](https://docs.kernel.org/admin-guide/cgroup-v2.html#memory-interface-files),
[OOM adjustment](https://man7.org/linux/man-pages/man5/proc_pid_oom_score_adj.5.html).

systemd documents OOMPolicy and a Delegate-related default; #811 does not pin an
observed OOM policy. Explicit stop/kill might fail-stop some shared-OOM cases,
but the configured policy, hierarchy and latency must all be verified. An event
or requested unit stop is not evidence of completed bounded quiescence.
[Service OOM policy](https://www.freedesktop.org/software/systemd/man/255/systemd.service.html#OOMPolicy=).

## External termination, containment and reaping

cgroup.kill addresses descendants/fork races; it does not grant migration rights
to the workload or prove prior escape containment. Review delegated boundaries,
namespace/credential changes and all writable migration paths before relying on
the tree. A process-group signal alone misses changed-session descendants.
[Kernel cgroup core interfaces](https://docs.kernel.org/admin-guide/cgroup-v2.html#core-interface-files).

pidfds improve process identity, not authorization: EPERM remains possible.
[pidfd signal API](https://man7.org/linux/man-pages/man2/pidfd_send_signal.2.html).
A subreaper can collect adopted descendants; wait/WNOHANG can bound a caller's
wait attempt, not force a live child to exit by then. Signal success, exit,
population0 and successful reaping are distinct observations. Zombie-related
population observations cannot replace a wait result and complete owned-child
inventory. [Subreaper](https://man7.org/linux/man-pages/man2/PR_SET_CHILD_SUBREAPER.2const.html),
[wait](https://man7.org/linux/man-pages/man2/waitid.2.html).

Linux provides uninterruptible as well as killable waits. This is a concrete
reason not to infer a finite exit bound from SIGKILL issuance; it is not a claim
that every blocked I/O or D-state task is permanently unkillable. No bounded
return for every admitted kernel/device wait is established by the chosen
system. [Kernel wait interfaces](https://docs.kernel.org/driver-api/basics.html#wait-queues-and-wake-events).

Existing PID1/unit policy is the already allowed infrastructure fallback. It is
not immune to host OOM, starvation or kernel/device stalls. Suspend/VM pauses are
already excluded by #811 and are not used here as the sole counterargument. Configuring
KillMode=control-group/SendSIGKILL and a runtime timer supplies mechanisms, not
a guarantee of the accepted1s inner or2s outer completion/reaping bound.
[systemd kill procedure](https://www.freedesktop.org/software/systemd/man/255/systemd.kill.html).

## Accounting boundary: hosting a watcher does not make it free

Existing standard PID1 behavior already allowed by #811 remains that exception.
A new service/script, watchdog daemon, controller extension or periodic unit
performing F01-specific custom work outside D violates all-custom-work-in-D,
even if root-owned or called host-managed. Adding a separately measured number
to a receipt does not place its memory, CPU, affinity or bandwidth under D.
Replacing an external script with bespoke logic inside PID1 or the kernel does
not silently authorize a new accounting/trust exception; classification and
attribution would require explicit review. No such change is proposed here.

Moving G elsewhere within D could retain aggregate charging, but is not the
shared64MiB leaf topology currently accepted. It needs a separate amendment,
not a source-only implementation label. A new observer on another CPU would
also contradict the one-CPU custom-execution constraint. We make neither change.

## Recommendation, smallest next step and STOP rules

**UNSUPPORTED:** ordinary documented mechanisms and present repository source do
not establish the complete shared-leaf/fault/latency contract without additional
assumptions or design changes. This does not invalidate #809's accepted source
or #810's input byte identities; source acceptance was never host qualification.

Smallest next step: one independently reviewed admission-proof/decision record,
not guard implementation or host acquisition. Require an exact mechanism/fault
dependency trace for G loss while C remains alive, shared-OOM/reclaim, lost rights,
and nonreturning target waits; prove the47→48 CPU/53→54 wall and manager fallback
implications with all custom costs inside D. If no such argument exists, retain
UNSUPPORTED and stop this implementation lane. Do not implement a polling timer
and use successful normal-case probes as a substitute for that decision.
A narrower observed-latency research objective would need a separate explicit
program interpretation/approval; it is not adopted by this report.

STOP before implementation/provisioning if any of the following holds:

- Shared-leaf/failure dependencies lack an admitted fail-closed path or a surviving
  authority/reaper; guard disappearance can leave R runnable without a CPU bound.
- The claimed deadline requires unbounded scheduling, kernel/I/O or reaping
  progress, or finite samples are being promoted to a universal guarantee.
- Proposed rescue moves custom work outside D, adds a CPU/leaf, omits guard cost,
  relies on unapproved capabilities or changes the accepted accounting contract.
- Timer origin, CPU accounting/error budget, OOM policy, containment, rights,
  signal-to-quiescence or final evidence behavior is absent/unknown.
- A target test cannot be safely contained or budgeted under separate authority;
  no retry, cap increase or silent fault exclusion follows.
- Evidence/limit violations or missing receipts are relabelled success. No helper
  or post-kill writer can fabricate a terminal record after unit termination.

TEST_REQUIREMENTS.md defines the minimum evidence categories for any later
admitted candidate, **not an executable/authorized test campaign**. Passing them
could support only their measured scope. Under an unchanged universal timing
claim, tests alone cannot turn this recommendation into FEASIBLE.
