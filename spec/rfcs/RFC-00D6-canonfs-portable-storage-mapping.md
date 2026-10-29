# RFC-00D6: CanonFS portable storage mapping

- Status: draft; additive opt-in implementation candidate
- Scope: persistent filename mapping, format selection and compatibility
- Date: 2026-10-22
- Related: RFC-0054, RFC-00D1; preserved audit PR #554

## Compatibility decision

The accepted RFC-0054/current supplemental contract describes the legacy
`objects/<Base81>.blk`, `caps/<Base81>.cap`, `parity/` layout. Replacing it
implicitly would break readers and offline verifiers. This proposal does not do
that. `make_persistent_driver(root)` retains the legacy format and default. A
separate `make_portable_persistent_driver(root)` explicitly selects storage-map v2
for absent/empty roots or reopens a previously valid v2 root. The CLI and v1
interchange default remain legacy. No legacy data is renamed or migrated.

The new factory and a reserved root marker are additive. No existing object,
reference, typed envelope, algorithm or default layout changes. Switching the
default, teaching old binaries to read v2, changing existing filename components,
or migrating historical stores requires a separately reviewed compatibility plan.
If this additive boundary is not accepted, retain this RFC/tests as a proposal;
do not ship a silent replacement of the legacy factory.

## Identity versus representation

The existing identity is the first 32 bytes of SHA3-512 over raw payload bytes.
`ObjectType` does not participate. Base81 display and existing `sha3-256:` CLI
vocabulary are unchanged (the prefix is historical, not a new algorithm claim).
A storage key is not an identity or a user-visible reference scheme.

For a digest byte sequence `b[0..31]`, v2 uses exactly 64 lowercase ASCII hex
digits, big-endian byte order, including every leading zero. For each byte, emit
`hex[b >> 4]`, then `hex[b & 15]`, using `0123456789abcdef`. The strict inverse
requires exactly 64 such digits and accepts no uppercase, whitespace, suffix,
NUL, separators, alternate alphabet, normalization or truncation. The mapping is
injective on all 2^256 byte sequences: equality of keys implies equality of each
pair of nibbles and therefore every original byte. This is independent of digest
collision resistance: two different payloads with the same digest remain a hash
collision, not a filename-mapping collision.

## Versioned layout

```text
canonfs-layout                  # exact bytes: t81.canonfs.storage-map.v2 + LF
v2/objects/<64hex>.blk           # unchanged raw payload bytes
v2/caps/<64hex>.cap              # unsigned canonical decimal uint16 + LF
v2/parity/                      # empty placeholder, no repair guarantee
v2/staging/<64hex>.partial       # private incomplete publication, never a read target
```

Every fixed component is ASCII and every digest-derived stem is 64 ASCII digits.
No component is a Win32 reserved device name, contains a reserved separator or
colon, ends in a dot/space, or changes under case folding/NFC/NFD. Longest derived
component is 72 characters (`.partial`). Caller-selected root and total path
length remain subject to the actual OS/filesystem limits; no truncation or hash
shortening is permitted to satisfy them. Windows uses wide native path calls for
v2 creation/publication; filesystem path streams use native path overloads.

Marker creation is exclusive. Interrupted initialization remains incomplete and
is refused on reopen; it is not silently completed/reinterpreted as legacy. The
v2 factory refuses nonempty unmarked/unknown roots, malformed markers, extra root
names, unknown version directories, malformed object/cap/staging names, symlinks
and Windows reparse points at managed boundaries. Empty legacy directory trees
are still legacy, not permission to convert in place.

The updated legacy factory refuses any root containing the reserved marker;
legacy stores without that marker retain their behavior. A historical root that
already used this formerly unreserved marker name is refused, not overwritten;
operator review is required. Pre-update binaries do not recognize the marker:
**they must never be pointed at v2 roots**. If they create legacy directories in
one, the new v2 reader refuses the mixed namespace. No fallback to another
namespace, capability state, object version or hash algorithm is allowed.

