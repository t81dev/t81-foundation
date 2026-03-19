# **canonfs-spec.md — Version 0.4.2**

# CanonFS — Canonical File System for Ternary Machines

**Version:** 0.4.2\
**Category:** Standards Track\
**Status:** Reconciliation Draft — aligned to current T81 CanonFS drivers pending RFC-0054 review\
**Target v1.0 Release:** Q4 2026\
**Applies to:** T81 Architecture, HanoiVM, Axion Governance Kernel, CanonNet, T243–T19683 tiers

______________________________________________________________________

# 1. Abstract

CanonFS is the native filesystem for ternary (T81-class) computing.\
In the current T81 release line it is primarily an immutable, content-addressed, capability-gated object store with deterministic verification and Axion-observable persistence hooks.

CanonFS replaces:

- POSIX paths
- inodes
- mutable state
- ACLs and permission bits

…with a single unifying primitive:

**CanonRef = capability + content hash + (optional) sealed envelope metadata**

CanonFS guarantees deterministic indexing, stable object lookup, provenance-friendly content identity, and immutable writes. Richer typed envelopes, parity recovery, and compression-aware canonicalization remain versioned extensions rather than current persistent-driver guarantees.

______________________________________________________________________

# 2. Architectural Invariants (Non-Negotiable)

CanonFS is legally defined by five invariants. Violation of any is a protocol-level error:

1. **Immutability** — Once sealed, objects can never be modified.
2. **Content Addressing** — Current-release object identity is `CanonHash-81(payload_bytes)`.
3. **Capability Binding** — The current persistent driver enforces access through published permission masks; richer signed capability objects are future-versioned.
4. **Deterministic Indexing** — The mapping from CanonRef to on-disk object path MUST remain stable for a given identity function.
5. **Extension Honesty** — Parity, typed envelopes, and compression-aware canonicalization MUST NOT be described as current persistent-driver guarantees unless implemented end to end.

______________________________________________________________________

# 3. CanonBlock — 729-Tryte Atomic Unit

A CanonBlock is the logical storage atom of CanonFS.

- Size: **exactly 729 trytes**
- Payload: **721 trytes + 8-tryte metadata tag**
- Identity: `CanonHash-81(block_bytes)`
- The current drivers persist raw payload bytes. Compression/encryption normalization rules are extension material and are not part of the current persistent-driver contract.

## 3.1 CanonBlock Forms

| Form Type | Tag Range | Description |
|-------------|-----------|---------------------------------------------------|
| Raw | 0x00–0x0F | Uncompressed payload |
| Compressed | 0x10–0x1F | LZ81 or Z3std tryte-optimal compression |
| Encrypted | 0x20–0x2F | CanonSeal AEAD envelope (ternary ChaCha variant) |

For the current release line, CanonBlock hashing occurs over the exact persisted payload bytes. Post-decompression/decryption identity equivalence is a future-versioned extension.

______________________________________________________________________

# 4. CanonHash-81 — Current Release Identity Function

For the current T81 CanonFS drivers, `CanonHash-81` denotes the project-defined canonical object identity function produced by `t81::hash::hash_bytes(...)` over payload bytes.

- Output: implementation-defined canonical string form used throughout T81
- Deterministic across platforms
- Fully content-addressing compliant
- User-facing CanonFS and related CLI surfaces currently expose CanonFS object references as `sha3-256:<hash>`

Changing this identity function requires an explicit migration plan. RFC-0054 defines the reconciliation and compatibility questions that must be answered before any future algorithm or prefix change.

______________________________________________________________________

# 5. CanonObject and ObjectType Semantics (v0.4.2)

The T81 CanonFS API carries an `ObjectType`, but the current drivers do not include `ObjectType` in object identity. In the current release line, `ObjectType` is descriptive API metadata rather than part of the hashed persistent identity.

