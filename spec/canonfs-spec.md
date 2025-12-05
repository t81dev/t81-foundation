# **canonfs-spec.md — Version 0.4.1**

# CanonFS — Canonical File System for Ternary Machines

**Version:** 0.4.1\
**Category:** Standards Track\
**Status:** Final Draft — Authorized for canonfs-rs 1.0\
**Target v1.0 Release:** Q4 2026\
**Applies to:** T81 Architecture, HanoiVM, Axion Kernel, CanonNet, T243–T19683 tiers

______________________________________________________________________

# 1. Abstract

CanonFS is the native filesystem for ternary (T81-class) computing.\
It is immutable, content-addressed, capability-secured, self-healing, tryte-compressible, and tensor-indexable.

CanonFS replaces:

- POSIX paths
- inodes
- mutable state
- ACLs and permission bits

…with a single unifying primitive:

**CanonRef = capability + content hash + (optional) sealed envelope metadata**

CanonFS guarantees deterministic indexing, perfect provenance, global uniqueness, and automatic parity-based recovery — all without compromising immutability.

______________________________________________________________________

# 2. Architectural Invariants (Non-Negotiable)

CanonFS is legally defined by five invariants. Violation of any is a protocol-level error:

1. **Immutability** — Once sealed, objects can never be modified.
2. **Content Addressing** — Object identity is `CanonHash-81(serialized_form)`.
3. **Capability Binding** — Access requires a signed capability.
4. **Deterministic Indexing** — The mapping `f(hash)` MUST remain stable forever.
5. **Self-Healing** — Writes MAY include parity shards for automatic recovery.

______________________________________________________________________

# 3. CanonBlock — 729-Tryte Atomic Unit

A CanonBlock is the indivisible storage atom of CanonFS.

- Size: **exactly 729 trytes**
- Payload: **721 trytes + 8-tryte metadata tag**
- Identity: `CanonHash-81(block)`
- All canonical forms (raw, compressed, sealed) MUST produce the same hash.

## 3.1 CanonBlock Forms

| Form Type | Tag Range | Description |
|-------------|-----------|---------------------------------------------------|
| Raw | 0x00–0x0F | Uncompressed payload |
| Compressed | 0x10–0x1F | LZ81 or Z3std tryte-optimal compression |
| Encrypted | 0x20–0x2F | CanonSeal AEAD envelope (ternary ChaCha variant) |

Hashing always occurs **post-decompression + decryption**, ensuring identical logical content yields identical identities.

______________________________________________________________________

# 4. CanonHash-81 — Mandatory Hash Function

CanonHash-81 is defined as:

```

CanonHash-81(data) := base81_encode( BLAKE3(data)[0..60] )

```

- Output: **81 Base-81 symbols**
- Security: ≥ 480 bits effective entropy
- Deterministic across platforms
- Fully content-addressing compliant
- Upgradeable via future version prefix (reserved)

CanonHash-81 is the sole hash function permitted for CanonFS v0.3–v1.0.

______________________________________________________________________

# 5. CanonObject — Sealed Envelope Standard (v0.4.1)

All objects begin with a one-byte **Type ID**, preventing type confusion attacks.

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
| 0x20 | CanonView | (Reserved) Lazy materialized views |

Object identity is always:

```

CanonHash-81( TypeID || SerializedPayload )

```

______________________________________________________________________

# 6. Capability System v2 (Fully Specified)

## 6.1 CapabilityGrant v2

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

Capabilities are **content-addressed**, **delegatable**, and **tamper-evident**.

## 6.2 CanonLink (Optional Display Hint)

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
3. **CI artifact parity** — The `canonfs_axion_trace_test` reproduction (see `docs/guides/axion-trace.md`) records those exact strings into `build/artifacts/canonfs_axion_trace.log`, providing auditors with a reference trace that CanonFS policies can expect before touching the filesystem.

Maintaining this contract makes CanonFS the canonical source of policy-verified persistence for Axion-aware binaries: canonical trace strings, capability enforcement, and deterministic writes all execute before the block is sealed inside CanonFS.

## 7.1 Persistent CanonFS Driver

The reference implementation exposes `make_persistent_driver(root)` (see `include/t81/canonfs/canon_driver.hpp`), creating a directory tree under `root` with `objects/`, `caps/`, and `parity/`. Each object is stored at `objects/<hash>.blk`, while capability masks live at `caps/<hash>.cap` as decimal `perms` values. The driver currently treats `parity/` as a placeholder directory for future repair shards.

