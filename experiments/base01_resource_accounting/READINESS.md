# WL-RES-RUSAGE-METRICS engineering readiness

**RESOURCE ACCOUNTING ENGINEERING COMPLETE — AWAITING QUALIFICATION**

This disposition covers a bounded fixture collector. It grants no B1
measurement readiness, execution authorization or benchmark-repeatability claim.

| Execution environment | Compiler | Final correctness results |
| --- | --- | --- |
| Linux x86_64, Alpine 3.21.9, kernel 6.12.114-0-virt; fresh diskless QEMU 10.2.2 TCG guest on macOS ARM64, 1 vCPU / 768 MiB configured | GCC 14.2.0, x86_64-alpine-linux-musl | 27 passed, 0 failed, 0 skipped |
| macOS ARM64, Darwin 25.5.0 | Apple Clang 21.0.0, arm64-apple-darwin25.5.0 | 27 passed, 0 failed, 0 skipped |
| Windows / other OS | Unsupported | No execution claimed; rejection contracts tested synthetically |

The Linux evidence comes from a real Linux kernel and userspace, executing an
x86_64 fixture under CPU emulation. It is not synthetic Linux output, native
x86 hardware validation, a qualified measurement host, or comparative evidence.
QEMU boot/package provisioning and SSH transport are environment setup only.
No old research scripts from the directories supplying the stock Alpine boot
assets were executed. The guest was shut down after retrieving evidence.

## Evidence and tests

`evidence/linux.json` and `evidence/macos.json` retain exact CLI/compile commands,
implementation file hashes, executable hashes, compiler identification, APIs,
test logs, raw resource structures, wait statuses, parent/child identities,
and explicit empty performance sample arrays. Linux distribution/kernel/CPU
metadata are included. `linux-environment.txt` adds installed package versions,
compiler target and memory configuration. `provenance.json` binds the tested
implementation commit, source/foundation tree identities, QEMU invocation and
stock boot artifact hashes. `SHA256SUMS.json` inventories the supplement's
files; it intentionally excludes its own hash to avoid a self-reference.
Hashes establish byte identity, not independent validation or authenticity.

The 27 methods cover successful exit; nonzero exit and signal rejection;
distinct sequential PIDs; exact positive-PID wait arguments; foreign ownership,
wrong parent and repeated reap rejection; external reaping/ECHILD; real parent
versus child attribution; invalid selectors; Linux/macOS RSS units; unsupported
platforms/APIs; cross-platform override rejection; binary/source identity;
nonfinite/negative/missing samples; malformed return tuples and child reports;
deterministic checksum/size; and Python/native measurement-request rejection.
Subtests are included within these method totals, not counted as extra tests.

Final receipts contain 19 collector records each: eight accepted records and
11 expected rejections. Rejections include real exit 7, real SIGTERM and real
ECHILD, plus eight explicitly labeled synthetic corruptions after actual
reaping. The externally reaped child is intentionally absent from successful
collector accounting. Mocked missing APIs/selectors/units are contract tests,
not additional platform execution. Expected negative cases are passing tests,
not test-suite failures. No OS counter is invented to force acceptance.

## Development failures and evidence limits

The first macOS compile failed because `_POSIX_C_SOURCE` hides Darwin's extended
`struct rusage` fields. `evidence/macos-initial-build-failure.json` retains that
unaltered failed receipt: compilation failed, zero test methods executed.
Its historical implementation hashes describe that earlier working snapshot,
not the final implementation. `_DARWIN_C_SOURCE` fixes the declaration without
changing accounting units or workload code.

An intermediate test-runner attempt also failed with `StringIO` missing
`writeln`; no complete receipt was produced for that aborted attempt. A minimal
log adapter fixed it. Initial correctness runs used unittest's internal duration
bookkeeping; no performance data or duration-based acceptance was produced.
The final runner explicitly disables that bookkeeping and retains no durations.
These development failures are not replaced with fabricated passing evidence.

No broad/root CI, T81 correctness executable, B1 workload, PMU or benchmark job
was launched. No Phase A/B0/#691 evidence was regenerated or altered. The branch
adds only `experiments/base01_resource_accounting/`. CI skip markers are used
because existing repository workflows can execute benchmarks; workflow files
are unchanged. Hosted check observations are reported separately at PR creation.

## Remaining prerequisites

There is no unresolved blocker for the scoped fixture collector on the two
executed environments. B1 host qualification, bare-metal Linux qualification,
workload equivalence, repeatability and any performance authorization remain
separate, incomplete work. No dispatcher, signed-permit activation, timeout
policy, process-tree collector or arbitrary child-command facility is provided.
Concurrent external reapers or unexpected OS errors fail closed and require
operator diagnosis. Generalization to arbitrary VM execution or energy/hardware
measurement is unsupported. Other workloads' #691 readiness classifications
remain historical and unchanged.
