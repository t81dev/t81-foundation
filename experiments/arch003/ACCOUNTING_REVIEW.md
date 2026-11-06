# ARCH-003-I2 accounting closure review

**ACCOUNTING VERIFIED for the bounded Linux ARM64 fixture scope below.** This is
an implementation-agent assessment with raw evidence, not independent review,
full-campaign acceptance or `PASS_CORRECTNESS_CORE`.

I2 continues PR #716 from `e38574ec935f67542ca15f5679c1e80c22292cfa`.
Accepted protocol remains `1454bd63426d21686cfb5ef22dbaebc3c9cae94b`.
The new implementation SHA is in the PR and final handoff. Only
`experiments/arch003/` changes. Exact I2 inventory:
[I2_CHANGED_FILES.txt](I2_CHANGED_FILES.txt). No candidate/oracle mathematics, frozen schedule,
protocol, production source or CI registration changed.

Codex authored the implementation, fixtures and this assessment with I1/P0
context. Model identity is unestablished/null. Separate processes, test names,
source hashes and this review label do not establish reviewer independence.

## Mechanism and guarantees in scope

`linux_trace.py` runs inside L. No monitoring process, wrapper or preload library
is added. L forks its one job root, installs Linux ptrace fork/vfork/clone, exec,
exit and syscall observation, and follows every descendant before it can run
unobserved. Births are recorded before reading `/proc`, so missing identity data
still leaves a kernel PID for cleanup. PID/start-tick and thread-group identities
are checked on subsequent reads. Unexpected process depth/roster blocks.
Maintenance threads are recorded individually and their shared process RSS is
charged once; shared resident pages across different processes are counted in
each process's RSS.

Each process has its own `/proc/PID/status` VmHWM observations, including before
every exec syscall and at the early exit stop, before the address space is lost.
The named task's `wait4` raw RSS and wait status are retained at each reap.
These are separate receipts for the compiler driver and its actual tools,
including tools shorter lived than the sampling interval. Missing data is
explicitly null/incomplete and blocks acceptance; it is never replaced by zero.

Linux wait4's RSS can include waited descendants. The receipt labels that value
as a conservative bound, separately from the process's own VmHWM observations.
The aggregate uses the larger available OS bound for each identified process,
then the maximum sum over overlapping process lifetimes plus L's campaign peak.
It does not manufacture a missing tool receipt by doubling a driver maximum,
and does not add peaks of every historical sequential tool/job. Raw KiB values,
1024-byte conversion, process identities, birth/exit/reap events, configured
interval, actual largest sampling gap and sample count remain in the receipts.
These are OS-reported RSS quantities, not a claim of exact physical-page
measurement or instantaneous containment.

The owner retains the original absolute 512 MiB RSS, inclusive 1,800-second
monotonic deadline and 100 MiB output cap and category sub-budgets. The public
start value is read-only; jobs reset neither it nor the sampling clock. Polling
checkpoints target 25 ms; a gap over 100 ms blocks. Post-reap OS peaks can reject
a job retrospectively. An incomplete accountant cannot launch replacement work.

Tracing uses syscall-entry information to capture pre-exec high-water data.
Its execution and memory costs are charged, not assumed affordable. Unsupported
syscall architectures, unavailable ptrace or `/proc`, and nonleader exec are
blocked. Nonleader exec is rejected before the kernel remaps task identities.
Cleanup sends termination to every known task, including stopped tasks and
processes with unavailable identity, then drains trace events and reaps. No
uncaptured child or incomplete peak receipt can produce accounting completeness.

## Prefixes, failures and finalization

V retains fixed frames without a userspace output buffer. A completed
wrong-result/oracle failure journal is atomically published before worker
termination or wait. L preserves such completed failures over subsequent
resource/accounting interruption; expected guard rejections are excluded.
Recovery sets the header to the actual complete raw-frame prefix and retains any
partial bytes in a separate diagnostic. Recovery does not certify semantics.

`finalization.py` keeps the same owner clock active across prefix recovery,
resource receipts, hashing, index/root creation and final verification.
Chunked hash and metadata checkpoints enforce RSS/deadline/output limits through
sealing. Retained-byte accounting settles against the actual index size. If a
checkpoint fails, the owner retains prefixes and terminal diagnostics and
removes any stale root; it does not retry sealing after the cap or leave a
fresh success root. Bounded termination/reaping and terminal failure writes may
finish after the triggering deadline, but no study work or acceptance resumes.

## Executed validation and retained negatives