Every `write_object`, `read_object_bytes`, `publish_capability`, and `revoke_capability` invocation runs the Axion hook before mutating `objects/` or `caps/`. That hook emits the canonical `meta slot axion event segment=meta addr=<n> action=Write` (for writes) and `action=Read` (for reads), so `scripts/capture-axion-trace.sh` can verify `canonfs_axion_trace_test` produces the same strings auditors expect. Capabilities gate access via the `CANON_PERM_READ`/`CANON_PERM_WRITE` bitmask; when no capabilities exist the driver permits bootstrap writes, but once a capability is published, only callers with the matching mask may read or write the object.

This driver fulfills the Axion observability requirements in a deterministic, disk-backed form: writes now flush to persistent storage, the Axion trace includes the canonical meta slot events before any data touches disk, and audits can replay the resulting `build/artifacts/canonfs_axion_trace.log` snippet to prove that canonical strings preceded persistence.

______________________________________________________________________

# 7. CanonParity — Reed–Solomon Recovery (v0.4.1)

CanonFS introduces automatic self-healing via parity shards.

**Default policy:** 3 data + 2 parity (survives any 2 losses)

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

Implementations MUST verify set_merkle_root before reconstruction.

______________________________________________________________________

# 8. Tryte-Native Compression (LZ81 & Z3std)

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

Compression occurs **before hashing**, ensuring identical logical content yields identical CanonHash-81.

______________________________________________________________________

# 9. CanonIndex — Sparse 81×81 Inverted Index Tensor

CanonIndex provides fast search and indexing capabilities.

- Maps **term_hash → list of (CanonRef, offset)**
- Term hashes are `CanonHash-81(token or n-gram)`
- Typical size: < 5 KB

Useful for full-text search, semantic index, and metadata lookup.

______________________________________________________________________

# 10. CanonMeta — Sparse Metadata Tensor

Key/value metadata controlled entirely by hashes.

- Keys = CanonHash-81(string)
- Values = CanonHash-81(string or object)
- Fully versioned, immutable, deterministic

Used for xattrs, user metadata, or extended ACL semantics.

______________________________________________________________________

# 11. CanonSeal — Per-Object Encryption

Encryption is object-local and content-address stable.

- Primitive: **T81-AEAD-81** (ternary ChaCha20-Poly1305 variant)
- Key: `KDF(capability || object_hash)`
- Decryption MUST occur **before** hashing for identity conformance.

______________________________________________________________________

# 12. CanonFile — Merkle-81 Tree

Leaves may be:

- Raw
- Compressed
- Encrypted (CanonSeal)

…but all MUST reduce to identical logical content before hashing.

______________________________________________________________________

# 13. CanonDirectory — Sparse 81×81 Tensor

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

# 14. Wire Formats (v0.4.1)

| Object | Canonical Wire Format |
| --------------- | ---------------------------------------------------------------- |
| CompressedBlock | [0x10][algo u8][len u16][compressed_bytes] |
| CanonParity | [0x11][data_count u16][parity_count u8][set_merkle_root][idx][…] |
| CanonIndex | [0x12][term_count u32]\[term_hash, CanonRef[], offsets[]\] |
| CanonMeta | [0x13][pair_count u16][(key_hash value_hash)…] |
| CanonSeal | [0x14][nonce 24 trytes][ciphertext][tag 16 trytes] |

Integers must be little-endian.

______________________________________________________________________

# 15. Storage Semantics (Self-Healing Guaranteed)

Implementations MUST:

- Verify CanonHash-81 on every read
- Auto-reconstruct lost shards via CanonParity
- Transparently decompress / decrypt
- Rebuild CanonIndex and CanonMeta lazily
- Never expose inconsistencies

Deletion is logical:

**Delete = publish CapabilityRevoke tombstone**

______________________________________________________________________

# 16. Operational Semantics

| Operation | Behavior (v0.4.1) |
| --------- | --------------------------------------------------------------------- |
| Write | Creates new objects + parity + optional index |
| Read | Capability validation → hash check → auto-repair → decompress/decrypt |
| Search | Via CanonIndex (fallback to brute traversal) |
| Delete | `CapabilityRevoke` tombstone |
| Recover | Fully automatic |

______________________________________________________________________

# 17. Reference Implementation Requirements (canonfs-rs 1.0)

Must implement:

- Rust + BLAKE3 → Base-81
- LZ81 & Z3std compressors
- Reed–Solomon GF(3⁹) parity engine
- Sparse CanonIndex + CanonMeta generators
- FUSE and HTTP gateways
- Background repair daemon
- Axion policy hooks

______________________________________________________________________

# 18. Compatibility Layer (FUSE)

Exposes structured, layered views:

- `.canon/meta/`
- `.canon/index/`
- `.canon/parity/`
- `.canon/raw/`

Hidden unless `CANONFS_DEBUG=1` or `-o debug` is enabled.

______________________________________________________________________

# 19. Future Extensions

Planned for post-1.0:

- `0x20 CanonView` (lazy materialized views)
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
