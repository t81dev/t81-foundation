# BASE-01 B1 proposed contract supplement

**PROPOSED — NOT APPROVED. No execution or resource authorization.**

Target: B1 design `64c3f71ff100ea16270f1a70497ac39860c1b772`, as preserved in
main `a98b22fb65c80f4225151cbcd5c6934ae076cbe1`. Interpret alongside the
[source-pinned report](REPORT.md). Original accepted/historical bytes stay intact.
Acceptance of this draft PR alone must not be construed as a measurement permit.

## Minimal clarification proposed for acceptance

Replace the ambiguous interaction of CanonFS first-write fraction and count
doubling with this supplemental rule, **only after explicit maintainer approval**:

> For each size-specific write trial, perform exactly 100 timed successful
> writes of the fixed payload to one newly constructed, initially empty in-memory
> driver. Only write 1 starts with the key absent; writes 2–100 replace the same
> key. The first-insertion fraction is defined per trial as absent-key timed
> writes divided by all completed timed writes: exactly 1/100. Warm-up, correctness
> checks and read preloading do not enter this count. Keep all returned references
> through post-interval verification. Do not double this cell's count. A failure
> of duration/overhead qualification leaves the cell inconclusive/blocked and
> requires a separate protocol amendment; do not add restarts or remove insertion.

The read retention contradiction remains a qualification blocker until a human
chooses a lifetime/resource design. No implementation may pick an implicit fix.

## Conditional candidate B for maintainer design decision

This is a concrete alternative, **not a recommendation of scientific equivalence**.
It would supersede B1's all-100-live and single-whole-batch read interval clauses.
Choose it only if bounded chunk allocation is the intended scientific quantity:

> For each size-specific read trial, use one fresh in-memory driver preloaded
> with the fixed object outside intervals. Perform exactly 100 verified allocating
> `read_object_bytes` calls as ten ordered chunks of ten, on that same driver.
> Preconstruct ten empty caller-owned vector slots outside each interval.
> Each interval includes all ten public API calls and moves into those slots.
> Do not reuse an already allocated destination to replace the allocating API.
> Retain all ten results until the interval ends. Verify success, length, expected
> hash and every byte of every output outside timing, then destroy all ten before
> starting the next chunk. No result payload or serialized copy survives into
> the next chunk; preserve verification records, identities and failure details.
> Never hide a per-read free or an overwritten live vector inside the interval.
> Stop immediately on failure and retain an incomplete trial.
>
> Keep allocation/copy/hash work intrinsic to the API inside intervals. Keep
> construction, verification, destruction and serialization outside. Allocator
> caches and verification-induced cache state can carry between chunks; register
> that profile explicitly. Do not reset the store, disable verification, change
> bytes, bypass hashing, or call a new allocation-free API. Warm-up remains five
> operations on separate disposable state and needs its own authorization.
>
> At each point let R be all live caller-owned read-result vectors, including
> any not-yet-moved successful return. Count the return-to-slot move as ownership
> transfer, never as permission for an eleventh live payload. Require |R| <=10
> and accounted retained output <=16,777,216 bytes. Account vector capacity in
> bytes plus output-container storage without double-counting transferred payload;
> include any other simultaneously retained output storage. Budget admission must
> include the next result before its call. An implementation/platform without a
> defensible next-allocation bound is ineligible; a post-call capacity check alone
> is detection, not hard prevention. Reject capacity overshoot, extra retained
> copies or unsupported ABI. Do not treat allocator caches as live output or call
> this accounting a process RSS cap. Inventory API transient allocations, store,
> parity, input and evidence buffers separately against their applicable budgets.
>
> Retain ten raw integer intervals T_k and counts N_k=10; N=sum(N_k)=100 and
> T=sum(T_k). Compute decimal MB/s as S*N*1000/T and microseconds/block as
> T/(1000*N). Do not average chunk rates or count chunks as trials. Timer-pair
> count B=10 must satisfy the unchanged overhead/duration rules; no overhead
> subtraction. All 30 fresh-child trials use this identical fixed plan. CanonFS
> reads as well as writes are exempt from count doubling under this candidate;
> a qualification failure requires another amendment, not a favorable batch.

The largest retained payload is 10,485,760 bytes (10 MiB). The 6 MiB difference
from the ceiling is accounting headroom, not permission to ignore capacity,
containers or transients, and not a measured RSS prediction. Apply the same chunk
schedule at all three payload sizes to avoid an implicit size-dependent profile.

If selected, register a distinct approved protocol/profile digest covering this
supplement, the unchanged base design and exact accounting/boundary rules. The
runner, host report, qualification evidence and permit must bind that identity.
Candidate B cannot be pooled with all-live or per-read-release measurements.
Implement and verify the [acceptance tests](READINESS.md) before asking to qualify.

## Preserved controls

No change to measured source `cda9135d36144d4de2771af174e52ac7b30a83c9`, payloads,
hash oracles, public APIs, six VM builders, R75 software-count interpretation,
14-cell scope, 30 fresh-child trials, warm-up definitions, statistical thresholds,
host controls, source/artifact identity, raw evidence/failure retention, signed
permits, revocation, replay protection, launcher/worker checks or time/evidence/RSS
ceilings. Ordinary builds/CI stay unconditionally measurement-disabled until a
separately reviewed activation change and applicable human permits exist.
This proposal changes no governance activation, protected contract or frozen
experiment. No qualification/campaign permit is granted, requested or generated.
