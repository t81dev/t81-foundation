# Executable artifact boundary — proposal, not implementation

Disposition: **reproduced defect; design ready for maintainer review; implementation pending authorization.**
Main refreshed at `9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2`; released v1.9.4 source
`295fc67245179f1b26e5284c70520019e72bb6c3`. This supplements the release-scope
proposal in #788 without modifying that PR. v2.0.0 remains recommended / HOLD;
#780 remains excluded. Runtime SemVer is not an ISA/profile identifier.

Codex authored the release assessment and contributed earlier Windows/capture and
#787 work. These are new local engineering reproductions, not independent review
or authentication of the original release binaries. No runtime/specification,
active workflow, existing fixture, opcode assignment or version was changed.

## Reproduced through actual readers and writers

Freshly compiled the actual `isa/encoding.cpp`, `isa/binary_io.cpp`, and required
`codec/base81.cpp` at each revision, with matching extracted headers and `-DNDEBUG`.
The two codec sources and `program.hpp` are identical across those revisions;
the mapping in `opcodes.hpp` differs. No reused runtime archive was needed.
The probe uses ordinary conditionals/return codes, not disabled assert checks.

Each producer created one `VLoad {a=1,b=2,c=3}` with actual `save_program` and
`encode`, then the actual reader functions loaded the resulting files:

| Producer → reader | Persisted `load_program` | Raw `decode` |
| --- | --- | --- |
| v1.9.4 → v1.9.4 | VLoad, byte 194 | VLoad, byte 194 |
| v1.9.4 → main | **TWMATMUL, byte 194; accepted** | **TWMATMUL, byte 194; accepted** |
| main → main | VLoad, byte 240 | VLoad, byte 240 |
| main → v1.9.4 | **Unknown, byte 240; accepted** | InvalidOpcode rejection; probe exits 5 |

The old persisted file is 106 bytes, SHA-256
`96a18c69e33cd90abf5553cbae4e03af52446430446c3ed63664f998e05b4951`.
Its raw counterpart is 13 bytes, SHA-256
`d0079eb1555bd5eee53731ed106780dcef2350807212d09707e644a214fb711b`.
The current standalone `tools/tisc_base81.cpp` file-reading/conversion utility also
compiled freshly and accepted the old raw file (exit 0); its rendered output is
retained. It has no producer-profile check.

This demonstrates semantic misidentification at actual artifact-loading boundaries,
not successful execution of either operation. Full CLI disassembly/validation,
VM execution and Linux/Windows runs were **not** executed in this task. CLI
reachability below is source inspection. These are newly generated files from old
source, not authenticated historical artifacts. Initial missing-codec link failures
are retained; dependency-complete compiles succeeded. Commands/logs are in evidence/.

## Inventory: distinct formats and call paths

| Surface | Implemented representation | Readers / writers and impact |
| --- | --- | --- |
| Raw instruction records | `encode/decode`: 13 bytes per instruction; u8 opcode and 3 little-endian i32 operands; no magic/version/profile | Public `include/t81/isa/encoding.hpp`; `tools/tisc_base81.cpp` reads raw file and emits Base81 text, or parses text and writes `.bin`; encoding/property/determinism tests. Raw decoder checks current opcode validity, which cannot detect a valid reassignment |
| Saved Program (`.tisc` by convention) | Native-endian u64 instruction count; each record u8 opcode, i32 A, **i64 B**, i32 C, u8 LiteralKind = **18 bytes**; then pools/policy/type aliases/match/enum metadata, optional BigInt tail | `save_program/load_program` in binary_io.cpp. Count caps exist, but opcode/LiteralKind values are cast unchecked; optional tails are discovered by EOF, not a schema version. No magic/profile/endian declaration. Final stream/write checks and aggregate resource bounds need strengthening, not assumed complete |
| Base81 view | Numeric opcode/operands/LiteralKind in ternary digit text; sectioned pools/metadata | `base81_view::render/parse`. Not raw bytes, and not an authenticated/profile-bound executable. CLI `tisc encode/decode` uses **save/load Program**, whereas standalone `tisc_base81` uses **raw encode/decode**. Similar command names do not imply the same format |
| In-memory Program | Typed Insn vector with i64 B, pools and metadata, no artifact identity/profile field | `BinaryEmitter::emit` produces Program, not a serialized container. `IVirtualMachine::load_program` copies it and initializes state. Source compilation, Python/WASM, DPE, AI backend and C++ tests enter here; they do not load an on-disk format through this API |
| CanonFS / traces / hashes | Content-addressed opaque bytes and separately recorded execution data | Hashing/importing bytes does not bind opcode semantics. `trace replay` loads `.tisc`; CLI run resolves locally compiled or cached project `main.tisc`. Hash/profile conflicts must be rejected at execution admission; do not treat CanonFS identity or mtime as a version |