## Publication and corruption

The host owns a root exclusively and serializes drivers, policy updates and
filesystem mutations. This is not a multiwriter/sandbox or a transaction with
external side effects. Pre-existing symlinks/reparse points are refused; concurrent
hostile namespace replacement/ancestor traversal is outside that trust boundary.

Object write: hash proposed bytes, run existing Axion/capability admission,
validate the v2 layout, then either verify an existing object's exact bytes and
digest or create an exclusive private staging file. Close the complete staging
file before publishing without replacement: same-filesystem link/unlink on POSIX,
wide `MoveFileExW` without REPLACE_EXISTING on Windows. A failed publication keeps
staging for explicit inspection. A corrupted/truncated existing object is rejected
and retained, never overwritten or treated as a successful duplicate. Stale
staging blocks new publication of that key; it is never implicitly resumed or
promoted. A successful read of an already published object may coexist with stale
staging after an interruption between POSIX link/unlink.

V2 reads always verify actual disk bytes, including empty payloads; legacy
`T81_CANONFS_READ_VERIFY` diagnostic/cache semantics remain unchanged. The v2
profile does not permit disabling verification via that legacy environment flag.
There is no fsync/FlushFileBuffers or power-loss durability claim. Failed writes
can leave private staging; after publication an I/O/cleanup failure can leave a
valid final object with an error result. No atomic effect/evidence receipt is
implied. Filesystems must support the stated local publication primitive; failure
is reported, never downgraded to unsafe replacement.

Capability filenames use the same injective mapping. Existing bitmask/bootstrap,
Axion admission and last-capability revocation semantics are retained; this is
not a redesign of authority. V2 masks have strict decimal parsing and staged
replacement so invalid/truncated masks fail closed. No signatures/delegation or
anti-rollback capability protocol is added. Parity remains a placeholder.

## Legacy reads and migration

Existing v1 roots remain readable by the legacy factory when their actual
filenames can be represented without aliasing under the host encoding used to
create them (including the legacy Windows ANSI interpretation of UTF-8 bytes), the marker is absent,
and read verification succeeds. A store already damaged by case folding or
invalid/missing filenames cannot be repaired by inventing identities or disabling
verification. Original PR #554 Windows failures remain immutable evidence.

There is **no migration command, automatic read fallback or in-place rename** in
this increment. A future separately approved copy migration must open the source
read-only, verify every payload against its exact digest, enumerate/verify masks
without relaxing admission, write a fresh v2 destination, detect duplicates and
case/normalization aliases, preserve raw failures/manifests, and obtain explicit
approval before cutover. It must retain the source, never overwrite existing
objects silently, and define crash recovery, rollback and provenance. Stores
with ambiguous/corrupt identity mappings stop for review rather than guessing.

## RFC-00D1 implications

Import/export result/provenance JSON, logical object references and historical
fixtures do not change. A layout selector is not added to the existing CLI or
stable interchange surface in this increment. Passing a v2 root to legacy-only
CLI tooling is an unsupported format selection, not a missing-object fallback.
A later explicit CLI/API extension needs separate admission/error/schema review.
Content identity can be stable across storage mappings without making host paths
part of the canonical object identity.

## Validation

The opt-in audit replays all 110 original storage transactions, using exact PO
and evidence bytes recovered from the preserved Windows archive, through both
factories. Native Windows must reproduce the original 100 legacy failures and
pass all 110 portable transactions. Linux/macOS run the same corpus and retain
all outputs and exact identities. Tests also cover all byte values at every
32-byte position, strict malformed keys, native Unicode roots, real filesystem
case/normalization behavior, empty/binary payloads, legacy cold reads, duplicate
insertion, corrupted/truncated/appended files, stale partial publication,
capability persistence and malformed masks, denial before staging, format mixing,
unknown markers, filename aliases, links/reparse boundaries and excessive path
components. Cross-platform conclusions require inspected native artifacts.
