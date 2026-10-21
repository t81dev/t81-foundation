# Concurrent authorization and revocation experiment

Baseline: main after merged PR #538,
`dec74cfa4d7be321e0c28adb276a8dc2d278c502`.

**Decision:** explicit ownership/synchronization is necessary for the stronger
completed-revocation guarantee defined below. For one trusted process and one
ownership domain, the minimum tested implementation is a caller-owned mutex held
across authority evaluation and the complete driver operation; revocation uses
the same mutex. This is already consistent with the production API's requirement
for external serialization. No violation of that existing contract is demonstrated,
so this PR adds an experiment and reference ownership pattern, not a production
lock, epoch service, transaction subsystem, or new revocation API.

## Actual production contract and audit

PR #538's `Driver::set_axion_hook` comment requires external serialization of
operations and callback replacement and expressly excludes concurrent revocation
or a transaction spanning policy evaluation and I/O. Its report makes the same
limitation explicit. Raw drivers are not general concurrent objects.

| Production surface | Authorization through mutation | State and synchronization |
| --- | --- | --- |
| Persistent | Hash payload, invoke hook, check capability mask, exclusive open/create, write bytes, close, update LRU/cache, return | Callback, capability cache, bootstrap flag, object cache, and LRU are ordinary mutable state without a mutex. Authorization precedes the existing-object fast path too. Successful return is not fsync/crash durability or atomic publication to outside readers. |
| In-memory | Invoke hook, hash, check capability, insert/assign object bytes, encode/store parity, return | Object/parity/capability maps and callback are unsynchronized. Reads can also mutate or depend on state. The insertion and parity steps are not a transaction. |
| Block-backed | Invoke hook for write/read, pack bytes, CanonStore put writes a data block, update index, flush index/device, return | Store index, allocation state, caps map, callback, and underlying device require serialized ownership. The test device is RAM, not real crash-safe hardware. Data/index publication is multi-step, and the driver does not turn a failed flush into a write error. No durability result is inferred here. |
| Text hook / PolicyEngine | Parse and capture policy, request final authorization, inspect current attempted-event strings, return verdict | Policy text is a fixed snapshot, not a live policy-file watcher. Copied callbacks share the captured engine. Engine loop-evidence caches and process-global trace/meta counter are mutable; shared_ptr lifetime safety does not make evaluation thread-safe. |

The protected path has an authorization decision and later storage mutations;
there is no shared linearization point joining those with an independent authority
owner. Exclusive file creation is a filesystem primitive, not an authorization
transaction. We do not treat an observed method return as proof of atomic storage
publication or crash durability.

### Three different meanings of revocation

1. **Callback replacement:** a trusted host may replace/clear the std::function.
   Replacing it concurrently with its invocation violates the raw API's contract
   and may race on the function/captured state. This experiment never does that.
   An empty callback retains explicitly ungoverned bootstrap behavior.
2. **Capability APIs:** persistent revocation removes a capability file and updates
   that instance's cache/flag; another instance's permission cache is not invalidated.
   In-memory revocation erases local state. Removing the last mask can restore
   bootstrap access. These are not a documented global authority-generation protocol.
3. **The experiment's revocable authority:** an explicit host-owned atomic active
   flag paired with the fixed policy callback. The revoker changes only this flag,
   not the Driver, callback, captured PolicyEngine, or trace. This is a defined
   operational model, not a claim that the Axion text DSL already has live revocation.

An additional audit correction matters: block-backed `publish_capability`,
`revoke_capability`, and `parity_repair_subtree` do **not** invoke axion_allow, and
block-backed reads/writes do not consult its caps map. PR #538's report generalized
all five callback sites from the other drivers; the current source only establishes
block-backed write/read interception. We document this discrepancy rather than
pretending those APIs provide equivalent revocation semantics or opportunistically
expanding their implementation in this write-concurrency experiment.

## Exact operational models

Both models have one writer and one revoker, a fresh store, no competing reader,
and no direct raw-driver mutation outside the owner. Inspection occurs after both
threads join. All callbacks are installed before either actor starts. The T81
callback combines a real production Axion hook requiring the current Write event
with the experiment-owned live permission; the conventional callback uses the
same live-permission predicate and no T81 policy engine. The static policy is
never represented as dynamically revoked.

**Snapshot model:** authorization samples active permission once. A request that
samples Allow can finish after the authority flag changes. This model provides
no stronger promise for the in-flight request. It is a data-race-free probe of the
current authorization-to-mutation interval: only one thread invokes a Driver or
Axion; the second thread updates an atomic host-owned flag.