Raw encoding narrows in-memory B from i64 to i32 and omits LiteralKind/pools and
metadata. It is not a lossless general Program serializer. A saved Program must
not be converted to raw13 merely to add a version header. Program fields such as
function_metadata and weights_model also are not serialized by save_program;
new format claims must explicitly enumerate support instead of promising all
in-memory state is persistent.

Source-inspected loading/writing integration points (complete line inventory in
`evidence/callers.json` and context files):

- `tools/cli/core/driver.cpp`: compile → save (1122); run_tisc (1412), disasm
  (1505), debug (1539), trace replay (2405), LLVM (3083), MLIR (3199) → load.
  Source-only REPL paths produce in-memory programs instead.
- `tools/cli/core/main.cpp`: Axion simulate (5364); determinism capture/replay/
  multi-run (6033, 6315, 6603); VM inspect/run paths (7085, 7271, 7323);
  tier check/gate (7547); tisc diff (7622–7623), validate (7703), stats (7722),
  encode (7765), decode (7780); memory-stats (8084). Profile command delegates
  to source compilation or run_tisc. `run` can reuse project main.tisc by mtime
  (10810 onward), which is also subject to strict admission.
- `tools/tisc_base81.cpp`: separate raw file adapter. Public raw helpers remain
  for explicit low-level use; this tool must declare the supplied interpretation.
- Python and WASM inspected bindings compile source/pass in-memory Program;
  no separate persisted loader was found there. Native consumers can call the
  public file helpers directly; centralize the default guard in those helpers,
  not just CLI argument parsing.

The tracked suffix inventory finds 41 `.tisc` binary candidates and no tracked
`.raw13`/`.bin` files under that search. All 41 fit the current 18-byte record
prefix under an explicitly **assumed** little-endian layout and show only bytes
below 174. This is inventory, not producer identification, semantic validation,
or permission to auto-admit. Hidden/external artifacts and other suffixes are not
covered. Original files and their hashes remain unchanged.

## Bounded design: one admission layer, no opcode translation

### 1. Separate container identity from execution profile

Introduce a strict artifact reader/writer around the existing full-Program
payload, plus an explicit raw adapter. Do not change opcode assignments, operand
semantics, raw13 width or public raw encode/decode results.

A container version answers how bytes are framed/serialized. A payload codec
answers how fields are represented. An execution descriptor identifies the
specific mapping **and operand/trap semantics**, not just the runtime release or
opcode-name table. None may substitute for another.

Initially support only a maintainer-approved descriptor for the implemented
hosted revision and a constrained existing Program codec. The descriptor must
pin mapping, literal kinds, execution semantics, admitted subset, source/conformance
references and digest. This documents implementation behavior; it must not claim
v1.1.0 compliance for extension bytes or ratify proposed tisc-v2.0 semantics.
All other descriptors, including released-v1.9.4 extension mapping, reject as
UnsupportedProfile. An explicit old label does **not** make the current VM a
legacy interpreter. A future legacy evaluator is a separate decision/project.

### 2. Proposed envelope (format v1; values pending approval)

Use a new magic `T81PROG\0` and a fixed 64-byte, little-endian header:
8-byte magic; u16 envelope_version=1; u16 header_size=64; u32 flags=0;
u32 manifest_size; u64 payload_size; 32-byte SHA-256; u32 reserved=0.
Digest input is the header with its digest field zeroed, followed by manifest
bytes and payload bytes. This binds declarations to payload identity, not author
truth, signatures or authority. Existing signed/CAS identities are never reused.

Manifest has a separately specified deterministic JSON encoding and exact schema:
`payload_codec`, `execution_profile`, `execution_profile_digest`, `producer_revision`.
Producer revision is a claim retained for diagnostics, **not a profile selector**.
Reject duplicate keys, noncanonical encoding, unknown required fields/flags,
unsupported versions/codecs/profiles and any contradictory explicit declaration.
Suggested caps: 16 KiB manifest and 64 MiB payload, with overflow-safe header/length
checks and aggregate allocation budget. Caps are proposals to validate against
real fixtures; an artifact may never raise its own limits.

First codec is explicitly host-layout-restricted existing Program serialization,
not a newly claimed portable standard. Require declared/verified little-endian,
32-bit int, 64-bit length/B and binary64 environment; maintain an audited list of
supported numeric-pool serializations. Reject unsupported hosts/fields. Check
stream state after every required field, checked length multiplication, valid
enums/opcodes for the descriptor, pool/operand references, and exact payload
consumption, including defined optional-tail variants. Wrapping a permissive
parser or checking only final EOF is insufficient. A fully portable canonical
payload codec is a subsequent versioned design, not this containment repair.

