# H04-R03 Linux enforcement feasibility investigation

**Overall disposition: FEASIBILITY BLOCKED.**

The frozen protocol's exact continuous RSS and complete scratch-enforcement
requirements are not established by the evaluated stock Linux mechanisms. This
is an investigation and a set of bounded counterexamples, not an implementation
of a benchmark collector or a recommendation to relax either limit.

Baseline: `dbc8c1ac05aebf9f64dccc45968b75e891b6ff21`, verified current `main`;
PR #682 merged at `1c2fc645176030bb76e52e5deec0ee66ee5ecfbb`, PR #683 at the
baseline SHA. The authoritative frozen preregistration is retained at
[`8e44c0f667c62e0d4143bb93169fc3235d5ebd66`](https://github.com/t81dev/t81-foundation/blob/8e44c0f667c62e0d4143bb93169fc3235d5ebd66/docs/research/continuity/H04_R03_CANDIDATE_A_PREREGISTRATION.md),
Sections 6–8. Its earlier removal from the current tree is not repaired here.
The accepted codecs, workloads, limits, statistical definitions, execution freeze,
and historical records are unchanged.

This supports canonical H04 through EXP-H04-R03's fixed-codec apparatus. It tests
no adaptive scheduler, radix advantage or codec performance. No Phase C run or
four-arm campaign occurred. The diagnostics do not import the codec/harness.

## Evidence scope and status rules

The actual available host was **macOS 26.5.2 ARM64**, not a dedicated Linux
x86_64 machine. A disposable **QEMU 10.2.2 TCG x86_64 guest** ran Alpine 3.21.9,
Linux `6.12.114-0-virt`, one virtual CPU and 1024 MiB RAM. These are real guest
kernel syscalls under full-system emulation, not mocked process/cgroup outputs.
They cannot qualify physical CPU, cache, frequency, tenancy or platform timing.

The guest used GCC 14.2.0-r4, binutils 2.43.1-r3, musl 1.2.5-r12, Linux UAPI
headers 6.6-r1 and CPython 3.12.16-r0. The dynamically linked diagnostic was
built twice with identical output SHA-256
`fb004923584b0bee31be1100c41dcc21c4d3c3db70cd1a37f0a6ec0471e1f2d0`.
This proves repeatability of those two builds in that installed environment,
not hermetic or cross-host reproducibility. Full package inventory, compiler,
linker, libc and Python binary hashes appear in [the raw run](evidence/diagnostics.final.log).
The ISO's downloaded bytes matched its publisher's HTTPS SHA-256; a PGP
signature and independent kernel rebuild were not verified.

`PROVEN` below is restricted to a named finite observation/counterexample or
specified API contract. `PARTIALLY DEMONSTRATED` means some required properties
have evidence but the whole requirement does not. `BLOCKED` means a mandatory
property has no demonstrated conforming mechanism in the evaluated setup.
`NOT TESTED` means no qualifying execution/evidence was obtained. No passing
unit test, exit code or hash upgrades these meanings to overall feasibility.

| Requirement | Classification | Evidence and limit |
| --- | --- | --- |
| F1 — containment before worker user-space execution/startup | PARTIALLY DEMONSTRATED | `clone3(CLONE_INTO_CGROUP)` ran in the root guest; first marked child operation, parent observation and post-dynamic-exec observation agree. Source/API ordering supports pre-runnable placement. Dedicated-host, unprivileged delegation and kernel-binary/source attestation remain untested |
| F2 — exact continuous process RSS <=32 MiB | BLOCKED | Neither RLIMIT_RSS nor memory.max supplies this policy; concrete counterexamples below. Sampling/peaks are not continuous admission enforcement |
| F3 — complete <=64 KiB native/interpreter scratch enforcement | BLOCKED | Direct native allocation bypassed Python tracing. Complete allocation/lifetime/accounting coverage of the accepted worker is not demonstrated |
| F4 — retention and reclamation | PARTIALLY DEMONSTRATED | One Python alias lifetime and one process exit demonstrated. Retained shared backing remained resident after exit. Allocator-page release, complete physical reclamation and cache reset are not established |
| F5 — dedicated Linux x86_64 qualification | NOT TESTED | Only an emulated one-vCPU guest was available; no dedicated physical Linux host was qualified |

The machine-readable [requirements-to-evidence matrix](requirements-to-evidence.json)
separates these top-level classifications from proven *counterexamples*, which
are not successful enforcement results.

## F1 — launch-time containment

The native diagnostic opens an already provisioned cgroup-v2 leaf, verifies the
filesystem and 32 MiB `memory.max`, and calls the raw `clone3` syscall with
`CLONE_INTO_CGROUP`, a directory FD and SIGCHLD. There is **no fork/migrate
fallback**. The child uses fixed buffers and raw syscalls for its first marked
membership observation, signals the parent, and waits. The parent independently
reads `/proc/CHILD/cgroup` and the target's `cgroup.procs`, then permits an
`execve` of the same dynamically linked diagnostic. ELF PT_INTERP names
`/lib/ld-musl-x86_64.so.1`; post-exec membership is recorded.

Observed in [the final run](evidence/diagnostics.final.log):

```text
parent PID 1936:                      0::/
first marked child operation, 1937:   0::/h04-feasibility
parent's /proc/1937/cgroup read:      0::/h04-feasibility
target cgroup.procs:                  1937
post-exec PID 1937:                   0::/h04-feasibility
```

This observes checkpoints, not every instruction or dynamic-loader allocation.
The pre-user-space guarantee relies on the trusted kernel contract. In inspected
Linux 6.12.114 upstream commit
[`85d82e0d7c2dfd3e75d34f35b7ebbf41f45c9c4e`](https://github.com/gregkh/linux/commit/85d82e0d7c2dfd3e75d34f35b7ebbf41f45c9c4e),
[`copy_process`](https://github.com/gregkh/linux/blob/85d82e0d7c2dfd3e75d34f35b7ebbf41f45c9c4e/kernel/fork.c#L2507)
checks fork policy and calls `cgroup_post_fork` at line 2637 before returning to
`kernel_clone`, whose `wake_up_new_task` is at line 2854. Target validation and
attachment are in
[`cgroup_css_set_fork` / `cgroup_post_fork`](https://github.com/gregkh/linux/blob/85d82e0d7c2dfd3e75d34f35b7ebbf41f45c9c4e/kernel/cgroup/cgroup.c#L6588).
Thus the mechanism can establish membership before the child resumes user space
and before a subsequent exec's loader runs, under that kernel contract. The
Alpine binary was not rebuilt and compared against this upstream source plus
Alpine patches; source inspection is not binary attestation.

Limits that remain important even with successful placement:

- Launcher initialization happens in the parent. Inherited mappings, shared
  pages and earlier charges do not become wholly owned by the destination
  merely because the child is born there. F1 placement does not solve F2.
- The ready/parent checks occur after the first child instructions. They alone
  cannot prove the earlier boundary; the kernel contract is part of the argument.
- Cgroup placement is not a security sandbox. This trusted diagnostic remained
  root; it did not demonstrate prevention of a privileged worker changing limits
  or moving itself. A production privilege/delegation boundary is untested.
- A libc fork followed by writing `cgroup.procs`, a child-side pre-exec callback,
  or moving an already running interpreter is too late for the stated boundary.

### Kernel, permissions and delegation requirements

The [clone API](https://man7.org/linux/man-pages/man2/clone.2.html) introduces
`clone3` in Linux 5.3 and this flag in **5.7**; distro version strings alone do
not prove availability. Cgroup v2 and required kernel configuration/controllers
must actually exist. The chosen infrastructure also needs usable memory/pids
controls and peak/event interfaces; availability must be probed on the pinned LTS
kernel. No special set_tid/checkpoint capability is used by this diagnostic.

The directory FD must identify a live v2 group, not a `cgroup.procs` file. The
caller's credentials must satisfy target and common-ancestor migration permissions,
namespace/delegation visibility, and destination domain/type constraints. See
[the pinned permission call path](https://github.com/gregkh/linux/blob/85d82e0d7c2dfd3e75d34f35b7ebbf41f45c9c4e/kernel/cgroup/cgroup.c#L5221).
Correctly delegated unprivileged use is possible in the API model; root is not
an unconditional API requirement. Only the root case was executed here.
Seccomp/LSM policy can deny the syscall. EACCES, EBUSY, invalid FD/type, unsupported
syscall and controller failures must stop execution, never trigger a weaker fallback.

Use a controller-enabled, process-free delegated parent, separate manager and
empty worker leaves, and preinstall limits before child creation. The guest's
root cgroup is an exception to the no-internal-process restriction; do not copy
that layout blindly into an unprivileged non-root subtree. Memory and pids must
both be enabled in the parent to expose their respective leaf controls. The
initial controls setup failed when only memory was enabled; this failure is
[retained](evidence/diagnostics.first-controls-failure.log).

## F2 — exact continuous RSS

Process RSS counts resident mappings; it is different from address-space size,
proportional/private usage, allocator requests, and cgroup-accounted charges.
Readouts are observations. Even accurate detailed page-table walks are not an
atomic future-allocation veto. `/proc/PID/status` and `statm` contain approximate
RSS counters; [the proc API](https://man7.org/linux/man-pages/man5/proc_pid_statm.5.html)
recommends smaps/rollup for accurate detailed observations. A periodic watcher,
peak read, event notification followed by SIGKILL, or proactive reclamation does
not establish an exact never-missed continuous boundary.

Two bounded synthetic counterexamples actually executed:

| Candidate substitute | Observed counterexample | Classification of counterexample |
| --- | --- | --- |
| RLIMIT_RSS set to 33,554,432 bytes | Process touched a 48 MiB private mapping and survived with smaps_rollup RSS **50,880,512 bytes** | PROVEN for this run |
| `memory.max=33,554,432` with launch-time containment | Parent precharged 48 MiB shmem; contained child mapped/touched it. Child RSS **50,892,800 bytes**, group current **245,760 bytes**. Historical group peak **262,144**, all OOM counters zero | PROVEN for this run |

The first agrees with the documented modern-Linux ineffectiveness of
[RLIMIT_RSS](https://man7.org/linux/man-pages/man2/getrlimit.2.html).
The second directly disproves the inference that a 32 MiB memcg bound implies
a 32 MiB process RSS bound, even when initial child placement is correct.
Memory ownership, shared charges, extra kernel/file accounting and permitted
transient overshoot are described in the
[cgroup-v2 contract](https://www.kernel.org/doc/html/v6.12/admin-guide/cgroup-v2.html).
This run did **not** demonstrate overshoot; that is a separate documented limit.

`RLIMIT_AS` limits virtual address space. Under a proved ordinary-mapping model,
RSS cannot exceed the mapped address-space bound, so a 32 MiB AS ceiling could
be a **stronger sufficient constraint**, not a weaker memory metric. It is not
an RSS-triggered policy: it can reject sparse/reserved mappings with low RSS.
Setting it on an already large process also does not remove existing mappings.
A valid use would need proof of initial parent/child mappings, fork/exec and all
mapping paths, plus admissibility of the *unchanged* interpreter/dependency and
lifecycle footprint. That proof and actual-worker fit were **NOT TESTED** here;
no AS ceiling is adopted as an established solution. A correct stronger-bound
proof must not be dismissed merely because its control variable differs from
RSS, but neither may it be assumed from a limit setting or a small diagnostic.
`RLIMIT_DATA`, allocator limits and page locking likewise constrain different
quantities. No alternate metric or reduced cgroup budget is substituted for the
registered RSS rule.

**Finding:** no conforming continuous RSS-bound proof/backend was established for
the frozen worker with the evaluated apparatus. The directly evaluated RSS and
memcg substitutes fail. This is **not a universal impossibility proof** about
all available Linux constructions: conservative address-space bounds remain an
explicitly untested avenue, and a custom kernel or closed-world mapping design
could also change the analysis. Any candidate needs its own complete proof and
unchanged-worker admissibility evidence. No such redesign or experiment is
implemented in this feasibility PR.

## F3 — complete scratch enforcement

The synthetic Python probe requested **131,072 native bytes**, touched and freed
them through libc via ctypes, while tracemalloc's recorded peak was **304 bytes**.
The [Python allocation model](https://docs.python.org/3.12/c-api/memory.html)
permits C-library allocations outside Python's memory manager. This is a concrete
coverage counterexample, not a measurement of any codec's scratch requirement.
The ctypes path is deliberately synthetic; it is not claimed to be reached by
the accepted Phase 6 call paths.

| Technique | Missing coverage / limitation |
| --- | --- |
| tracemalloc/Python-domain hooks | Unregistered native malloc/mmap, private arenas, stack/TLS, native bookkeeping; tracing peaks does not refuse allocations beforehand |
| libc malloc-family interposition | Direct syscalls, static/private allocators, arena suballocation, stack buffers; bootstrap, recursion and thread safety must also be covered |
| mmap/brk syscall filtering | Cannot observe object allocation inside already reserved/resident arenas or native stack use; mapping size is not live scratch bytes |
| allocation event sampling/uprobes | Missed/uninstrumented paths, dropped events and retrospective action do not prove synchronous cap enforcement |
| explicit reservations | Only allocations routed through the interface; requested bytes are not automatically allocator footprint, metadata or stack consumption |

Known Phase 6 source paths use Python frames, bytes slicing, struct unpacking,
PyLong arithmetic/division, dictionaries and bounded target-block lists. Their
logical work is bounded by the fixed k; that is not proof of the interpreter's
entire allocation footprint. The Phase 7 xxHash64 backend is outside the Phase 6
scratch window but remains inside lifecycle RSS accounting. Do not use this
synthetic escape to allege an observed codec cap breach: no codec was run.

Complete coverage needs an audited, pinned interpreter/native dependency graph,
all allocation/free/reallocation paths, stack/TLS and allocator-internal bounds,
monitor overhead accounting, concurrent/reentrant behavior, and pre-allocation
failure/retained-evidence semantics. It must distinguish peak live scratch from
cumulative allocated traffic and account for transient coexistence during copy
or realloc. No complete proof or production backend was demonstrated. A redesigned
closed-world native worker might permit a different proof, but rewriting codecs
or altering the accepted experiment is outside this investigation.

## F4 — retention and reclamation

| Property | Classification | What is and is not established |
| --- | --- | --- |
| One Python object's logical lifetime | PROVEN, scoped observation | Weak reference stayed live with an alias and died after its last strong alias plus collection. Not all cycles/finalizers/extension-held references |
| Native buffer logical lifetime | PARTIALLY DEMONSTRATED | Known successful malloc, touch and free call; no universal native-alias or allocator ownership proof |
| Allocator release to OS | NOT TESTED | free/object death does not prove arena release. [malloc_trim](https://man7.org/linux/man-pages/man3/malloc_trim.3.html) is allocator-specific and attempts page release, not a universal guarantee; it was not used |
| Diagnostic child's process address space ended | PROVEN, scoped child | waitpid returned normal exit 0 and `/proc/1937` was absent; inspected [exit_mm/mmput path](https://github.com/gregkh/linux/blob/85d82e0d7c2dfd3e75d34f35b7ebbf41f45c9c4e/kernel/exit.c#L545). No CLONE_VM sharing in the probe |
| Shared physical backing all reclaimed by exit | BLOCKED as a general implication | After exit, retained shmem had a resident page in the mincore snapshot and value 90; empty group still accounted 53,248 bytes. Process destruction does not imply all shared backing disappears |
| Every relevant private physical page returned | NOT TESTED | Requires a complete ownership/reference model, including kernel/pinned/shared references and reclamation semantics; not inferable from RSS differences |
| CPU cache eviction/reset | NOT TESTED | No physical host/cache procedure or line/domain evidence. Emulation and a new process are not cache-reset evidence |

[mincore](https://man7.org/linux/man-pages/man2/mincore.2.html) is a residency
snapshot, not continuous monitoring or physical-frame identity tracking. Kernel
page reuse/compaction and shared/file-cache lifetimes are separate issues.
Logical release, allocator reuse, kernel unmapping, physical-page release, and
data zeroing are distinct; zeroing is not silently added as a new protocol
requirement here.

An ordering fence neither frees storage nor evicts caches. `munmap` removes
mappings; [MADV_DONTNEED](https://man7.org/linux/man-pages/man2/madvise.2.html) has
mapping-specific effects and is not a universal shared-page eraser. Linux
[`drop_caches`](https://www.kernel.org/doc/html/v6.12/admin-guide/sysctl/vm.html#drop-caches)
acts on kernel caches, not a defined reset of CPU L1/L2/L3 contents. None was used
as a substitute for the registered retention or cache conditions. The exact
residency/reclamation and cache-reset scopes need operational definition and
host evidence before execution, not an agent-selected weaker interpretation.

## F5 — host feasibility and reproduction

[HOST_QUALIFICATION.md](HOST_QUALIFICATION.md) is the qualification checklist.
It includes kernel/config/delegation, all enforcement proofs, frequency/turbo,
affinity/SMT, caches and reset, swap/NUMA/pages, compiler/dependency identities,
reproducible builds, failure retention and human authorization. It is **not a
completed qualification record**. The observed VM has one vCPU, no isolated CPU
list and no governor/turbo control interfaces; these are not measurements of the
physical macOS host's corresponding properties.

[REPRODUCE.md](REPRODUCE.md) gives exact commands, environment and retained failure
history. [evidence/host-environment.json](evidence/host-environment.json) records
host/guest scope and QEMU identity. The evidence manifest pins raw logs, diagnostic
sources and report artifacts. Hashes prove content identity, not factual truth,
independent authorship, trusted execution or authorization.

Author: Codex, also author of PRs #682/#683; model identity unrecorded. This is not
independent scientific reproduction. Independent review and dedicated-host
validation remain outstanding. Passing the offline evidence verifier checks only
artifact consistency and recorded counterexamples; it cannot establish feasibility.

## Unresolved blockers and stopping point

1. No exact continuous RSS-bound enforcement demonstrated for the frozen
   worker and cap; the principal substitutes have concrete counterexamples.
2. No complete peak-live native/interpreter scratch admission proof, including
   internals and transient allocations, at the unchanged 64 KiB ceiling.
3. No dedicated Linux x86_64 qualification, unprivileged delegation/security
   validation, or kernel-binary/source attestation.
4. Required allocator/residency/reclamation and physical cache-reset conditions
   lack a complete operational proof on a selected host.
5. Full environment/build/archive reproduction and independent verification are
   pending. No experimental execution authorization has been granted here.

The report neither amends the preregistration nor promotes the apparatus to
ready status. The existing freeze and unqualified state are untouched. No merge,
Phase C measurement, comparative codec result or performance advantage is claimed.

**Overall disposition: FEASIBILITY BLOCKED.**