**Serialized-owner model:** writer and revoker share one non-recursive mutex.
The writer holds it from before evaluating authorization through return from the
actual protected write. Revocation acquires that same lock, changes the flag, then
returns. Revocation linearizes at the flag change under the lock. A successful
write's logical commit is recorded after the driver returns, still under the lock;
physical mutations necessarily precede that record and occur inside ownership.
A request waiting behind an already-linearized revocation is denied.

The stronger requirement is: **after revocation completes, an old authorization
must not begin a later protected storage mutation, and subsequent new requests
must be denied**. Concurrent overlapping operations are ordered by ownership,
not by which invocation began first. In-flight writes may complete while revoke
is waiting for ownership, before revoke linearizes/returns. This is draining,
not cancellation on revocation invocation. We do not silently substitute the
stronger “a revocation request must preempt an existing owner” requirement.

For files and volatile maps, the callback pause proves whether a mutation can
start after revocation: the thread cannot leave its callback until released.
Logical commit is observed through the actual write result and readback/files.
Only block endpoints expose a second, exact final device-write checkpoint. We do
not claim an OS-open or map-insertion test seam that the public APIs do not expose.

A zero-violation result for serialized ownership does not prove these raw drivers
have become independently thread-safe. All actors sharing the same driver,
capability state, or global Axion trace must participate in the ownership contract.

## Deterministic schedules

Two actual std::threads are controlled by condition-variable predicates and
explicit pause/release handshakes. No random scheduling or sleeps select outcomes.
A ten-second watchdog aborts a broken schedule; expiry is never a successful test
condition. The controller never acquires the authority mutex. A failed try_lock
records contention before the controller releases the writer/revoker, so the
serialized case does not deadlock by waiting for revoke completion while holding
write ownership.

| Schedule | Controlled pause/order | Allowed result |
| --- | --- | --- |
| no_revocation | Writer paused before authorization; no revoke issued | Correct authorized write |
| before_authorization | Revoker completes and releases ownership before writer proceeds | Denial, no storage effect |
| after_authorization | Writer pauses inside the actual callback after computing Allow, before returning it | Snapshot can commit after revoke; serialized owner commits before revoke completes |
| during_revocation | Revoker pauses after flag change, before completion; writer attempts admission | Snapshot denies; serialized writer proves lock contention, waits, then denies |
| after_commit | Writer pauses after actual write success; revocation overlaps the remaining invocation | Existing commit remains valid; future new payload denied |
| at_mutation | Both block endpoints pause in the actual first write_block call before altering the device image | Snapshot can physically mutate after revoke; serialized owner drains before revocation completes |

There are six endpoints: conventional file/vector/RAM block and the real T81
persistent/in-memory/block-backed drivers. The three matched pairs share the
same authority predicate, ownership model, proposed result bytes, schedules, and
observed effect criteria. Conventional file writes have a single writer and fresh
store; no competing-creator or overwrite behavior is being compared. The block
pair shares an instrumented IBlockDevice but uses different storage formats;
there is no representation, footprint, or durability equivalence claim.

Every schedule additionally issues a new distinct payload after revoke completes
and requires rejection without any further file/device mutation. Inspection checks
that no future object was inserted in the T81 memory store. The actual memory
readback, file bytes, and first data-block image are recorded. All stores are fresh;
result directories cannot be reused.

## Observed results

Thirty fresh-store repetitions of 64 cases completed: **1,920 initial operations**
(plus post-revocation requests). Every case reproduced the same event trace and
storage state across its 30 repetitions. Timings are intentionally excluded from
trace equality. No watchdog expired.

| Endpoint family | Model | Initial operations | Counterexamples to stronger completed-revocation goal | Current serialized-contract violation demonstrated |
| --- | --- | --- | --- | --- |
| conventional | snapshot | 480 | 120 | No |
| conventional | serialized-owner | 480 | 0 | No |
| t81 | snapshot | 480 | 120 | No |
| t81 | serialized-owner | 480 | 0 | No |

**The shared-owner cases recorded zero prohibited commits in 960 operations.**
The 240 snapshot counterexamples are evidence against a proposed stronger
promise, not evidence that PR #538 violated its documented serialized contract.
Both implementations behave the same under equivalent ownership semantics.
All requests ordered after completed revocation were denied; all no-revocation
writes and writes owned before the revoker could linearize returned the expected
bytes. Denials had no result files or device writes.

