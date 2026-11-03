# Phase C enforcement and process infrastructure — blocked for execution

This follow-up depends on **unmerged draft PR #682**, reviewed at
`c2279d69c3b700c9d53e6403065fc5cce5fd8f24`. The verified integration baseline is
`d8e1376a7a77902cc26269c6e8d4991bcde117a7` (Candidate A PR #679). The dependency
is carried explicitly; its presence here is not a merge or research acceptance.
Use the preregistration at
[`8e44c0f667c62e0d4143bb93169fc3235d5ebd66`](https://github.com/t81dev/t81-foundation/blob/8e44c0f667c62e0d4143bb93169fc3235d5ebd66/docs/research/continuity/H04_R03_CANDIDATE_A_PREREGISTRATION.md),
the actual PR diff, and the accepted Phase B code as source contracts.

The user supplied a summary of Jules' external report, *Pasted
text(20261102-210057).txt*, reviewing the #682 SHA above: **ACCEPT WITH CONDITIONS
— infrastructure increment only; Phase C execution blocked**. Its reported
conditions concern memory enforcement, process infrastructure, retention,
xxHash64 and host qualification. The full report bytes were not provided here.
This is supporting, user-reported review material; no GitHub publication,
report-content hash, independent reconciliation or independent reproduction is
claimed. Codex authored #682 and this follow-up, so these tests are author
validation, not independent scientific review. Model identity is unrecorded.

## Demonstrated scope and unresolved production requirements

| Mechanism | Implementation and tested scope | Production limit |
| --- | --- | --- |
| Kernel memory guard | Read/verify dedicated cgroup v2 controls; retain current/peak usage and OOM counters | Tested with synthetic control files, not a Linux kernel. Accounted memory is not exact RSS; launch-time accounting unresolved |
| Scratch reservations | Fsync reservation/refusal before allocation; account live reservations and historical peak | Only explicitly routed allocations covered; current codec/interpreter coverage incomplete |
| Scratch tracing | Detect a freed transient CPython allocation exceeding 64 KiB | Traced domains only; observes a peak after allocation, does not prevent it; native coverage incomplete |
| Process apparatus | 30 paired blocks, four arms, fresh isolated process protocol, durable starts/results and read-only recovery | Full schedule tested with synthetic process transport; real diagnostic workers tested individually. Codec measurement worker remains frozen |
| xxHash64 | Exact package/source/library/seed pins, upstream known-answer vectors, separate full-workload witness | Host binary/build identity and witness still require independent verification before execution |
| Retention | Weak-reference alias detection and pinned C11 ordering-fence binary | Ordering and dead objects do not prove allocator-page reclamation, cache eviction or physical erasure |
| Cleanup | Parent waits/reaps worker and retains status | Shared pages and file caches can remain; no physical-reclamation inference |

**Exact continuous RSS enforcement and complete native scratch enforcement are
UNRESOLVED.** This increment does not simulate compliance: real adapters never
emit `continuous_enforcement: true`. `KernelMemoryGuard.enter()` rejects use as
the engine's exact RSS backend. Host configuration matching is reported separately
as `configuration_matches`; `qualified` stays false with explicit unresolved
requirements. Evidence strings and booleans supplied by an operator do not
establish missing backend coverage.

## Kernel accounting and transient evidence

An operator must separately provision a new, empty, non-symlink cgroup v2 leaf
under `/sys/fs/cgroup`, with no descendants, `memory.max=33554432`,
`memory.swap.max=0`, `memory.oom.group=1`, `pids.max=1`, and a fresh peak/event
history. The reader does not mount, create, migrate, change or reset cgroups.
Missing controls, previous OOM events, inconsistent peaks and overshoot fail
checks. Kernel raw controls and local OOM counters remain readable by the parent
after a worker has died and can be appended to the parent journal. No code
resets `memory.peak` and hides an earlier failure.

The [kernel cgroup documentation](https://www.kernel.org/doc/html/v6.14/admin-guide/cgroup-v2.html)
defines accounted memory and its historical peak, allows temporary `memory.max`
overshoot, and explains that process migration does not move previous charges.
These properties prevent treating a verified cgroup configuration as exact,
immediate 32 MiB RSS enforcement. A future launcher needs verified accounting
from process creation, such as a reviewed native launch mechanism, and shared
page/startup coverage. No attach-after-exec workaround is supplied.

`ScratchReservations` is a pre-allocation budget interface: reserve a unique
identity before allocating, release only after actual freeing, and reserve both
old/new live storage during copying reallocation. Oversized reservations are
refused and durably retained before the caller can allocate. It covers requested
client bytes, not allocator headers, its own bookkeeping or unregistered calls.
Those gaps preclude using it as the Phase 6 backend today.

`TracedScratch` owns a tracing session and retains the historical traced peak,
even if a large temporary object has already been freed. Its records explicitly
say that native coverage and pre-allocation enforcement are absent. Python's
[memory documentation](https://docs.python.org/3.14/c-api/memory.html) explains
why C allocations can escape the Python memory manager. Full allocator-domain
and native allocation coverage, overhead charging and refusal semantics need
separate validation. Periodic RSS polling and `RLIMIT_RSS` are not substitutes.

## Process plan, isolation, failures and recovery

`make_plan(seed, commit)` freezes 120 ordered slots: 30 blocks of four unique
arms in the existing `paired_orders` permutations. The uint64 order seed must
be prospectively pinned; tests use 73 without adopting it as an approved campaign
seed. Dataset/query seeds, workload generation, hashes, codecs, sample counts,
bootstrap definitions and decision thresholds are unchanged.

`run_process_plan` unconditionally reaches the execution freeze before launching
anything. The only runnable path is `execute_synthetic_plan`, which invokes the
fixed `synthetic_worker.py` diagnostic using the current Python executable with
`-I -S -B`, a minimal environment, closed inherited descriptors, separate working
directory, and a new POSIX session/process group. The worker has no package or
codec imports and accepts only `ok`, `nonzero`, `kill`, `malformed` and `wait`
fault scenarios. It accepts no commands, paths, modules, workload overrides or
authorization flags. The wait action is timeout fault injection, not timing data.
These mechanisms are process isolation for trusted diagnostics, not a malicious
code sandbox or host cache/core isolation.

Each slot runs once, including failures; no replacement runs or favorable-case
filtering occur. The parent fsyncs the plan and attempt start before launching.
The child waits for a request while the parent records birth identity. Linux
identity includes boot ID, PID and `/proc/PID/stat` start ticks; the parser handles
spaces/parentheses in `comm`. A separate per-attempt nonce binds output to its
request. On macOS there is no Linux birth-identity proof; records explicitly use
synthetic nonce scope. OS start identity is required for future Linux qualification.

Stdout/stderr are retained in separate files, flushed and hashed by the parent.
Nonzero exits, signals, malformed output, spawn failures and timeouts produce
failure records. Timeout cleanup kills the worker process group and reaps it.
Even a self-killed diagnostic retains its previously flushed output. No elapsed
experiment observations are recorded. The fixed worker produces bounded output;
this is not a reviewed arbitrary-worker output quota mechanism.

The exclusive, append-only parent journal fsyncs every hash-chained record.
Directory-entry persistence precedes child launch; a durability failure stops
further spawning. `recover_journal` never rewrites a damaged file: it returns the
valid prefix, incomplete attempts and exact unparsed tail. A parent crash can
leave an incomplete attempt; it is never reported as success. Semantic validation
checks schedule chronology, unique nonces, Linux birth-identity reuse, terminal
counts and worker identity. Artifact verification checks parent-derived file
hashes. Hashes establish content identity, not truthful execution, authenticity,
authorization or durability on every filesystem/controller.

## Pinned checksum and witness

The production checksum wrapper requires:

- Python distribution `xxhash==3.5.0` and bundled xxHash library `0.8.2`.
- Source tarball SHA-256
  `84f2caddf951c9cbf8dc2e22a89d4ccf5d86391ac6418fe81e3c67d0cf60b45f`.
- XXH64 seed **0**; checksum input is signed little-endian 64-bit values in
  full-dataset order, preserving #682's Phase 7 interpretation.
- The upstream pinned tests' `a` vectors at seeds 0, 1 and uint64 maximum, plus
  the pinned README's `xxhash` seed-0 vector. Both package and library versions
  and known answers are checked before returning the factory.

Sources: [release metadata](https://pypi.org/pypi/xxhash/3.5.0/json),
[pinned known-answer tests](https://github.com/ifduyue/python-xxhash/blob/v3.5.0/tests/test_xxh64.py),
[pinned algorithm specification](https://github.com/Cyan4973/xxHash/blob/v0.8.2/doc/xxhash_spec.md).
The backend records the compiled extension's SHA-256. Dependency/build installation
is explicit, outside trial lifecycles:

```sh
python3 -m venv /private/tmp/t81-h04-enforcement-venv
/private/tmp/t81-h04-enforcement-venv/bin/python -m pip install \
  --no-binary=xxhash --require-hashes \
  -r experiments/h04_r03_matched_codec/requirements-measurement.txt
```

The source dependency is hash pinned; build isolation tools, compiler, flags,
Python build and resulting extension binary still need host-specific qualification.
No automatic network installation occurs on import or during a trial.

`checksum_witness.py` implements a separate one-shot XXH64 formulation and a
separate transcription of the frozen generator; it imports neither candidate
codecs, workload generator nor the production checksum wrapper. It reproduces
the canonical signed-int16 dataset hash, then derives the Phase 7 checksum from
signed-int64 serialization. `checksum_witness.v1.json` pins its source hash and
result. The full-workload result is **`cb66754878debc38`**, with output SHA-256
`603c0634f39ac12e6951d1d2365f123be73b71a3551f7dfaab7e7de8c1b20090`.
The test checks that result again using the existing workload generator and the
pinned C streaming backend. These are untimed correctness derivations, not codec
performance trials. Source separation does not establish independent authorship
or independent laboratory reproduction. Seed/output conventions need human
review before execution; this does not amend the preregistration.

## Retention and reclamation

`OwnedBuffer` permits weak references to raw data; mapped arrays already support
them. The lifecycle can construct `RetentionBarrier` after Phase 1. Phase 4 drops
its strong owners, runs collection, invokes the hash-pinned native fence and
rejects remaining aliases. The native helper uses
`atomic_thread_fence(memory_order_seq_cst)`. The tested ARM64 binary disassembles
to `dmb ish; ret`; Linux x86_64 instruction/ABI verification remains pending.
Compile for the host explicitly, retain compiler/flags, and pin the binary hash:

```sh
cc -std=c11 -Wall -Wextra -Werror -fPIC -shared \
  experiments/h04_r03_matched_codec/retention_fence.c -o /private/tmp/h04-fence.so
```

On macOS the test uses `-dynamiclib` and a `.dylib`. No allocator trimming,
cache flushing, data erasure or resident-page return is inferred. Reaping a
process verifies that its address space has gone, not that shared/file-cache
pages have disappeared. Receipts retain `None`/false for unverified physical or
allocator claims. The stronger preregistered residency/reclamation condition
remains unresolved on a qualified host.

## Freeze and remaining requirements

No implicit real-clock instrumentation is available: `_instrument` requires an
explicit synthetic clock, rejects direct built-in measurement clocks, and labels
its records SYNTHETIC. Every non-synthetic artifact is INVALID while frozen;
invalid host/order/freeze checks precede statistical analysis. These are application
controls in a trusted research checkout, not protection against someone rewriting
Python or wrapping a clock manually.

Before any authorized execution:

1. Resolve exact continuous RSS enforcement, startup/shared-page accounting,
   full scratch allocator/native coverage and transient refusal evidence without
   relaxing 32 MiB RSS or 64 KiB scratch. Independently review kernel receipts.
2. Verify production process creation/accounting, process identity, bounded output
   retention, killed-worker evidence and recovery on the selected Linux filesystem.
   Connect the production worker only through a separately reviewed freeze lift.
3. Review checksum seed/output conventions, reproduce the witness independently,
   and pin the host Python, allocator, build tools, flags and native binary identities.
4. Demonstrate the required retention/residency and reclamation contract on that
   host, including allocator/shared-page limits. Ordering fences alone do not suffice.
5. Qualify one dedicated Linux x86_64 LTS host: >=4 physical cores, approved fixed
   frequency/performance governor, turbo disabled, isolated affinity and SMT/cache
   sharing, complete L1/L2/L3 topology, disabled swap, <=16 GiB allocation, reviewed
   inter-process cache reset and environment pins. A matching synthetic snapshot
   is not host qualification. Python/interpreter overhead may itself exceed 32 MiB;
   that would block the configuration, not authorize a larger cap.
6. Reconcile the historical preregistration pin and external review through human
   review. Obtain explicit execution authorization and a separately reviewed freeze
   lift. Neither this draft, tests, hashes, booleans nor the external assessment
   authorizes measurements or merges.

No performance, production-workload applicability, radix-exclusive or hardware
advantage is claimed.

## Validation receipt and changed-file inventory — 2026-11-02

Validation host: macOS 26.5.2 ARM64, Python 3.14.3. The native fence was compiled
by Apple clang 21.0.0 (`clang-2100.1.1.101`), target
`arm64-apple-darwin25.5.0`. The checksum extension records SHA-256
`d3a80d6f7dfd6d9e94462cfd0c501d413be6a189290d23c927add9e027bc07d3`.
No Linux cgroup kernel enforcement, Linux x86_64 launch/accounting, Windows
execution, host isolation, power-loss durability or physical reclamation was
validated. POSIX process/journal paths are unsupported on Windows; no fallback
claims qualification. Linux ARM64 remains outside the registered execution host
profile even though pure deterministic checks can be run there.

Commands used after dependency setup (run from the repository root):

```sh
/private/tmp/t81-h04-enforcement-venv/bin/python -B -m unittest \
  experiments.h04_r03_matched_codec.test_production_infrastructure -v
/private/tmp/t81-h04-enforcement-venv/bin/python -B -m unittest discover \
  -s experiments/h04_r03_matched_codec -v
git diff --check
/private/tmp/t81-h04-enforcement-venv/bin/python -B tools/research_commons/commons.py \
  --request '{"protocol":"t81.research-commons.v1","operation":"get_hypothesis","arguments":{"hypothesis_id":"H04"}}'
```

The initial targeted slice passed 19 tests. After further failure/integrity checks,
the final targeted slice passed **20 tests** and final discovery passed **73 tests**,
with zero failures, errors or skips. An intermediate full run also passed 73.
`git diff --check` passed; Commons discovery returned success after explicitly
installing `jsonschema==4.23.0` outside the repository. Its returned dated evidence
scope is not substituted for current repository state.

Raw final logs remain locally available:

| File | SHA-256 |
| --- | --- |
| `/private/tmp/h04-enforcement-final.log` | `414b2441a3f99790142f418479e314d959de430ebd0d8ee86af5999ef3d91772` |
| `/private/tmp/h04-enforcement-targeted-final.log` | `80651853db7e1fe186677dd19d07e631aa3666f19f3f81f5d7f4db451a22158e` |

The tests execute tiny codec correctness fixtures with synthetic clocks, pure
checksum derivations, synthetic allocation faults and a fixed diagnostic child.
The 120-slot execution path uses a simulated process transport; individual
success/nonzero/signal/malformed/timeout cases use real diagnostic child processes.
No codec benchmark, performance sample, Phase C campaign or timing comparison
was executed. Test-runner duration summaries are ordinary validation metadata.

The exact follow-up inventory, relative to PR #682, is entirely under
`experiments/h04_r03_matched_codec/`:

| File | Change |
| --- | --- |
| `measurement_engine.py` | Default pinned checksum and retention integration; synthetic-only instrumentation; qualification/analysis guards |
| `test_measurement_engine.py` | Distinguish configuration matching from actual qualification |
| `MEASUREMENT_ENGINE.md` | Link current follow-up status without rewriting the earlier validation receipt |
| `checksum.py` | Checked production XXH64 backend and source/witness identity checks |
| `checksum_witness.py` | Source-separated untimed generator/hash oracle |
| `checksum_witness.v1.json` | Full canonical correctness witness and source identity |
| `requirements-measurement.txt` | Exact checksum source-distribution hash pin |
| `enforcement.py` | Kernel observations, explicit scratch reservations and transient tracing with coverage limits |
| `process_control.py` | Ordered process plan, diagnostic isolation, durable failure journal and integrity/recovery checks |
| `synthetic_worker.py` | Fixed diagnostic fault worker with no codec or timing dispatch |
| `retention.py` | Owned-reference alias verification, pinned fence loader and bounded reclamation claims |
| `retention_fence.c` | C11 ordering fence |
| `test_production_infrastructure.py` | Deterministic and synthetic-fault validation |
| `ENFORCEMENT_AND_PROCESS.md` | Contracts, limits, provenance, qualification procedure and validation receipt |

No existing codec, oracle, workload, statistical threshold, governance, CI or
historical evidence file was changed by this follow-up. This remains a draft
infrastructure implementation with unresolved production requirements, not a
claim that the experiment is ready to execute.
