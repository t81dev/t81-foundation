```markdown
# canonfs-spec.md — Version 0.4.1  
**Integrated Final Draft — Ready for Reference Implementation**  
**Authors:** T81 Foundation + Grok (xAI)  
**Date:** November 22, 2025 (updated same day)

---
title: CanonFS — Canonical File System for Ternary Machines
version: 0.4.1
status: Final Draft — Reference Implementation Authorized
author: T81 Foundation + Grok (xAI)
created: 2025-11-22
updated: 2025-11-22
---

# CanonFS — Canonical File System for Ternary Machines

**Version:** 0.4.1  
**Category:** Standards Track  
**Status:** Final Draft — Green-lit for canonfs-rs 1.0  
**Target v1.0 release:** Q4 2026  
**Applies to:** T81 Architecture, HanoiVM, Axion Kernel, CanonNet, T243–T19683 tiers

# 1. Abstract

CanonFS is the immutable, content-addressed, capability-secured, self-healing, compressible, and searchable filesystem native to ternary (T81-class) computing.

It replaces paths, inodes, POSIX permissions, and mutable state with a single primitive: the CanonRef — a cryptographically bound triple of capability + content hash + optional parity/encryption envelope.

CanonFS guarantees perfect provenance, global uniqueness, verifiable integrity, automatic recovery, tryte-native compression, and deterministic indexing without ever violating immutability.

# 2. Architectural Invariants (non-negotiable)

1. Immutability — No object is ever mutated.  
2. Content Addressing — Identity = CanonHash-81 of serialized form.  
3. Capability Binding — Access requires an unforgeable CanonRef.  
4. Deterministic Indexing — f(hash) is stable forever.  
5. Self-Healing — Every write may emit parity for automatic recovery.

# 3. CanonBlock — Fixed 729-tryte Block

- Exact size: 729 trytes (3⁶ trytes = one “block”)  
- Payload: 721 trytes + 8-tryte metadata tag  
- Identity: CanonHash-81(block)  
- All forms below hash to the same value when decompressed/decrypted:

| Form        | Tag Range | Description                                      |
|-------------|-----------|--------------------------------------------------|
| Raw         | 0x00–0x0F | Uncompressed, direct payload                     |
| Compressed  | 0x10–0x1F | LZ81 or Z3std compressed payload                 |
| Encrypted   | 0x20–0x2F | CanonSeal AEAD envelope                          |

# 4. CanonHash-81

Unchanged: BLAKE3-base81, exactly 81 Base-81 symbols, ≥480-bit security.

# 5. Object Types (v0.4.1)

| Type ID | Object Type           | Description / v0.4.1 Notes                                      |
|---------|-----------------------|-----------------------------------------------------------------|
| 0x00    | RawBlock              | 729-tryte block                                                |
| 0x01    | FileNode              | Merkle-81 tree node                                            |
| 0x02    | Directory             | Sparse 81×81 tensor                                            |
| 0x03    | Snapshot              | CanonHash-81 of root directory                                 |
| 0x04    | CapabilityGrant       | v2 — fully specified (§6.1)                                    |
| 0x05    | CapabilityRevoke      | Tombstone; canonical delete mechanism                          |
| 0x10    | CompressedBlock       | [algo][compressed payload]                                     |
| 0x11    | CanonParity           | Reed–Solomon parity with set_merkle_root (§7)                  |
| 0x12    | CanonIndex            | Sparse inverted index tensor                                   |
| 0x13    | CanonMeta             | Sparse key→value tensor (xattr compatible)                     |
| 0x14    | CanonSeal             | Encrypted + authenticated envelope (T81-AEAD-81)               |
| 0x15    | CanonLink             | Symbolic link; optional UTF-8 display_hint (§6.2)              |
| 0x16    | CanonExec             | Executable metadata + entry point                              |
| 0x17–0x1F | Reserved            |                                                                |
| 0x20    | CanonView             | RESERVED — future lazy materialized views                      |

## 6.1 CapabilityGrant v2 (fully specified)

```text
CapabilityGrant ::= {
  target:         CanonHash-81    // object or subtree granted
  permissions:    u16             // bitfield: rwx-search-exec
  granted_by:     CanonHash-81    // public key or prior grant hash
  expires_at:     u64             // T81 chain height, 0 = never
  revocable_by:   CanonHash-81    // entity that can revoke this grant
  signature:      81-tryte ed448-ph (or future QR scheme)
}
```

## 6.2 CanonLink (optional display hint)

```text
CanonLink ::= {
  target:         CanonHash-81
  display_hint:   Option<UTF-8 String> ≤ 255 trytes   // cosmetic only
}
```
display_hint MUST NOT affect CanonHash-81.

# 7. CanonParity — Reed–Solomon Recovery (enhanced)

Default policy: 3 data + 2 parity (survives any 2 losses).

Wire format (v0.4.1):

```text
CanonParity ::= [
  0x11,
  data_count:       u16,
  parity_count:     u8,
  set_merkle_root:  CanonHash-81,    // NEW — O(1) set validation
  shard_idx:        u8,
  targets:          CanonHash-81[data_count],
  parity_data:      729 trytes
]
```

Implementations MUST verify set_merkle_root before reconstruction.

# 8. Tryte-Native Compression

| Algo   | Code | Target Ratio | Speed  | Use Case                  |
|--------|------|--------------|--------|---------------------------|
| LZ81   | 0x10 | ~2.8×        | Fast   | General files             |
| Z3std  | 0x11 | ~4.2×        | Medium | Large tensors, logs, etc. |

Compression occurs before hashing → identical logical content always shares CanonHash-81.

Reference implementation heuristic (if no policy):

```rust
if entropy_9 ≥ 7.8 trit/bit → Z3std
else if repetitiveness ≥ 0.42 → LZ81
else → Raw
```

# 9. CanonIndex — Sparse 81×81 Inverted Tensor

Optional per-directory or per-snapshot. Maps term → list of (CanonRef, offset). Terms are CanonHash-81 of n-grams or tokens. Typically <5 KB.

# 10. CanonMeta — Sparse Extended Metadata Tensor

Key and value are both CanonHash-81 (content-addressed strings). Fully versioned, deterministic.

# 11. CanonSeal — Per-Object Encryption

T81-AEAD-81 (ternary ChaCha20-Poly1305 variant). Key = KDF(capability ‖ object_hash).

# 12. CanonFile — Merkle-81 Tree

Leaves may be Raw, Compressed, or Sealed blocks (all participate uniformly).

# 13. CanonDirectory — Sparse 81×81 Tensor

```text
Directory ::= {
  entries:   Map<(u8,u8), CanonRef>    // name → object
  index:     CanonRef | null
  meta:      CanonRef | null
  parity:    [CanonRef]                // optional recovery shards
}
```

# 14. Wire Formats (selected, updated)

| Object            | Wire Format                                                                 |
|-------------------|-----------------------------------------------------------------------------|
| CompressedBlock   | [0x10][algo u8][compressed_len u16][compressed bytes]                       |
| CanonParity       | [0x11][data_count u16][parity_count u8][set_merkle_root][shard_idx u8][targets…][parity] |
| CanonIndex        | [0x12][term_count u32][(term_hash CanonRef×offsets)…]                       |
| CanonMeta         | [0x13][pair_count u16][(key_hash value_hash)…]                              |
| CanonSeal         | [0x14][nonce 24 trytes][ciphertext][tag 16 trytes]                          |

All integers little-endian.

# 15. Storage Semantics — Self-Healing & Delete

Implementations MUST:
- Verify CanonHash-81 on every read
- Automatically reconstruct from CanonParity
- Decompress/decrypt transparently
- Rebuild missing indices on-the-fly
- Never expose corruption

Logical deletion = publish CapabilityRevoke tombstone (objects become ENOENT).

# 16. Operational Semantics

| Operation | Behavior in v0.4.1                                          |
|---------|-------------------------------------------------------------|
| Write   | → new objects + optional parity + optional index           |
| Read    | → hash + capability + auto-repair + decompress/decrypt     |
| Search  | → CanonIndex tensor (brute fallback)                        |
| Delete  | → publish CapabilityRevoke (tombstone)                      |
| Recover | fully automatic via CanonParity                             |

# 17. Reference Implementation Requirements (canonfs-rs 1.0)

MUST implement:
- Rust + BLAKE3 → Base-81
- LZ81 & Z3std compressors with heuristic
- Reed–Solomon GF(3⁹) + CanonParity v2
- Sparse CanonIndex & CanonMeta builders
- FUSE + HTTP gateways
- Background repair daemon
- Axion policy hooks

# 18. Compatibility Layer — FUSE

Exposes:
- `.canon/meta/`, `.canon/index/`, `.canon/parity/`, `.canon/raw/`

These are hidden by default; visible only with `CANONFS_DEBUG=1` or `-o debug`.

# 19. Future Extensions

- 0x20 CanonView — content-addressed lazy materialized views
- CanonSync — CRDT convergence
- CanonNet — global P2P parity-gossip fabric
- Quantum-resistant signatures

# Appendix A: Recommended Default Policies

| Workload              | Compression | Parity (data+parity) | Index |
|-----------------------|-------------|----------------------|-------|
| General files         | LZ81        | 3+2                  | Yes   |
| Large tensors/logs    | Z3std       | 6+3                  | Yes   |
| Cognitive graphs      | Z3std       | 9+3                  | Full  |
| Cold archival         | Z3std       | 10+10                | No    |

---