| Type ID | Object Type | Description |
|---------|--------------------|----------------------------------------------------------------|
| 0x00 | RawBlock | 729-tryte canonical block |
| 0x01 | FileNode | Merkle-81 interior node or leaf reference |
| 0x02 | Directory | Sparse 81×81 tensor |
| 0x03 | Snapshot | CanonHash-81 of root directory |
| 0x04 | CapabilityGrant | v2 signed capability (§6.1) |
| 0x05 | CapabilityRevoke | Signed anti-token used for canonical deletion |
| 0x10 | CompressedBlock | [algo][compressed_payload] |
| 0x11 | CanonParity | Reed–Solomon parity shard (§7) |
| 0x12 | CanonIndex | Sparse inverted index |
| 0x13 | CanonMeta | Sparse metadata tensor |
| 0x14 | CanonSeal | Encrypted + authenticated envelope |
| 0x15 | CanonLink | Symbolic link with optional display hint (§6.2) |
| 0x16 | CanonExec | Executable metadata + entrypoint |
| 0x17–0x1F | Reserved | |
| 0x20 | CanonTensor | Canonical tensor object |

Current-release object identity is:

```

CanonHash-81( PayloadBytes )

```

Future typed-envelope CanonFS versions MAY define identities over `TypeID || SerializedPayload`, but they MUST do so under an explicit compatibility or migration contract.

______________________________________________________________________

# 6. Capability System

## 6.1 Current Persistent-Driver Capability Contract

The current persistent driver stores capability state as per-object permission masks at `caps/<hash>.cap`.

Current guaranteed fields/semantics are:

- `target`
- `permissions`
- bootstrap access before any capability exists for an object
- read/write enforcement once a capability mask has been published

The richer `CapabilityGrant` structure in the C++ API is forward-looking. The current persistent driver does not persist or verify the full signed grant object described below.

## 6.2 Future CapabilityGrant Object

```text
CapabilityGrant ::= {
  target:         CanonHash-81,   // Object or subtree root
  permissions:    u16,            // Bitfield: rwx-search-exec
  granted_by:     CanonHash-81,   // Public key OR prior grant hash
  expires_at:     u64,            // T81 chain height; 0 = never
  revocable_by:   CanonHash-81,   // Entity allowed to revoke
  signature:      81-tryte ed448-ph (or QR-resistant successor)
}
```

This structure is a future-versioned CanonFS object model, not a current persistent-driver guarantee.

## 6.3 CanonLink (Optional Display Hint)

```text
CanonLink ::= {
  target:        CanonHash-81,
  display_hint:  Option<UTF8 String ≤ 255 trytes>
}
```

display_hint MUST NOT affect CanonHash-81.

______________________________________________________________________

# 7. Axion Observability Contract

CanonFS operations are **Axion-guarded**. Every `AXSET`/`AXREAD`/`AXVERIFY` interaction triggers an Axion syscall before touching storage so the Axion kernel can enforce capability constraints, recursion bounds, and policy predicates.

1. **Meta slot journaling** — Each Axion syscall prepends a deterministic `meta slot axion event segment=meta addr=<value>` entry to the Axion trace before the actual write. CanonFS implementations MUST emit `action=Write` for `AXSET`/`write_object` paths and `action=Read` for `AXREAD`/`read_object` paths so policies can assert them via `(require-axion-event (reason "<substring>"))`.
2. **Trace hygiene** — Axion traces MUST include the same `meta slot` string every time CanonFS writes persist metadata (e.g., capability grants, snapshots, links). Changing the verbatim string invalidates RFC-0020 policies and must be coordinated through a new RFC.
3. **CI artifact parity** — The `canonfs_axion_trace_test` reproduction (see `docs/developer-guide/internals/axion-trace.md`) records those exact strings into `build/artifacts/canonfs_axion_trace.log`, providing auditors with a reference trace that CanonFS policies can expect before touching the filesystem.

Maintaining this contract makes CanonFS the canonical source of policy-verified persistence for Axion-aware binaries: canonical trace strings, capability enforcement, and deterministic writes all execute before the block is sealed inside CanonFS.

## 7.1 Persistent CanonFS Driver

The reference implementation exposes `make_persistent_driver(root)` (see `include/t81/canonfs/canon_driver.hpp`), creating a directory tree under `root` with `objects/`, `caps/`, and `parity/`. Each object is stored at `objects/<hash>.blk`, while capability masks live at `caps/<hash>.cap` as decimal `perms` values. The driver currently treats `parity/` as a placeholder directory for future repair shards.

