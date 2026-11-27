# Proposed resource policy — numbers, reasons and qualification limits

These are **proposed admission limits**, not measured universal workload needs.
They replace revision 1's unsupported provisional 16 KiB manifest / 64 MiB payload.
No runtime limits change here. A reviewer must accept the compatibility restriction
and tests before activation. Inputs cannot raise their own limits; any later larger
host policy requires explicit configuration/review and separate qualification.

## Observed scale and allocation risk

Retained inventory at pinned main contains 41 `.tisc` candidates, total **47,789 B**;
largest **4,123 B** (cross_layer.tisc). Under an explicitly assumed current record
layout, the largest instruction count is **181**. No producer version is inferred.
The new local Apple clang/ARM64 header inspection reports sizeof(Insn)=24,
sizeof(Program)=328 and sizeof(string)=24. These are ABI observations, not Windows
sizes, memory peaks or benchmark results. The descriptor-map probe invokes no VM.

A file-size bound is insufficient: binary_io.cpp limits certain counts to 2^24,
but nested BigInt deserialize reads a u64 length and constructs a string directly;
tensor deserialize resizes shape/data from nested counts. A tiny hostile payload
can request enormous allocations. Do not call those paths and then claim bounded
memory merely because the outer file passed a size check.

## Proposed initial policy

| Limit | Value | Rationale and required check |
| --- | ---: | --- |
| Fixed envelope header | 64 B | Exact wire framing, flags/reserved zero; unsupported versions reject, no variable header scan |
| Manifest | **1,024 B** | Four fixed keys; codec <=64 ASCII bytes, profile token <=128, digest 64 hex, producer revision 40 hex. Fixed punctuation/keys + maximum values fit below 512 B. 1 KiB allows bounded parsing overhead without arbitrary extension fields; duplicate/unknown keys reject |
| Serialized payload | **8 MiB (8,388,608 B)** | >2,000 times largest sampled fixture; models stay external. Enough headroom for the count limits below while reducing single-file exposure 8-fold from the initial proposal. This does not establish compatibility with external workloads |
| Instructions | **65,536** | >362 times sample maximum; 18-byte serialized entries occupy 1,179,648 B. At observed 24-byte Insn, vector data is 1,572,864 B. Check actual ABI sizeof and count multiplication before allocation; do not assume those observed sizes on other hosts |
| Total pool scalar values | **65,536** | Aggregate across float/fraction/complex/BigInt values, tensor elements and shape dimensions; compound entries also count their nested numeric components. Avoid independent per-pool allowances multiplying the budget |
| Structural objects/entries | **16,384** | Aggregate string entries, pool/container headers, aliases, enum variants/fields and other nested metadata nodes. Charges even empty objects, so zero-length payloads cannot create millions of native containers |
| Metadata/string bytes | **1 MiB (1,048,576 B)** | Aggregate symbol text, policy, match/alias/enum text. Per-field and remaining-byte checks apply. It is input parsing budget, not permission to evaluate arbitrarily expensive policy expressions |
| Tensor rank | **16** | Conservative fixed metadata bound; negative dimensions and checked shape products handled by the codec's existing representation contract. This is a new admission restriction, not a VM rank-semantic change; test higher-rank rejection and qualify any real workload requiring more |
| BigInt serialized token | **256 B each; 64 KiB aggregate** | Bounds nested text allocation and numeric construction inputs, including fraction components. Caps input amplification; does not prove a CPU deadline. It excludes arbitrarily large persisted integers until a larger policy is justified |
| Decoded allocation budget | **32 MiB (33,554,432 B), cumulative** | Includes native vector capacities, string capacities, nested allocations and decoder scratch; charge before each allocation, including reallocations, with overflow-safe arithmetic. A conservative proven upper bound may substitute for instrumentation; unknown bounds reject. This is not a claim about the existing parser |

Limits are conjunctive. One count limit cannot grant more bytes or allocation
than another. Each count is checked against remaining wire bytes where a minimum
encoded size is known. Cumulative accounting prevents repeated allocate/free from
resetting the budget. Prefix/metadata work must consume bounded bytes/nodes; do not
permit recursive/expanding JSON constructs or reparse-until-success loops.

Retain at most one input snapshot of header + manifest + payload: **8,389,696 B**
maximum. Together with 32 MiB accounted decoding allocations this yields
**41,944,128 B** of specified buffer/allocation allowance, before allocator overhead,
code, stack and OS costs. This is **not** a measured/proven RSS bound. Streaming
or copying implementations must account any additional buffers explicitly.
VM load copies state and can allocate model/tensor caches; those execution costs
are outside this parser budget and need the existing runtime/resource policy.

## Implementation conditions, not a promise from a wrapper

The bounded codec must inspect every nested size, using checked arithmetic before
resize/reserve/construct. It must account representation-specific numeric scratch,
UTF-8/token decoding and tensor payload variants. Byte-preserving decoding must
not normalize values or change arithmetic to enforce these limits. If a numeric
or tensor variant has no demonstrated safe budgeted decoder, **reject that variant
before construction** and list it as unsupported. Do not ship an unbounded
fallback. A first limited implementation may therefore support fewer pools than
the full current in-memory Program; that restriction must be explicit in its codec
and writer and must not silently drop fields.

No parser deadline or child-process sandbox is proposed as a substitute for
bounded allocation. These caps do not stop arbitrary program execution, huge
external model loads or expensive runtime policy evaluation. Review those at their
separate trust boundaries. Non-regular/blocking file input needs an explicit host
I/O policy; byte limits alone do not promise a wall-clock bound.

## Required evidence before accepting these values

- Regenerate actual saved artifacts for current deterministic source fixtures
  without changing their witnesses; compare size and decoded requirements with
  these caps. Retain per-variant counts, failures and unsupported cases.
- N-1 / N / N+1 tests for every cap, aggregate exhaustion across multiple pools,
  huge nested lengths in tiny files, allocation-failure injection and budget
  exhaustion during growth. Rejections must occur before the charged allocation.
- Release-active tests on Windows MSVC/clang-cl and Linux/macOS x86_64/ARM64,
  with actual ABI sizes; supported ASan/UBSan and allocator instrumentation.
- Measure peak allocations/RSS and pathological parsing time as correctness/resource
  qualification, not a performance campaign; report limits of that measurement.
- Maintainer reviews legitimate external use cases or accepts that this first
  policy intentionally rejects them. Do not call corpus headroom general support.