Only accept an envelope after opening once and validating bounded lengths,
metadata, supported profile, and digest on the **same retained bytes/open object**
that are parsed. Do not hash a path and reopen it. Build Program locally and return
it only on complete success. No VM load/step, policy action, output file replacement,
or externally visible execution state occurs on rejection. Normal policy checks
remain necessary after format admission.

### 3. API and call-site migration

Add `read_program_artifact(path, expected_binding)` returning an admitted Program
plus immutable binding (or structured error), and `write_program_artifact(bound,
path)` requiring an explicit supported binding. Centralize all persisted CLI
callers listed above on that layer. The compiler knows its implementation target
and passes the approved descriptor explicitly; loading an arbitrary old file does
not establish that descriptor. Transformations must preserve compatible binding
or reject; they must never stamp current metadata onto an unchecked loaded Program.

The existing `load_program(path)` signature can become a strict wrapper accepting
only supported envelopes; it must reject unversioned input by default. Keep the
old parser internal to the explicit compatibility adapter, not an automatic retry.
For writes, add an explicit binding overload and migrate all producers. Deprecate
or reject the unbound two-argument save call; do not silently infer provenance
from an arbitrary Program. This behavioral/API migration needs approval even
with a proposed major runtime version. Return structured errors through CLI
adapters with nonzero status, not uncaught exceptions or successful validate JSON.

Leave raw `encode/decode` unchanged as low-level representation APIs, including
frozen test paths. They are not an admission or provenance guarantee. Introduce
an explicit raw admission adapter only where external records become executable;
it requires codec + approved profile + expected artifact hash from an explicit
caller/trusted manifest. `tisc_base81` should require an explicit interpretation
for semantic conversions and must not be an execution shortcut. Numeric/raw
inspection may remain available if clearly marked unbound and never reports
"valid executable". Do not add headers to raw helper output.

In-memory Program/VM APIs remain trusted typed interfaces in this bounded repair;
no claim that arbitrary direct native caller casts are sandboxed. Future foreign
byte entry points must use the admission layer. No broad VM ABI refactor is needed.

### 4. Legacy and conflict policy

Missing magic/version/profile: **reject by default**, including current-generated
unversioned files. Never infer from extension, file size/divisibility, opcode
histogram, timestamps, current runtime version or successful parse. Do not try
container, raw13 and Base81 readers in succession until one succeeds.

A separate explicit legacy import/inspection operation may accept codec, payload
ABI, producer profile and exact content hash as out-of-band inputs, after checking
the profile is supported and all declarations agree. If legacy semantics are not
supported, allow opaque archival inspection only; execution/transcoding rejects.
For supported, independently established semantics, wrapping may preserve bytes
and add binding under a **new** identity. It never silently remaps opcode bytes.
Changing old instructions requires recompilation from known source and a new
artifact with fresh policy/signature/provenance review, not a translator guess.

CLI flag, envelope, CanonFS manifest and embedded note, where implemented, must
agree on their explicit shared declarations. No priority override of a conflict.
Absence is not a conflict but is also not sufficient authorization. A profile
string supplied by an untrusted artifact selects only an approved evaluator;
it does not authorize execution or authenticate its producer.

## Fixtures, frozen contracts and authorization

- Preserve `include/t81/isa/encoding.hpp` raw contract, golden raw byte tests,
  `tisc_encoding_test`, full opcode matrix values, and existing symbolic/compile
  determinism fixtures. Do not add envelope bytes to their raw API output.
- `TISC_CAPABILITY_CONTRACT_v1.1.0.md` freezes 13-byte width and values 0–173,
  reserving 174–255. Current extension behavior is not blanket conformance to it.
  Registry, header and freeze script disagree in scope; do not “fix” them by
  reassigning values or broadening freezes in this change. Mapping digest alone
  does not settle RFC-00D6's LOAD/STORE/branch/trap semantic discrepancies.
- `scripts/ci/t81lang_repro_gate.py` hashes **whole saved .tisc files**, not raw13.
  New envelopes necessarily change these hashes. Keep the old fixture sources,
  expected hash and historical evidence intact; establish a separately versioned
  artifact/gate namespace and approved expected witness. Do not simply regenerate
  the existing golden to make CI green. Existing metadata/complex/alias roundtrip
  tests must retain legacy coverage while adding new-container cases.