Every `write_object`, `read_object_bytes`, `publish_capability`, and `revoke_capability` invocation runs the Axion hook before mutating `objects/` or `caps/`. That hook emits the canonical `meta slot axion event segment=meta addr=<n> action=Write` (for writes) and `action=Read` (for reads), so `scripts/capture-axion-trace.sh` can verify `canonfs_axion_trace_test` produces the same strings auditors expect. Capabilities gate access via the `CANON_PERM_READ`/`CANON_PERM_WRITE` bitmask; when no capabilities exist the driver permits bootstrap writes, but once a capability is published, only callers with the matching mask may read or write the object.

Read paths re-verify content identity by default: after loading bytes, the driver recomputes `CanonHash-81(payload_bytes)` and compares it to the requested `CanonRef`. Mismatches fail with deterministic decode errors. A diagnostic override exists via `T81_CANONFS_READ_VERIFY=0`.

This driver fulfills the Axion observability requirements in a deterministic, disk-backed form: writes flush payload bytes to persistent storage, the Axion trace includes the canonical meta slot events before any data touches disk, and audits can replay the resulting `build/artifacts/canonfs_axion_trace.log` snippet to prove that canonical strings preceded persistence.

______________________________________________________________________

# 8. CanonParity — Optional / Future-Versioned Recovery

CanonFS parity support is not a current persistent-driver guarantee.

The persistent driver currently creates a `parity/` directory but does not persist or consume parity shards for automatic repair. The in-memory implementation may provide stronger parity-related behavior for testing and experimentation.

Any CanonParity wire format or repair workflow described below is future-versioned until the persistent reference driver implements it end to end.

Wire format (v0.4.1):

```text
CanonParity ::= [
  0x11,
  data_count:       u16,
  parity_count:     u8,
  set_merkle_root:  CanonHash-81,     // NEW: O(1) validation
  shard_idx:        u8,
  targets:          CanonHash-81[data_count],
  parity_data:      729 trytes
]
```

Implementations that claim CanonParity support MUST verify `set_merkle_root` before reconstruction.

______________________________________________________________________

# 9. Tryte-Native Compression (Extension Material)

| Algo | Code | Typical Ratio | Speed | Use Case |
| ----- | ---- | ------------- | ------ | ----------------------------- |
| LZ81 | 0x10 | ~2.8× | Fast | General files |
| Z3std | 0x11 | ~4.2× | Medium | Large tensors, logs, archives |

### 8.1 Recommended Heuristic

```rust
if entropy_9 >= 7.8 trit/bit      => Z3std
else if repetitiveness >= 0.42    => LZ81
else                               => Raw
```

Compression-normalized identity is not part of the current persistent-driver contract. Implementations that add compressed-object support MUST define the exact hashing boundary explicitly.

______________________________________________________________________

# 10. CanonIndex — Sparse 81×81 Inverted Index Tensor

CanonIndex provides fast search and indexing capabilities.

- Maps **term_hash → list of (CanonRef, offset)**
- Term hashes are `CanonHash-81(token or n-gram)`
- Typical size: < 5 KB

Useful for full-text search, semantic index, and metadata lookup.

______________________________________________________________________

# 11. CanonMeta — Sparse Metadata Tensor

Key/value metadata controlled entirely by hashes.

- Keys = CanonHash-81(string)
- Values = CanonHash-81(string or object)
- Fully versioned, immutable, deterministic

Used for xattrs, user metadata, or extended ACL semantics.

______________________________________________________________________

# 12. CanonSeal — Per-Object Encryption

Encryption is object-local and content-address stable.

- Primitive: **T81-AEAD-81** (ternary ChaCha20-Poly1305 variant)
- Key: `KDF(capability || object_hash)`
- If CanonSeal becomes part of the persistent-driver contract, the identity boundary MUST be versioned explicitly. This is not a current persistent-driver guarantee.

______________________________________________________________________

# 13. CanonFile — Merkle-81 Tree

Leaves may be:

