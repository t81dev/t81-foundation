# canonfs-spec.md — Version 0.4.1  
**Integrated Final Draft — Ready for Reference Implementation**  
**Authors:** T81 Foundation + Grok (xAI)  
**Date:** November 22, 2025 (updated 13 hours later)

---
title: CanonFS — Canonical File System for Ternary Machines
version: 0.4.1
status: Final Draft — Reference Implementation Authorized
author: T81 Foundation + Grok (xAI)
created: 2025-11-22
updated: 2025-11-22
---

# CanonFS — Canonical File System for Ternary Machines

**Version:** 0.4.1 (minor revisions & clarifications for v1.0)  
**Status:** Final Draft — Green-lit for canonfs-rs 1.0  
**Target v1.0 release:** Q4 2026

All invariants and core design from v0.4 remain unchanged. This 0.4.1 revision only adds clarifications, minor fields, and explicit best-practice recommendations that were implicitly agreed but previously underspecified.

### Changes from 0.4 → 0.4.1

| Section                 | Change                                                                 |
|-------------------------|------------------------------------------------------------------------|
| CanonParity             | Now requires Merkle-81 root of the protected set for O(1) validation   |
| CapabilityGrant v2      | Fully specified: includes expiration height + revocable-by field       |
| CanonLink               | Optional UTF-8 display hint permitted (does not affect identity)      |
| Compression selection  | Reference impl MUST use entropy+repetition heuristic if no policy      |
| Delete semantics        | Explicitly standardized on tombstone CapabilityRevoke objects          |
| FUSE debug dirs         | Hidden by default; shown only when `CANONFS_DEBUG=1`                   |
| New object type hint    | CanonView reserved (0x20) for future lazy materialized views           |

---

# 6. Object Types (v0.4.1 — now 15 defined + 1 reserved)

| Type ID | Object Type           | Notes / v0.4.1 Additions                                 |
|---------|-----------------------|-----------------------------------------------------------|
| 0x00    | RawBlock              | unchanged                                                 |
| 0x01    | FileNode              | Merkle-81 tree node                                       |
| 0x02    | Directory             | sparse 81×81 tensor                                       |
| 0x03    | Snapshot              | CanonHash-81 of root directory                            |
| 0x04    | CapabilityGrant       | v2 — see §6.1                                             |
| 0x05    | CapabilityRevoke      | Tombstone; now the canonical delete mechanism             |
| 0x10    | CompressedBlock       | unchanged                                                 |
| 0x11    | CanonParity           | now includes set_merkle_root (§7)                         |
| 0x12    | CanonIndex            | unchanged                                                 |
| 0x13    | CanonMeta             | unchanged                                                 |
| 0x14    | CanonSeal             | unchanged                                                 |
| 0x15    | CanonLink             | may include optional UTF-8 display_hint                   |
| 0x16    | CanonExec             | unchanged                                                 |
| 0x17–0x1F | Reserved            |                                                           |
| 0x20    | CanonView             | RESERVED for future lazy materialized views               |

### 6.1 CapabilityGrant v2 — Now Fully Specified

```text
CapabilityGrant ::= {
  target:          CanonHash-81          // object or tree granted
  permissions:     u16                   // bitfield (rwx-search-exec)
  granted_by:      CanonHash-81          // public key or prior grant
  expires_at:      u64                   // T81 chain height, 0 = never
  revocable_by:    CanonHash-81          // hash of revoker key/grant
  signature:       81-tryte ed448-ph   // or future QR sig
}
```

### 6.2 CanonLink — Optional Display Hint

```text
CanonLink ::= {
  target:          CanonHash-81
  display_hint:    Option<UTF-8 String> ≤ 255 trytes
}
```
`display_hint` is purely cosmetic for FUSE/UX; it MUST NOT affect the object’s CanonHash-81.

---

# 7. CanonParity — Enhanced (still Reed–Solomon)

Wire format (updated):

```text
CanonParity ::= [
  0x11,
  data_count:     u16,
  parity_count:   u8,
  set_merkle_root: CanonHash-81,        // NEW — O(1) set membership proof
  shard_idx:      u8,
  targets:        CanonHash-81[data_count],
  parity_data:    729 trytes
]
```

Implementations MUST verify `set_merkle_root` covers all listed targets before reconstruction.

---

# 8. Compression Selection Heuristic (Reference Impl Requirement)

If no explicit policy is given, `canonfs-rs` MUST choose algorithm as follows:

```rust
if entropy_9 ≥ 7.8 trit/bit → Z3std
else if repetitiveness ≥ 0.42 → LZ81
else → Raw
```

This heuristic will be part of the official test suite.

---

# 15. Storage Semantics — Explicit Delete via Revocation

- Physical deletion remains forbidden.
- Logical deletion = publish a `CapabilityRevoke` object whose hash is added to the revoker’s revocation list.
- All mounts automatically treat revoked capabilities as ENOENT.
- Garbage collection may eventually drop unreachable objects.

---

# 18. Compatibility Layer — FUSE Behavior (v0.4.1)

Special directories under every mount:

```
.canon/meta/      → CanonMeta xattrs
.canon/index/     → live query interface
.canon/parity/    → manual recovery tools
.canon/raw/       → force-uncompressed view
```

These directories are hidden by default. They appear only when the environment variable `CANONFS_DEBUG=1` is set or the mount flag `-o debug` is used.

---

# 19. Future Extensions (unchanged, except explicit reservation)

- 0x20 CanonView — lazy, content-addressed, capability-constrained materialized views (the “SQL for CanonFS”)
- CanonSync — eventual-consistency CRDT layer
- CanonNet — global parity-gossiping P2P fabric

---
