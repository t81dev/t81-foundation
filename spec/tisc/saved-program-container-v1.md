# Saved Program Container v1 — exact draft contract

Status: **design package for review; no runtime activation; freeze approvals pending**
Version: 1.0.0 documentation contract
Decision: ADR-0007; reviewed input 07968a9a307415bf8a7aa6f04056caa22db01c78.

This specifies full saved programs, not the frozen raw13 encoding. It neither
accepts a new ISA nor changes the meaning of any instruction. The maintainer's
revision-2 design approval is retained separately from review of these finalized
wire details. Until formal review/qualification, no build is trusted by this file.

## 1. Exact framing

All integer header fields are unsigned, fixed-width, little-endian. No padding,
BOM, compression, signatures, extension areas or trailing bytes exist in v1.

| Offset | Bytes | Field | Required value |
| ---: | ---: | --- | --- |
| 0 | 8 | magic | hex `54 38 31 50 52 4f 47 00` (T81PROG then NUL) |
| 8 | 2 | envelope version | 1 |
| 10 | 2 | header size | 64 |
| 12 | 4 | flags | 0 |
| 16 | 4 | manifest length M | 1..1,024 |
| 20 | 8 | payload length P | 1..8,388,608 |
| 28 | 32 | SHA-256 digest | raw digest bytes, not hex text |
| 60 | 4 | reserved | 0 |

File is exactly `header || manifest || payload`, length `64 + M + P` using
checked arithmetic. Digest input is the complete 64-byte header with bytes 28..59
zeroed, then the **same retained** manifest and payload bytes. A digest proves
content identity, not author authenticity or authorization. Unexpected version,
flags, reserved bits, lengths or EOF/trailing bytes reject; no fallback reader.

## 2. Exact manifest bytes

ASCII JSON, exactly these four keys in this order, no whitespace/newline/escapes:

`{"execution_profile":"VALUE","execution_profile_digest":"VALUE","payload_codec":"VALUE","producer_revision":"VALUE"}`

- execution_profile = literal `hosted-sha256-` followed by descriptor SHA-256 in
  64 lowercase hex digits. Must equal the locally supported descriptor identity.
- execution_profile_digest = those same 64 lowercase hex digits.
- payload_codec = exactly `native-program-v0-le64-i32-f64` in this contract.
- producer_revision = exactly 40 lowercase hex digits; informational producer
  claim, never a selector, source-authentication proof or ancestor compatibility rule.

No other keys, duplicate keys, alternate order, alternate spelling, escaped ASCII,
non-ASCII, NUL, BOM, whitespace or terminal LF is canonical. Implementations may
parse with a bounded exact parser or validate then canonical-reencode/byte-compare;
they must not allocate from unbounded JSON before checking M. An unknown identity,
codec or conflicting explicit caller/container declaration rejects. Absent caller
expectations do not override the envelope's required supported binding.

## 3. Exact descriptor identity and local authority

Descriptor bytes are the immutable `execution-descriptor.json` in the ADR-0007
package: ASCII JSON, recursive ASCII key sort, compact commas/colons, no whitespace
or terminal LF. Arrays retain their listed order; values are only objects, strings
and nonnegative decimal integers (no floats). SHA-256 is:

`515f144f1639688c1d7f1d014143eec93a4eebacdcfbbf702a14ed234a18f317`

execution_profile is therefore:

`hosted-sha256-515f144f1639688c1d7f1d014143eec93a4eebacdcfbbf702a14ed234a18f317`

The descriptor pins current hosted interpreter source, existing opcode map and
LiteralKind assignments. It does not assert normative ISA compliance, universal
implementation/verification of the 244 named values or v1.9.4 compatibility.
Build association is mandatory under the package BUILD_ASSOCIATION.md contract;
there are currently **zero approved associations**. An artifact cannot supply its
own trusted association. New descriptor content requires a new digest and review;
unsupported identities fail, never remap or choose the nearest known revision.