The actual Linux fixture host was an ephemeral Alpine 3.22.2 AArch64 guest on
QEMU 10.2.2/HVF, one vCPU, Linux 6.12.51-0-virt, Python 3.12.16 and Alpine Clang
20.1.8. The ISO URL/hash, full commands, package versions, compiler/fixture/source
hashes, command exits and stdout/stderr are in
[validation/i2/linux/run.json](validation/i2/linux/run.json) and adjacent files.
The VM's 768 MiB configured capacity includes its OS and diskless filesystem;
it does not change the accountant's 512 MiB study-process limit.

The 20 opt-in Linux tests passed. Complete job receipts recorded a largest
aggregate bound of 180,518,912 bytes and a largest observed sampling gap of
25,635,583 ns; these are resource-accounting observations for these fixtures,
not campaign feasibility or performance measurements. They cover actual Clang compile/link tool
receipts; transient sequential children; free-then-exec peak retention; L/V/W
verification of a fixed empty-BYTE observation; maintenance threads; abnormal
signals; unexpected/nested descendants; unavailable `/proc` at launch, mid-job
and exit; unavailable tracing options; injected PID identity change and sampling
gap; nonleader exec blocking; failure/prefix retention across deadline and
accounting termination; successful resource-bounded sealing; and deadline/output
failure during finalization. All recorded tasks in the cleanup edge fixtures
are asserted reaped. Clock/PID/proc faults are explicitly injected: no real PID
reuse, 1,800-second wait or physical-memory-exhaustion qualification is claimed.
Each test is a separate small infrastructure fixture, not a reset of campaign
phases. No normal candidate, campaign G1/G2, sanitizer diagnostic probe or LARGE
scientific input executes.

The first Linux run failed 2 of 13 tests: a partially visible failure journal and
stale pending-stop cleanup. Its source snapshot, logs and receipts are retained
under [validation/i2/first-failed/](validation/i2/first-failed/). Atomic JSON
publication and explicit stopped-task/reap bookkeeping corrected those paths.
A later fixture setup failed to compile because its dispatch branch was missing,
and its shell continued against an old binary. That failed run is retained
under [validation/i2/fixture-setup-failed/](validation/i2/fixture-setup-failed/),
not counted as coverage. The dispatch was repaired; final validation removes the
old binary and requires successful compilation before starting tests.
Intermediate passing development runs are not independent replications.

D/R/S standalone builds and the three bounded CTests were re-run on Darwin ARM64
with Apple Clang 21.0.0. Each profile also executed the 20 existing Python/oracle
component fixtures; all 20 Linux-only tests are explicitly skipped there.
See [validation/i2/macos/components.json](validation/i2/macos/components.json).
The S diagnostic target is compiled only. No sanitizer qualification is claimed.
Linux full D/R/S study builds were not run; Linux compilation here covers the
infrastructure fixture and its real compiler/linker receipt jobs.

## Review readiness, limitations and campaign blockers

The missing I1 compiler-tool receipt hook is implemented and supported by the
bounded kernel fixtures. The source and retained records are ready for
independent accounting review on the tested tuple. No universal accounting or
independent scientific validation is claimed. Windows and Darwin campaign
accounting remain unsupported. Linux x86-64, other kernels/libcs, restrictive
ptrace/seccomp configurations and actual sanitizer maintenance behavior need
host-specific checks; unsupported/missing observations retain
`BLOCKED_RESOURCE_ACCOUNTING`.

Before campaign authorization:

1. Independently review/accept this exact accounting source, scope and evidence,
   including peak aggregation, signal/exit cleanup and finalization.
2. Accept the two independently implemented oracles and independent verifier
   under P0's gate. I2 does not execute or accept candidate count behavior.
3. Name the exact resulting implementation SHA, accepted protocol SHA, qualified
   host, Clang/runtime identities, fixed backend and a new output location in
   explicit human authorization.
4. Treat feasibility of the complete builds, all oracle streams, six guards,
   sanitizer probes and all six entries under one 512 MiB / 1,800-second /
   100 MiB envelope as unproven until that separately authorized campaign.
   Tracing overhead and the bounded event ledger may cause an incomplete result;
   do not remove checks, retry automatically or drop coverage to obtain a pass.

Primary API references used for implementation review:
[Linux ptrace](https://man7.org/linux/man-pages/man2/ptrace.2.html) and
[Linux getrusage](https://www.man7.org/linux/man-pages/man2/getrusage.2.html).
Documentation is not evidence of behavior on an untested host.
