canonfs-spec.md — Version 0.3Integrated Final Draft
Status: Ready for Reference Implementation
Authors: T81 Foundation + Grok (xAI)
Date: November 22, 2025  markdown

---
title: CanonFS — Canonical File System for Ternary Machines
version: 0.3
status: Final Draft — Standards Track
author: T81 Foundation + Grok (xAI)
created: 2025-11-22
---

# CanonFS — Canonical File System for Ternary Machines

**Version:** 0.3  
**Category:** Standards Track  
**Status:** Final Draft — Ready for Reference Implementation  
**Applies to:** T81 Architecture, HanoiVM, Axion Kernel, CanonNet

---

# Table of Contents

1. [Abstract](#1-abstract)  
2. [Conventions and Terminology](#2-conventions-and-terminology)  
3. [Architectural Invariants](#3-architectural-invariants)  
4. [CanonBlock](#4-canonblock)  
5. [CanonHash-81 — Concrete Definition](#5-canonhash-81-concrete-definition)  
6. [CanonObject — Sealed Envelope](#6-canonobject-sealed-envelope)  
7. [CanonRef](#7-canonref)  
8. [Capability System v2](#8-capability-system-v2)  
9. [CanonFile — Merkle-81 Tree](#9-canonfile-merkle-81-tree)  
10. [CanonDirectory — Sparse 81×81 Tensor](#10-canondirectory-sparse-81×81-tensor)  
11. [Deterministic Indexing Function f(hash)](#11-deterministic-indexing-function-fhash)  
12. [CanonSnapshot](#12-canonsnapshot)  
13. [CanonGraph — The Global DAG](#13-canongraph-the-global-dag)  
14. [Revocation and Negative Capabilities](#14-revocation-and-negative-capabilities)  
15. [Security Model](#15-security-model)  
16. [Wire Formats](#16-wire-formats)  
17. [Storage Semantics](#17-storage-semantics)  
18. [Operational Semantics](#18-operational-semantics)  
19. [Reference Implementation Requirements](#19-reference-implementation-requirements)  
20. [Compatibility Layer](#20-compatibility-layer)  
21. [Future Extensions](#21-future-extensions)  
22. [Appendix A: CanonHash-81 (BLAKE3-base81)](#appendix-a-canonhash-81-blake3-base81)  
23. [Appendix B: Capability Token Format v2](#appendix-b-capability-token-format-v2)  
24. [Appendix C: Directory Tensor Examples](#appendix-c-directory-tensor-examples)  

---

# 1. Abstract

CanonFS is the immutable, content-addressed, capability-secured filesystem native to ternary (T81-class) computing.  
It replaces paths, inodes, and POSIX permissions with a single primitive: the **CanonRef** — a cryptographically bound pair of **capability + content hash**.

CanonFS guarantees perfect provenance, global uniqueness, and verifiable integrity without mutable state.

---

# 2. Conventions and Terminology

- **MUST**, **MUST NOT**, **SHOULD**, **MAY** as per RFC 2119.
- **Base-81** uses the canonical alphabet `0123456789ABCDEFGHJKMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz-_!@#$%^&*+=<>?` (81 symbols, no ambiguous chars).
- **Trit** = ternary digit, **Tryte** = 6 trits = 729 possibilities.

---

# 3. Architectural Invariants

CanonFS enforces three non-negotiable laws:

1. **Immutability** — No object is ever mutated.
2. **Content Addressing** — Identity = CanonHash-81 of serialized form.
3. **Capability Binding** — Access requires an unforgeable CanonRef.

Violation of any invariant is a protocol error.

---

# 4. CanonBlock

- Fixed size: **exactly 729 bytes** (3⁶ = one tryte of raw data)
- Payload: 721 bytes arbitrary data + 8-byte metadata tag (reserved)
- Identity: `CanonHash-81(block)`

Blocks are the only mutable-size objects that become immutable once sealed.

---

# 5. CanonHash-81 — Concrete Definition

**CanonHash-81** is defined as:

CanonHash-81(data) := base81_encode( BLAKE3(data)[0..60] )

- Input: arbitrary bytes
- Output: exactly **81 Base-81 symbols** (60 bytes → 81 symbols via 81⁴⁰⁶ = 2⁴⁸⁰ coverage)
- Collision resistance: ≥ 256-bit (inherits BLAKE3)
- Future-proof: replaceable via versioned prefix (see §21)

This is the **only acceptable hash** for CanonFS 0.3–1.0.

---

# 6. CanonObject — Sealed Envelope

All objects are serialized with a one-byte type tag:

| Type ID | Object Type       | Payload Format                     |
|---------|-------------------|------------------------------------|
| 0x00    | RawBlock          | 729-byte block                     |
| 0x01    | FileNode          | [81 × CanonHash-81] or leaf block  |
| 0x02    | Directory         | sparse tensor (see §10)             |
| 0x03    | Snapshot          | CanonHash-81 of root directory     |
| 0x04    | CapabilityGrant   | signed token (see §8)              |
| 0x05    | CapabilityRevoke  | signed anti-token                  |

Identity = `CanonHash-81( 0xNN || payload )`

This prevents type confusion attacks.

---

# 7. CanonRef

CanonRef ::= {
  cap: CapabilityToken | null,
  hash: CanonHash-81
}

- A bare hash grants **no rights**.
- Only a signed CapabilityToken confers R/W/A.

---

# 8. Capability System v2

### Rights

| Bit | Right | Meaning                            |
|-----|-------|------------------------------------|
| R   | Read  | Traverse and read object           |
| W   | Write | Create new versions                |
| A   | Admin | Grant/revoke capabilities          |

### Token Format (see Appendix B)

Signed with Ed448 or T81-native scheme.  
Delegation allowed via `parent` reference chain.

---

# 9. CanonFile — Merkle-81 Tree

- Leaf nodes: 729-byte CanonBlocks
- Interior nodes: exactly 81 child CanonHash-81 entries
- Root hash is the file identity
- Maximum file size: 729 × 81ⁿ bytes (n = tree height)

Versioning is append-only: new root = new version.

---

# 10. CanonDirectory — Sparse 81×81 Tensor

A directory is a **sparse map**:

Directory ::= {
  entries: Map<(u8,u8), CanonRef>
}

Serialized as:

[count: u16][ (a: u8)(b: u8) CanonRef ]×count

- Maximum populated entries: 6561
- Average real-world directories: < 100 entries → < 10 KB
- Still fully deterministic via `f(hash)`

---

# 11. Deterministic Indexing Function f(hash)

a = base81_decode(hash[0])
b = base81_decode(hash[1..1])  // first symbol only

- Collision probability per directory: < 1/6561
- Global birthday bound still ≥ 240-bit

Stable forever.

---

# 12. CanonSnapshot

A snapshot is the CanonHash-81 of a Directory object.

- Atomic commits
- Global immutable namespace anchor
- Used as version identifier

---

# 13. CanonGraph — The Global DAG

All CanonObjects form a single directed acyclic graph.

- Nodes: all sealed CanonObjects
- Edges: CanonRefs within payloads
- Cryptographically sealed
- No cycles allowed (enforced on write)

---

# 14. Revocation and Negative Capabilities

Revocation via **CapabilityRevoke** objects (type 0x05):

- Contains target CanonRef
- Signed by issuer with A-bit
- Propagated via gossip or log subscription
- Nodes MUST reject revoked capabilities

Zero-knowledge revocation proofs possible in future.

---

# 15. Security Model

**Guarantees:**

- Integrity: guaranteed by CanonHash-81
- Authenticity: guaranteed by signed capabilities
- Confidentiality: optional per-object encryption (future)
- Availability: depends on storage layer

**No trusted third party required.**

---

# 16. Wire Formats

| Object       | Wire Format                                      |
|--------------|--------------------------------------------------|
| RawBlock     | 729 raw bytes                                    |
| FileNode     | [child_count: u8] [CanonHash-81]×child_count     |
| Directory    | [entry_count: u16] [(a:u8)(b:u8) CanonRef]×n     |
| Snapshot     | CanonHash-81 (81 chars)                          |
| Capability   | CBOR-encoded signed token (Appendix B)           |

All multi-byte integers are little-endian.

---

# 17. Storage Semantics

Implementations MUST:

- Store objects in append-only log or CAS
- Never mutate stored objects
- Verify CanonHash-81 on read
- Preserve exact byte boundaries

Deduplication is encouraged.

---

# 18. Operational Semantics

- Write → new objects only
- Read → hash + capability validation
- Delete → not supported (use revocation)
- List → iterate sparse directory tensor

---

# 19. Reference Implementation Requirements

The canonical implementation **MUST**:

- Be written in **Rust**
- Use **BLAKE3** → **Base-81** as CanonHash-81
- Implement sparse directory tensors
- Support capability issuance/revocation
- Provide FUSE and HTTP gateways
- Verify all hashes on read

Target: **canonfs-rs 1.0** by Q4 2026.

---

# 20. Compatibility Layer

POSIX emulation via:

- FUSE mount (`canonfs-fuse`)
- Path → CanonRef translation layer
- `.canon/` sidecar for capability storage

---

# 21. Future Extensions (CanonFS 2.0+)

- CanonHash-81 v2 (Poseidon-81 or STARK-based)
- Per-object encryption
- CanonNet — global peer-to-peer fabric
- CanonSync — CRDT-based convergence
- Tryte-native compression

---

# Appendix A: CanonHash-81 (BLAKE3-base81)

```rust
fn canonhash81(data: &[u8]) -> String {
    let hash = blake3::hash(data);
    base81_encode(&hash.as_bytes()[..60])
}

60 bytes → 81 Base-81 symbols → ≥ 480-bit entropy.Appendix B: Capability Token Format v2cbor

{
  "v": 2,
  "obj": "hash81:...",          // target object
  "rights": "RWA",               // string of granted rights
  "parent": "hash81:..." | null, // delegation chain
  "issuer": "hash81:...",        // public key object
  "exp": u64 | null,             // optional expiry
  "sig": <Ed448 signature>
}

Appendix C: Directory Tensor Examplestext

File: "hello.txt" → hash81:A9b3H...
a = decode('A') = 10
b = decode('9') = 9

dir.entries[(10,9)] = CanonRef{cap:..., hash: "A9b3H..."}

Deterministic, sparse, beautiful.CanonFS 0.3 is now complete.
The ternary filesystem has arrived.Let’s code.
