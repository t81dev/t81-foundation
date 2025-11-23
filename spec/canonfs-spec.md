# canonfs-spec.md — Version 0.4  
**Integrated Final Draft — Ready for Reference Implementation**  
**Authors:** T81 Foundation + Grok (xAI)  
**Date:** November 22, 2025  

```markdown
---
title: CanonFS — Canonical File System for Ternary Machines
version: 0.4
status: Final Draft — Standards Track (v1.0 target Q4 2026)
author: T81 Foundation + Grok (xAI)
created: 2025-11-22
---

# CanonFS — Canonical File System for Ternary Machines

**Version:** 0.4  
**Category:** Standards Track  
**Status:** Final Draft — Ready for Reference Implementation  
**Applies to:** T81 Architecture, HanoiVM, Axion Kernel, CanonNet, T243–T19683 tiers

---

# 1. Abstract

CanonFS is the immutable, content-addressed, capability-secured, **self-healing**, **compressible**, and **searchable** filesystem native to ternary (T81-class) computing.

It replaces paths, inodes, POSIX permissions, and mutable state with a single primitive: the **CanonRef** — a cryptographically bound triple of **capability + content hash + optional parity/encryption envelope**.

CanonFS guarantees perfect provenance, global uniqueness, verifiable integrity, **automatic recovery**, **tryte-native compression**, and **deterministic indexing** without ever violating immutability.

---

# 2. Major Changes from v0.3 → v0.4

| Feature                     | v0.3          | v0.4 (this spec)                              |
|-----------------------------|---------------|-----------------------------------------------|
| Compression                 | Future only   | Native tryte-native Z3std + LZ81              |
| Data Recovery               | Detection only| Built-in Reed–Solomon parity (CanonParity)    |
| Search / Indexing           | None          | CanonIndex — sparse 81×81 inverted tensor     |
| Metadata                    | Minimal       | CanonMeta — sparse tensor, xattr compatible   |
| Encryption                  | Future        | CanonSeal — per-object AEAD (T81-AEAD-81)     |
| Object Types                | 6             | 14 (new: Compressed, Parity, Index, Meta…)   |

---

# 3. Architectural Invariants (still non-negotiable)

1. **Immutability** — No object is ever mutated.
2. **Content Addressing** — Identity = CanonHash-81 of serialized form.
3. **Capability Binding** — Access requires an unforgeable CanonRef.
4. **Deterministic Indexing** — f(hash) is stable forever.
5. **Self-Healing** — Every write may emit parity for automatic recovery.

---

# 4. CanonBlock — Fixed 729-byte Tryte

- Exact size: **729 bytes** (3⁶ = one tryte)
- Payload: 721 bytes + 8-byte metadata tag
- Identity: `CanonHash-81(block)`

Now comes in three physical forms (all hash to the same CanonHash-81 when decompressed):

| Form        | Tag Range | Description                         |
|-------------|-----------|-------------------------------------|
| Raw         | 0x00–0x0F | Uncompressed, direct payload        |
| Compressed  | 0x10–0x1F | LZ81 / Z3std-compressed payload     |
| Encrypted   | 0x20–0x2F | CanonSeal envelope (AEAD)           |

---

# 5. CanonHash-81 — Unchanged (BLAKE3-base81)

Still exactly 81 Base-81 symbols, ≥480-bit security.

---

# 6. New Object Types (v0.4)

| Type ID | Object Type           | Payload Format                                   |
|---------|-----------------------|--------------------------------------------------|
| 0x00    | RawBlock              | 729-byte block                                   |
| 0x01    | FileNode              | Merkle-81 tree node                              |
| 0x02    | Directory             | sparse 81×81 tensor                              |
| 0x03    | Snapshot              | CanonHash-81 of root directory                   |
| 0x04    | CapabilityGrant       | signed token v2                                  |
| 0x05    | CapabilityRevoke      | signed anti-token                                |
| 0x10    | CompressedBlock       | [algo][compressed 729 bytes]                     |
| 0x11    | CanonParity           | Reed–Solomon parity for N data blocks            |
| 0x12    | CanonIndex            | sparse inverted index tensor                     |
| 0x13    | CanonMeta             | sparse key→value tensor (xattr compatible)       |
| 0x14    | CanonSeal             | encrypted + authenticated envelope               |
| 0x15    | CanonLink             | symbolic link (hash only)                        |
| 0x16    | CanonExec             | executable metadata + entry point                |
| 0x17    | Reserved              | future extensions                                |

---

# 7. CanonParity — Built-in Reed–Solomon Recovery

- Every write **MAY** emit one or more CanonParity objects.
- Default policy: 3 data + 2 parity (5 total) → survives any 2 losses.
- Parity objects contain:
  - List of target CanonHash-81 (data blocks)
  - Reed–Solomon shard index
  - Recovery is fully automatic on read if ≥ threshold present.

Implementations **MUST** verify and regenerate missing blocks transparently.

---

# 8. Compression — Tryte-Native LZ81 & Z3std

Two algorithms (both deterministic, ternary-optimized):

| Algo | Code | Ratio target | Speed | Use case            |
|------|------|--------------|-------|---------------------|
| LZ81 | 0x10 | ~2.8×        | Fast  | General files       |
| Z3std| 0x11 | ~4.2×        | Medium| Large tensors, logs |

Compression occurs **before** hashing → same CanonHash-81 for compressed/uncompressed identical content.

---

# 9. CanonIndex — Sparse 81×81 Inverted Tensor

- Optional per-directory or per-snapshot index.
- Maps term → list of CanonRef (file + offset)
- Terms are CanonHash-81 of n-gram or token.
- Stored as sparse tensor, <5 KB for typical directories.
- Full-text, semantic, or metadata search all use the same primitive.

Axion may auto-generate indices for cognitive-tier objects.

---

# 10. CanonMeta — Sparse Extended Metadata Tensor

- Key and value are both CanonHash-81 (content-addressed strings).
- Supports arbitrary keys: `mtime`, `owner`, `mime`, `tags`, `provenance`, etc.
- Fully deterministic and versioned.

---

# 11. CanonSeal — Per-Object Encryption

- Uses T81-AEAD-81 (ternary-native ChaCha20-Poly1305 variant).
- Key derived from capability + object hash.
- Confidentiality without trusting storage layer.

---

# 12. CanonFile — Merkle-81 Tree (unchanged structure, now supports mixed block types)

Leaves may be Raw, Compressed, or Sealed blocks — all participate in the same Merkle-81 tree.

---

# 13. CanonDirectory — Sparse 81×81 Tensor (now richer)

```text
Directory ::= {
  entries:   Map<(u8,u8), CanonRef>          // name → object
  index:     CanonRef | null                 // optional CanonIndex
  meta:      CanonRef | null                 // optional CanonMeta
  parity:    [CanonRef]                      // optional recovery shards
}
```

Deterministic indexing function f(hash) unchanged and still perfect.

---

# 14. Wire Formats (updated)

| Object          | Wire Format                                                               |
|-----------------|---------------------------------------------------------------------------|
| CompressedBlock | [0x10][algo: u8][compressed_len: u16][compressed bytes]                  |
| CanonParity     | [0x11][data_count: u16][shard_idx: u8][targets: CanonHash-81×N][parity]  |
| CanonIndex      | [0x12][term_count: u32][(term_hash CanonRef×offsets)…]                    |
| CanonMeta       | [0x13][pair_count: u16][(key_hash value_hash)…]                           |
| CanonSeal       | [0x14][nonce: 24 bytes][ciphertext][tag: 16 bytes]                        |

All integers little-endian, all hashes CanonHash-81.

---

# 15. Storage Semantics — Now Self-Healing

Implementations **MUST**:

- Verify CanonHash-81 on read
- Automatically reconstruct missing blocks from CanonParity
- Decompress transparently
- Decrypt with valid capability
- Rebuild indices on-the-fly if missing
- Never expose corruption to userland

Deduplication, compression, and parity are all encouraged and interoperable.

---

# 16. Operational Semantics — Upgraded

| Operation | Behavior in v0.4                                          |
|---------|-----------------------------------------------------------|
| Write   | → new objects + optional parity + optional index         |
| Read    | → hash + capability + auto-repair + decompression        |
| Search  | → query CanonIndex tensor (fallback: brute if missing)   |
| Delete  | still not supported (use revocation)                     |
| Recover | fully automatic via CanonParity                           |

---

# 17. Reference Implementation Requirements (canonfs-rs 1.0)

**MUST** implement:

- Rust + BLAKE3 → Base-81
- LZ81 & Z3std compressors
- Reed–Solomon parity generation/verification
- Sparse CanonIndex builder (n-gram + metadata)
- CanonMeta xattr emulation
- FUSE + HTTP gateways
- Automatic background repair daemon
- Axion hooks for index/auto-parity policies

Target: **canonfs-rs 1.0** by Q4 2026.

---

# 18. Compatibility Layer — Now Richer

- FUSE mount exposes:
  - `.canon/meta/` for xattrs
  - `.canon/index/` for search
  - `.canon/parity/` for manual recovery
- Full POSIX emulation + extended attributes

---

# 19. Future Extensions (CanonFS 2.0+)

- CanonSync — CRDT convergence for offline nodes
- CanonNet — global P2P fabric with built-in parity gossip
- Tryte-native quantum-resistant signatures
- Axion-driven adaptive compression/parity policies

---

# Appendix A: Recommended Default Policies

| Workload              | Compression | Parity (data+parity) | Index |
|-----------------------|-------------|----------------------|-------|
| General files         | LZ81        | 3+2                  | Yes   |
| Large tensors/logs    | Z3std       | 6+3                  | Yes   |
| Cognitive graphs      | Z3std       | 9+3                  | Full  |
| Cold archival         | Z3std       | 10+10                | No    |

---
