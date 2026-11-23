# Retention and resource bounds — correctness profile only

These are output-lifetime contracts and logical/admission bounds. They are not
peak RSS measurements, hard process RSS enforcement, latency results or a
qualification of future larger B1 batches. No resource campaign was run.

## Arithmetic

The exact ordered 1,031-input corpus and independent B0 positional oracle are
preserved. Corpus identity:
`0fefb0e6b968e8c11556699700530d133505a0bac2013674a42203b10fcb54ef`.
It includes signed extrema/neighbors, zero and +/-1 plus 1,024 original LCG values;
there is no random seed change or distribution expansion.

`encode_all` calls the actual allocating `encode_i64` API into preconstructed
outer slots; it does not replace it with an allocation-free codec. Encoded vectors
are retained while canonicality is checked. `decode_all` receives those already
encoded vectors in a separate call and retains every integer result. Native
comparison and full serialization happen before any result is destroyed. The
serialized copies remain until independent Python verification and evidence
sealing, even after native containers are freed.

Each encoded value must have 1..41 trits, no redundant most-significant zero,
only -1/0/+1 digits, and capacity <=64. Decode must equal the exact original
integer. The Python oracle independently evaluates the positional integer, so a
shared encode/decode error that cancels in a roundtrip is insufficient to pass.
Three outer vector capacities must not exceed 1,031; the tested ABI uses one byte
per Trit and 24 bytes per vector object. Unsupported larger capacities reject.

| Accounted storage | Bound |
| --- | ---: |
| Retained encoded digit capacity | 1,031 * 64 = 65,984 bytes |
| Input and decoded integer payloads | 2 * 1,031 * 8 = 16,496 bytes |
| Encoded-vector object array | 1,031 * sizeof(vector<Trit>) |
| Accounted total on tested 24-byte vector ABI | At most 107,224 bytes |

The receipt records actual capacity accounting and sizeof values, not allocator
resident memory. It excludes top-level vector object headers, allocator headers,
fragmentation/caches, stack, runtime and evidence serialization. No total-process
memory claim follows. Allocator name/policy is whatever the pinned compiler/
standard-library build uses; it is not reset or locked. Destruction occurs after
native verification/serialization; encode-and-free cost is not measured here.
Future measurement counts/lifetimes need a separate reviewed bound.

## In-memory CanonFS

Sizes are exactly 4,096, 65,536 and 1,048,576 bytes, all 0x42. Each size uses a
fresh in-memory driver and a single repeated content key. Both successful write
references and both successful read vectors remain alive through byte comparison
and serialization. The two reads are returned copies, not references into storage.
Read verification is explicitly enabled; bootstrap mode has no installed hook or
capability table. This is correctness, not policy-overhead evidence.

The independent CanonHash expectation is SHA3-512's first 32 raw bytes, computed
by Python hashlib rather than any T81 library. This follows the pinned
`vm/tracing/canonhash81.cpp` contract. It does not use the Base-81 text encoder,
substitute SHA-256 for CanonHash, or claim hash identity establishes authenticity.
Full read bytes are independently compared again by Python after decompression.

For block size S, caller payload/read capacity is bounded by 3*S; two CanonRef
objects are fixed-size. The receipt records this client payload accounting.
Within the audited driver, one stored object has logical length S. For
q=ceil(S/3), ReedSolomonRepair's three data shards and two double-width parity
shards total 7*q logical bytes. Repeated replacement can transiently retain old
and new parity sets (14*q); a conservative known-buffer envelope is the larger
of `2*S+14*q` during replacement and `4*S+7*q` after reads. At the largest size
that envelope is 6,990,516 bytes, excluding object/allocator/runtime overhead.
These are source-derived logical lengths, not inaccessible private capacities
or an enforced heap/RSS cap. Buffers for different sizes do not coexist in native
validation; allocator caches may retain freed storage.

Hashing, copying, parity generation and intrinsic API allocation are not removed
or timed. No unique-object ingest, disk I/O, fsync, persistent/block driver,
policy-profile extension or repair workload is introduced. Evidence/build files
use the ordinary host filesystem; they are not a persistent CanonFS workload.

## VM and evidence/parser limits

Only the six historical builders are used, one fresh VM at a time. Flat memory
must equal `1,536 + program_instruction_count` words (maximum 1,552), matching
the pinned layout constants. Native assertions cap interpreter trace entries
at 512 and Axion log entries at 2,048. The receipt records actual logical counts.
These checks do not bound every allocation in arbitrary VM programs, generalize
R75, or qualify a future 16-machine pool. VM engineering remains provisional.

| Admission/check | Limit and scope |
| --- | --- |
| Native accepted stdout | 8 MiB before Python JSON loading; deterministic native data is below this. Writes are redirected to a file, not accumulated by launcher pipes. |
| Static artifact input | 128 MiB per file; reject before read/parse through the launcher. |
| Section/program/command counts | 4,096 entries, with aggregate Mach-O section count checked. |
| ELF section name | At most 4,096 bytes, with bounded table range/null termination. |
| Output directory | Must be newly created; existing directories are not overwritten. |

These are checked admission/logical bounds, not a watchdog, filesystem quota,
strict Python heap bound, or OS process-memory limit. The trusted native program
has fixed loops and bounded output; an arbitrary malicious executable or operator
is outside this foundation's trust model. The stdout cap is checked after child
exit and does not enforce a live disk-write quota. Actual RSS/CPU-time collection,
resource enforcement, timeouts and measurement budgets are not implemented.

Resource fixtures validate Linux raw KiB versus macOS raw bytes/1,024, preserving
fractional KiB. Negative/nonfinite values, unsupported platforms, aggregate
RUSAGE_CHILDREN for a worker, wrong PIDs and incomplete lifecycles reject. Only
POSIX selector constants are read on the actual host; no getrusage/wait4 samples
are collected. Process peaks cannot be subtracted to infer incremental allocation.
A future child-accounting collector and its phase/scope evidence require separate
review; unit fixtures do not establish real accounting accuracy.

ELF SHT_NOBITS and Mach-O zero-fill memory sizes do not represent file payload.
The parser retains native names/types/flags, addresses, disk offsets/file extents
and virtual extents. PAGEZERO/reserved virtual ranges are not resident-memory
measurements. Section/segment extents may overlap; their sum is not a unique file
footprint or cross-format equivalence. Archives are hashed but member sections
are explicitly unsupported; ELF32, fat, big-endian and PE files are unsupported.