## 4. Payload codec and supported variants

Codec preserves the exact bytes emitted by save_program at source
9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2, not a newly portable numeric codec.
Supported host prerequisites: 8-bit bytes, little-endian, 64-bit size_t, 32-bit int,
fixed-width two's-complement i32/i64 and IEEE binary32/binary64 storage. Reject
unsupported ABI. No direct C++ object-struct dumps are introduced.

Field order: u64 instruction count and records `(u8 opcode, i32 A, i64 B, i32 C,
u8 LiteralKind)`; float pool; fraction pool; symbol pool; tensor pool; shape pool;
complex pool; policy string; alias count and metadata; match string; enum count and
metadata; optional nonempty BigInt vector. This envelope requires the complete
base tail through enum metadata, not historical EOF-based schema guessing. If a
BigInt tail exists, its count must be nonzero and its complete encoding consumes
the remainder. Existing writer's no-BigInt form ends after enum metadata.

Pool/string/vector lengths are native u64 represented little-endian under the
above ABI; scalar floats are their existing IEEE bits. Alias/enum field order and
numeric/tensor variant bytes are exactly the pinned binary_io.cpp, type_alias.hpp,
T81BigInt.hpp, fraction.hpp, tensor.hpp and T81Complex.hpp serializers, identified
by source tree in the descriptor and CODEC_SNAPSHOT.json. No arithmetic
normalization, host conversion or canonical numeric re-encoding is authorized.

Each build association explicitly enumerates supported variant labels from:
`insn18`, `float64`, `strings`, `shapes-i32`, `alias-enum-metadata`,
`fraction-bigint-text`, `tensor-raw-elements`, `tensor-canonical-fixed`,
`complex-t81float18`, `bigint-text`. Empty pools require no numeric variant;
nonempty entries and tensor variant tags require the corresponding qualified
label **before construction**. Unknown tags/labels or unqualified bounded decoders
reject, even when the program is otherwise well framed. Nothing in this document
pre-approves any decoder implementation. Writer rejects unsupported variants or
unpersisted nonempty fields rather than dropping them.

Validate representation opcode membership in the pinned map, LiteralKind 0..8,
metadata tag/boolean representation and checked lengths/stream completion. Do not
preexecute instructions or move invalid-register, division, policy or other VM
operand faults into admission. Do not fix numeric invariants or VM semantics here;
any extra semantic rejection requires a separate decision.

## 5. Resource accounting and failure ordering

Use the exact revision-2 caps in caps.json. Wire header is separate from allocation
budget; all caps are conjunctive. Numeric pool scalar count charges each pool value
and its nested numeric components (fraction = 1 + 2 BigInt components; complex =
1 + 2 float components); shapes charge dimensions, tensors charge elements.
Structural count charges every string entry, nested container/pool header,
alias/field/variant and enum node, including empty objects. Metadata bytes aggregate
symbols, policy, match, alias and enum text. BigInt token bytes charge their own
per-token and aggregate budget, including fractions; they also count toward input
and decoded allocation caps. No per-pool budget reset or artifact-requested increase.

Cumulative allocation charges capacity bytes before resize/reserve/constructor,
including nested storage, scratch and reallocations. Use actual target sizeof and
checked arithmetic. A proven conservative bound may substitute; unknown accounting
or allocation failure rejects before exposure. Retain a single bounded immutable
input snapshot; a retained mutable file descriptor alone is insufficient.

Order: bounded framing/manifest → supported local identity/ABI/build association
and declaration agreement → digest on retained bytes → bounded payload decode →
return admitted Program plus immutable binding. All failures precede VM creation,
load/step, runtime policy effects and output replacement; diagnostic output and
necessary input reads are allowed. CLI failure is nonzero and cannot emit executable
validation success. No new Trap member/code is defined. Existing execution policy,
model authorization and runtime resource budgets still apply after admission.