- Raw
- Compressed
- Encrypted (CanonSeal)

…but these equivalence rules are future-versioned and not part of the current payload-hash persistent-driver contract.

______________________________________________________________________

# 14. CanonDirectory — Sparse 81×81 Tensor

```text
Directory ::= {
  entries: Map<(u8,u8), CanonRef>,
  index:   CanonRef | null,     // CanonIndex
  meta:    CanonRef | null,     // CanonMeta
  parity:  [CanonRef]           // Optional CanonParity shards
}
```

Deterministic. Immutable. Content-addressable.

______________________________________________________________________

# 15. Wire Formats (Versioned Object Extensions)

| Object | Canonical Wire Format |
| --------------- | ---------------------------------------------------------------- |
| CompressedBlock | [0x10][algo u8][len u16][compressed_bytes] |
| CanonParity | [0x11][data_count u16][parity_count u8][set_merkle_root][idx][…] |
| CanonIndex | [0x12][term_count u32]\[term_hash, CanonRef[], offsets[]\] |
| CanonMeta | [0x13][pair_count u16][(key_hash value_hash)…] |
| CanonSeal | [0x14][nonce 24 trytes][ciphertext][tag 16 trytes] |
| CanonTensor | [0x20][ver][fmt][rank][res][shape][payload] |

Integers must be little-endian. These wire formats are extension material unless and until a CanonFS implementation explicitly persists them as part of its normative contract.

______________________________________________________________________

# 16. Storage Semantics (Current Persistent Driver)

Implementations MUST:

- Verify CanonHash-81 on every read unless explicitly disabled for diagnostics
- Preserve immutability once `objects/<hash>.blk` is written
- Expose deterministic capability-mask enforcement once capability state exists
- Never silently return bytes whose recomputed CanonHash-81 does not match the requested CanonRef

Implementations MAY additionally:

- support parity-backed repair
- support transparent decompression/decryption
- support typed CanonObject envelopes
- support index and metadata rebuilding

Deletion is logical:

**Current persistent driver:** remove or alter capability state so future access is denied.

**Future versioned model:** publish `CapabilityRevoke` tombstone objects.

______________________________________________________________________

# 17. Operational Semantics

| Operation | Behavior (v0.4.1) |
| --------- | --------------------------------------------------------------------- |
| Write | Creates new immutable `objects/<hash>.blk` payload object; current identity hashes payload bytes only |
| Read | Capability validation → hash check → return bytes |
| Search | Out of scope for the current persistent reference driver |
| Delete | Current driver revokes/removes capability state; tombstone model is future-versioned |
| Recover | Persistent-driver automatic repair is not currently guaranteed |

______________________________________________________________________

# 18. Reference Implementation Requirements (Current T81 Drivers)

Must implement:

- deterministic object identity via the T81 CanonHash path
- payload persistence under `objects/<hash>.blk`
- capability-mask persistence under `caps/<hash>.cap`
- read-back hash verification
- Axion policy hooks

Future CanonFS implementations MAY add richer object envelopes, parity, compression, gateways, and repair daemons, but they MUST NOT be presented as current reference-driver guarantees unless implemented.

______________________________________________________________________

# 19. Compatibility Layer (FUSE)

Exposes structured, layered views:

- `.canon/meta/`
- `.canon/index/`
- `.canon/parity/`
- `.canon/raw/`

Hidden unless `CANONFS_DEBUG=1` or `-o debug` is enabled.

______________________________________________________________________

# 20. Future Extensions

Planned for post-1.0:

- CanonSync (CRDT-based multi-writer convergence)
- CanonNet (global parity-gossip fabric)
- Quantum-resistant signatures (QR-Falcon / STARK-Poseidon-81)

______________________________________________________________________

# Appendix A: Default Policy Matrix

| Workload | Compression | Parity (data:parity) | Index |
| ------------------ | ----------- | -------------------- | ----- |
| General files | LZ81 | 3:2 | Yes |
| Large tensors/logs | Z3std | 6:3 | Yes |
| Cognitive graphs | Z3std | 9:3 | Full |
| Cold archival | Z3std | 10:10 | No |

______________________________________________________________________
