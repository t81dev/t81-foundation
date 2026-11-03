# Resource accounting contract

## Identity, lifecycle and API boundaries

`Collector.launch` checks the fixed fixture source/executable hashes, rejects
non-fixture modes and captures parent snapshots before spawning. It calls
`posix_spawn` with a fixed executable, a fixed mode allowlist and `LC_ALL=C`.
The returned positive PID belongs to one collector instance in one parent
process. A handle from another collector, another parent, or a consumed handle
is rejected before waiting. Transferring a collector across `fork` is rejected.

The child reports `getpid()` and `getppid()`, deterministic payload size/checksum,
and a `getrusage(RUSAGE_SELF)` snapshot. Both identities must match the owned
spawn record. `wait4(child.pid, 0)` waits for that exact child, never `-1`, and
retains the raw wait status, decoded exit code/signal and resource structure.
Only normal exit 0 is accepted. Exit 7 and SIGTERM are intentional rejection
fixtures; their observations remain in the receipt. Successful reaping consumes
the handle even if subsequent report validation fails. An attempted failed
reap also consumes the handle; it cannot later accidentally refer to a reused
PID. ECHILD is an explicit failure, not an empty successful sample.

This is a single-process ownership contract, not protection against another
thread or foreign library intentionally reaping owned children. That conflict
fails closed (tested by an external `waitpid`). There is no concurrent campaign
manager or descendant process tree. The fixture is trusted and bounded; no
timeout or timing-based killing policy is implemented. An unexpected OS wait
error must be diagnosed by the operator; no accounting success is invented.

## Three different accounting scopes

| Observation | Selector/API | Subject | Meaning |
| --- | --- | --- | --- |
| Per-child lifetime | `wait4(owned_pid, 0)` | Exact waited PID | Resource accounting returned while reaping that child |
| Child pre-exit snapshot | Child `getrusage(RUSAGE_SELF)` | Child PID, PPID checked | Snapshot before JSON reporting and termination; not the entire lifetime |
| Parent self | Parent `getrusage(RUSAGE_SELF)` | Parent PID | Calling process including its threads; never relabeled as child usage |
| Aggregate children | Parent `getrusage(RUSAGE_CHILDREN)` | No per-PID subject | Cumulative waited children, potentially including compiler and other prior children |

Parent snapshots are retained before spawn and after reap. They are not
subtracted to manufacture per-child records. In particular, an aggregate peak
RSS is not a sum or a per-PID delta. Linux defines `RUSAGE_CHILDREN.ru_maxrss`
as the largest child's high-water mark, not the whole process tree's peak.
No cross-platform interpretation of the aggregate peak beyond its recorded
selector and native field is established here.

User CPU seconds, system CPU seconds, raw max RSS and normalized peak RSS are
separate fields. Native structures are retained in `raw_rusage` for parent and
wait4 calls. The child preserves timeval seconds/microseconds and raw max RSS.
Missing/nonfinite/negative required fields, malformed results, mismatched PIDs,
invalid selectors, malformed JSON and incorrect deterministic outputs reject.
Zero CPU and zero RSS values are permitted; a small fixture need not produce
a particular nonzero counter.

The child's pre-exit CPU snapshot must not exceed the corresponding lifetime
CPU observation, allowing one microsecond solely for timeval conversion
precision. Its RSS snapshot must not exceed the lifetime peak. These are
ordering/representation checks, with no latency, throughput or resource-budget
threshold. Tests never require exact OS CPU or RSS values or a particular rise.

## Units and scientific limits

Linux `ru_maxrss` is KiB; macOS `ru_maxrss` is bytes. The collector retains the
native value/unit and produces canonical KiB (`raw` on Linux, `raw / 1024` on
macOS). A caller cannot override the actual platform for real samples.
Unit and unsupported-platform tests are deterministic synthetic contracts;
Linux and macOS execution receipts separately establish actual API execution.

CPU time is scheduler accounting, not elapsed time. RSS is a lifetime resident
high-water mark, not incremental allocation, address-space size or a sum of
concurrent processes. These observations cover processes, not system-wide
resource consumption, hardware energy or hypothetical ternary hardware.
Correct accounting does not establish benchmark repeatability, efficiency,
speedup or B1 host qualification. Emulated Linux execution establishes API
correctness in that guest only.

## Fixture bounds and output retention

The native fixture touches exactly 65,536 volatile bytes on its stack, once,
using byte `(i * 17 + 3) & 255`, and accumulates checksum `8355840`. The fixed
loop exists to establish deterministic execution and output retention, without
any clock. It performs no T81 execution, arbitrary allocation, descendants,
caller-selected input size or repeated trial loop. Runtime/loader/stack
overhead is OS-dependent and is not claimed to fit a 64 KiB total RSS budget.

Output goes to one parent-owned anonymous temporary file per live child;
the parent reads at most 4,097 bytes and rejects outputs exceeding 4,096.
The trusted fixture writes one small JSON record. The file is retained until
reap and verification and closed on success or rejection. Structured records
retain child output and accounting after closure. The fixed test suite has a
finite number of fixtures; no user-selected iteration count or campaign entry
point is provided. General hostile binaries and filesystem races are outside
this trusted local correctness instrument's scope.

## Primary interface references

- [Linux getrusage(2)](https://man7.org/linux/man-pages/man2/getrusage.2.html): selectors, units and aggregate semantics.
- [Linux wait4(2)](https://man7.org/linux/man-pages/man2/wait4.2.html): exact-child wait and associated usage; BSD interface, not a portable POSIX guarantee.
- [Apple XNU getrusage(2) source](https://github.com/apple/darwin-xnu/blob/main/bsd/man/man2/getrusage.2): macOS byte units.

The supported OS allowlist is narrower than generic POSIX. These references do
not constitute execution evidence; the retained receipts identify actual runs.