Two representative adversarial orders (the JSON includes every full trace):

```text
snapshot / after_authorization:
  authorization.allow -> pause.authorization -> revoke.linearized
  -> revoke.complete -> release.authorization -> write.commit

serialized-owner / after_authorization:
  writer.lock_acquired -> authorization.allow -> pause.authorization
  -> revoker.lock_busy -> release.authorization -> write.commit
  -> revoker.lock_acquired -> revoke.linearized -> revoke.complete
```

The block at_mutation cases additionally record revoke.complete before
storage.first_mutation in snapshot mode, proving a physical device-write
counterexample after the authorization callback has returned. The serialized
cases record the physical mutation and write completion before revocation.

### Descriptive costs

| No-revocation writer interval (30 repetitions), microseconds | Snapshot median [min,max] | Shared owner median [min,max] |
| --- | --- | --- |
| conventional-file | 148.166 [125.500,434.959] | 128.875 [121.333,287.667] |
| t81-file | 141.041 [125.708,193.459] | 128.708 [119.375,291.041] |
| conventional-memory | 0.416 [0.291,0.709] | 0.459 [0.333,10.750] |
| t81-memory | 3.375 [2.917,6.375] | 3.146 [2.750,16.375] |
| conventional-block | 0.500 [0.375,1.125] | 0.583 [0.417,1.542] |
| t81-block | 6.292 [5.584,8.375] | 5.875 [5.542,6.667] |

On this platform sizeof(std::mutex) is 64 bytes and the test authority object is 72 bytes.
These are native object sizes, not total process memory. The reference adds
one ownership acquisition/release per request and revocation. It adds no storage
writes: the same endpoint emitted the same write/flush counts with and without
ownership for permitted commits. It holds ownership across I/O, so revocation
can wait for slow storage or callbacks.

Timings include scheduling, policy, storage, cache/parity work, and controlled
waits. The no-revocation interval excludes thread creation and the initial
controller pause but includes mutex acquisition and write return. No CPU isolation or randomized
trial order was imposed; raw samples and standard deviations are retained. Differences are
not a causal measurement of mutex overhead or cross-backend performance.


## Alternatives and minimum implementation

| Candidate | Atomicity / stale authorization | Cost and limitation |
| --- | --- | --- |
| Tag authorization with an epoch | Detects stale evidence but cannot by itself stop a write between a check and mutation | Needs a commit protocol binding the generation to storage ownership; an integer alone is insufficient |
| Recheck immediately before mutation | Narrows the interval; a revoker can still act after the last load | The controlled pause after the last authority decision illustrates why another unsynchronized check is not an atomic contract |
| Shared ownership around authority plus operation | Minimum reference tested here; orders a complete write before or after revocation | Holds the lock over hashing, policy callbacks, I/O, cache updates, and flush; contending revoke waits for an in-flight operation |
| Epoch admission plus in-flight drain/CAS protocol | Could allow more concurrency with a defined revocation barrier | Additional state, retry/ABA/lifetime and failure reasoning; not necessary to establish this bounded guarantee |
| Transactional service / storage commit | May address multi-process ownership or durable publication | Larger recovery/durability contract; not justified by this experiment |

The lock reference belongs in the trusted owner, not automatically inside every
Driver method. It must cover all entry points sharing mutable state, not just
write callbacks. Callbacks must be bounded and must not reenter the same owner or
wait for a revoker that needs that lock. A recursive mutex would not establish
safe recursive operation semantics. Lock ordering with application locks and
cancellation remains a caller responsibility. With process-global Axion trace
state, independent per-driver locks are insufficient for simultaneous hook
execution: those hooks also need shared serialization, or a separately justified
trace ownership change. This probe runs one Driver/hook at a time.

Independent processes or drivers using the same persistent root cannot obtain
this guarantee from unrelated std::mutex instances. They need a shared owner or
an explicit inter-process protocol and storage-visibility contract. We neither
execute unsupported racing set_axion_hook/revoke_capability calls nor claim to
have repaired their thread safety.

## Reproduce and independently inspect

Validated development environment and fingerprints are in concurrent-summary.json.
The build retains PR #538's Release C++23 and deterministic flags. It uses existing
C++/Python infrastructure, standard threads/condition variables, and the in-tree
CanonStore. No framework or package is added. The new option defaults OFF.