- The 41 checked-in .tisc artifacts must not be relabeled/replaced automatically.
  Establish producer identity or recompile separate new artifacts from known
  sources and retain originals; core-only bytes do not prove a unique profile.
- Commons discrepancy D-606-ENCODING already records raw13 versus saved18.
  Preserve it and append this pinned reproduction. RFC-00D6 references frozen CPA
  binaries/traces and hash `b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`.
  Its referenced experiments/cpa004–006 and evaluator paths are absent from pinned
  main; their validation/pins need locating separately, not borrowing local
  untracked copies or claiming those historical guarantees were reverified.

Required maintainer decisions **before implementation**: approve the admission
format and exact supported implementation descriptor/codec/limits; approve default
rejection and public loader/writer/CLI migration; approve legacy handling and
versioned golden/gate separation; determine the narrow RFC-00D6 DM-09 applicability
and record approval. Its Section 10 authorization remains blank; proposed profile
names are not accepted requirements. DM-01–08 ISA/semantics/registry changes remain
out of scope and require their own formal decisions. Frozen fixture/registry/schema
changes, protected-artifact replacement, merges, tags/publication and scientific
acceptance require their respective human authorization; none is granted here.

## Regression and review plan (proposed, not executed)

1. **Historical misinterpretation witnesses:** retain old-source-generated raw13
   and saved18 fixtures with producer commands/hashes. Confirm strict default
   rejection for each old unversioned file; explicitly labeling an unsupported
   old profile also rejects, with no VM load/step or output side effect. Test
   known-current unversioned artifacts likewise; no opcode-histogram escape hatch.
2. **Positive new format:** compiler → envelope → loader → disasm/run/trace under
   exact approved profile. Roundtrip i64 B boundaries, all LiteralKinds, empty and
   populated supported pools, policy, alias/match/enum and BigInt metadata; reject
   unsaved fields instead of silently discarding them. Verify deterministic bytes.
3. **Header/adversarial:** bad/partial magic, unsupported version/header-size,
   flags/reserved fields, missing/duplicate/unknown manifest data, lengths at and
   beyond caps, integer overflow, truncated scalar/pool/tail, trailing bytes,
   digest mismatch and unknown opcode/LiteralKind/pool references. Require exact
   diagnostic class/nonzero status and bounded allocation. Include reverse-old
   byte240 and valid-but-reassigned byte194 witnesses; validity alone must not pass.
4. **Binding conflicts:** each pair of explicit CLI/envelope/manifest/note bindings,
   including same profile name/different digest, fails closed. Test absence versus
   contradiction separately; unsupported descriptors never select current runtime.
5. **No path/race shortcuts:** replace/rename/symlink path after open in a controlled
   fixture; parsing must use admitted retained bytes or reject, never a second
   object. Test failed writes preserve pre-existing files and clean owned temps.
6. **Every consumer:** CLI run/debug/disasm/tisc validate/stats/diff/encode/decode,
   tier, VM, determinism, trace replay, memory/profile, project cached main.tisc,
   LLVM/MLIR and public C++ loader wrappers. Validate must not emit ok:true on
   failure; no tool may bypass admission or convert unknown-profile files to new
   current-profile artifacts. Source compilation/internal Program paths remain
   functional and policy-before-effect checks remain active.
7. **Raw preservation:** byte-for-byte raw encode golden and existing opcode matrix
   unchanged; explicit record length, signed i32 boundaries, invalid opcode tests.
   Raw adapter rejects non-i32 B or absent pools when converting from full Program;
   no change to low-level historical narrowing behavior in this bounded task.
8. **Evidence gates:** old reproducibility witness unchanged in its frozen scope;
   new separately named envelope witness and payload/component identities recorded.
   Historical raw/CPA files/traces/manifests hash-identical, or unavailable checks
   explicitly blocked. Do not weaken existing assertions or freeze checks.
9. **Execution matrix:** Release/-DNDEBUG active checks via TEST_CHECK/explicit
   returns; inject a failing expectation to prove nonzero status. Target Linux
   GCC/Clang, macOS ARM64/x86_64, Windows MSVC/clang-cl, Linux ARM64; ASan/UBSan on
   supported local/CI lanes. Mark unavailable lanes; require fresh applicable
   required checks on implementation SHA. No performance campaign or Gate 2.

Implementation can be split into three reviewable changes after authorization:
format/descriptor/strict reader-writer + unit tests; centralized CLI/public API
migration + all negative integration tests; separately approved fixture/gate
namespace and migration documentation. No raw API redesign or legacy VM emulation
is included. Release stays HOLD until the chosen boundary and upgrade path are
implemented, reviewed and qualified.