```sh
cmake -S . -B /tmp/t81-concurrent-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON \
  -DT81_BUILD_ACCOUNTABLE_EXPERIMENT=ON -DT81_BUILD_CONCURRENT_EXPERIMENT=ON \
  -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_TUI=OFF \
  -DT81_ENABLE_TERNARYOS=OFF -DT81_BUILD_FUZZ_TESTS=OFF
cmake --build /tmp/t81-concurrent-build --target t81_concurrent_revocation -j 6
ctest --test-dir /tmp/t81-concurrent-build --output-on-failure \
  -R '^canonfs_concurrent_revocation_experiment$'
python3 examples/governance/accountable-computation/measure_concurrent.py \
  --runner /tmp/t81-concurrent-build/t81_concurrent_revocation \
  --output /tmp/t81-concurrent-measured --repetitions 30
python3 examples/governance/accountable-computation/verify_concurrent.py \
  /tmp/t81-concurrent-measured/results.json --artifacts /tmp/t81-concurrent-measured
```

Use fresh paths. Local configuration reused the original pinned Asio test checkout
via FETCHCONTENT_SOURCE_DIR_ASIO for an offline build; it did not use build-bench/.
The separate checker imports no producer code. It checks case coverage, event
partial orders, actual filesystem bytes for every repetition when artifacts are
available, returned/readback consistency, physical block data, and the permitted
and prohibited outcomes for each model. Tests challenge reordered events, missing
cases/files, wrong result bytes, tampered filesystem artifacts, and store reuse.

concurrent-results.json preserves each case's first trace/state and all 30 writer
timings. The native harness requires identical trace/state for every repetition.
The live result directory retains every persistent artifact. For volatile stores,
readback/block images are trusted harness observations, not reconstructible proof
of RAM contents after process exit. The checker can validate the checked-in JSON
without the runtime; it does not authenticate a malicious producer or prove an
unreported write never occurred.

For the exact PR #538 regression slice:

```sh
cmake --build /tmp/t81-concurrent-build --target \
  $(cat examples/governance/accountable-computation/hardening-test-targets.txt) \
  t81_accountable_experiment -j 6
ctest --test-dir /tmp/t81-concurrent-build --output-on-failure \
  -R "^($(paste -sd '|' examples/governance/accountable-computation/hardening-test-targets.txt)|accountable_computation_experiment|canonfs_concurrent_revocation_experiment)$"
```

## Validation, uncertainty, and stopping criteria

All **50 selected tests passed** on macOS/ARM64: PR #538's 49-test native
VM/Axion/CanonFS and A/D evidence slice plus the new concurrency experiment.
The latter exercises 192 real operations per CTest run (three repetitions),
including evidence attacks and actual file tampering. The measured run separately
executes all 64 cases with 30 repetitions (1,920 initial operations). The detached
checker also validated every saved persistent artifact from that run. C++ format
and diff whitespace checks passed. Original #537/#538 fixtures and results are
unchanged.

At baseline main, remote AI Experiments, Format, QEMU x86_64, Speed Oracle, and
Pages runs passed. Documentation run
[39649283994](https://github.com/t81dev/t81-foundation/actions/runs/39649283994)
failed in setup-node before building the search index: no root npm lockfile exists.
The search dependencies are installed later inside docs/site/search, which also
has no checked-in lockfile. In accordance with the repository's CI-hygiene rule,
this PR removes that invalid cache setting only. Dependency versions, build steps,
and gates are preserved. The repair is separate from the concurrency result;
remote validation of this PR is pending. No production authorization repair is
inferred from that infrastructure failure.

No production source, CanonFS identity invariant, Axion evaluation contract, or
frozen TISC mapping is changed. The explicit existing external-serialization
contract is not silently upgraded. There is no production performance regression
claim because no production synchronization was inserted.

The portability review covers standard C++ thread/mutex/CV behavior and existing
POSIX/Windows persistent-driver paths. Only the available macOS/ARM64 development
platform is measured. Linux, Windows/MSVC, x86_64, crash recovery, device errors,
untrusted callbacks, multiprocess writers, lock fairness, callback reentrancy,
and global-trace concurrency remain unmeasured. The tests establish no
system-wide security result.

Stop before adding production mechanisms unless maintainers select the stronger
revocation semantics, ownership domain (including all trace/capability access),
callback reentrancy rules, and persistence/visibility boundary. If an application
can use one trusted owner, implement the tested external critical section. If
revocation must preempt an existing writer or span independent processes, specify
that stronger contract and design a separate falsifiable experiment first.
