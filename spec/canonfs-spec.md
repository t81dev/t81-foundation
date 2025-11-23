**canonfs-spec.md — Version 0.3 (Integrated Final Draft)**

**Status: Final Draft — Ready for Reference Implementation**
**Authors: T81 Foundation + Grok (xAI)**
**Date: November 22, 2025**

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
5. [CanonHash-81 — Concrete Definition](#5-canonhash-81--concrete-definition)  
6. [CanonObject — Sealed Envelope](#6-canonobject--sealed-envelope)  
7. [CanonRef](#7-canonref)  
8. [Capability System v2](#8-capability-system-v2)  
9. [CanonFile — Merkle-81 Tree](#9-canonfile--merkle-81-tree)  
10. [CanonDirectory — Sparse 81×81 Tensor](#10-canondirectory--sparse-81x81-tensor)  
11. [Deterministic Indexing Function f(hash)](#11-deterministic-indexing-function-fhash)  
12. [CanonSnapshot](#12-canonsnapshot)  
13. [CanonGraph — The Global DAG](#13-canongraph--the-global-dag)  
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
- **Base-81** uses the canonical alphabet  
  `0123456789ABCDEFGHJKMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz-_!@#$%^&*+=<>?`
- **Trit** = ternary digit  
- **Tryte** = 6 trits = 729 possible values  

---

# 3. Architectural Invariants

CanonFS enforces three fundamental laws:

1. **Immutability** — No object is ever mutated.  
2. **Content Addressing** — Identity = CanonHash-81 of serialized form.  
3. **Capability Binding** — Access requires an unforgeable CanonRef.

Violation of any invariant is a protocol error.

---

# 4. CanonBlock

- Size: **exactly 729 bytes** (3⁶)  
- Payload: **721 bytes of data** + **8-byte metadata tag**  
- Identity: `CanonHash-81(block)`  

Blocks are sealed once created and become permanently immutable.

---

# 5. CanonHash-81 — Concrete Definition

```

CanonHash-81(data) := base81_encode( BLAKE3(data)[0..60] )

```

- 60 raw bytes → 81 Base-81 symbols  
- Hash strength: ≥ 256-bit (BLAKE3 guarantees)  
- Mandatory for CanonFS 0.3–1.0  
- Future upgrades require a version prefix  

---

# 6. CanonObject — Sealed Envelope

Objects begin with a 1-byte type tag:

| Type ID | Object Type       | Payload Format                         |
|---------|-------------------|----------------------------------------|
| 0x00    | RawBlock          | 729-byte block                         |
| 0x01    | FileNode          | leaf block OR [81 × CanonHash-81]      |
| 0x02    | Directory         | sparse tensor                          |
| 0x03    | Snapshot          | CanonHash-81 of root directory         |
| 0x04    | CapabilityGrant   | signed capability token                |
| 0x05    | CapabilityRevoke  | signed anti-token                      |

Identity = `CanonHash-81(type || payload)`  
→ prevents type-confusion attacks.

---

# 7. CanonRef

```

CanonRef ::= {
cap: CapabilityToken | null,
hash: CanonHash-81
}

```

A bare hash confers **zero rights**.  
Capabilities are required for access.

---

# 8. Capability System v2

## Rights

| Bit | Right | Meaning                        |
|-----|-------|--------------------------------|
| R   | Read  | Read/traverse object           |
| W   | Write | Create new versions            |
| A   | Admin | Delegate or revoke capabilities |

See [Appendix B](#appendix-b-capability-token-format-v2) for full token format.

---

# 9. CanonFile — Merkle-81 Tree

- Leaves: 729-byte CanonBlocks  
- Interior nodes: 81 child hashes  
- Root = CanonHash-81 of the root node  

Each write produces a new file version.

---

# 10. CanonDirectory — Sparse 81×81 Tensor

```

Directory ::= {
entries: Map<(u8,u8), CanonRef>
}

```

Serialized as:

```

[count: u16]
[(a: u8)(b: u8) CanonRef] × count

```

- Maximum entries: **6561**  
- Typical use: < 100 entries  

---

# 11. Deterministic Indexing Function f(hash)

```

a = base81_decode(hash[0])
b = base81_decode(hash[1])

```

This assigns each object to a deterministic coordinate in the 81×81 grid.

---

# 12. CanonSnapshot

A snapshot is simply:

```

CanonHash-81(root_directory)

```

Properties:

- atomic  
- immutable  
- global namespace anchor  
- version identifier  

---

# 13. CanonGraph — The Global DAG

CanonFS forms a single immutable DAG composed of all CanonObjects.

- Nodes: sealed objects  
- Edges: CanonRefs  
- Cycles: not allowed  

---

# 14. Revocation and Negative Capabilities

**CapabilityRevoke** objects (type `0x05`) are signed anti-tokens.

- Target: one CanonRef or capability chain  
- Effect: immediate denial of access  
- Propagation: via gossip/subscription  

Enables minimal-trust, zero-deletion security.

---

# 15. Security Model

CanonFS guarantees:

- **Integrity** — via CanonHash-81  
- **Authenticity** — via Ed448/T81 signatures  
- **Confidentiality** — optional (future)  
- **Availability** — delegated to storage layer  

Hash verification is REQUIRED on every read.

---

# 16. Wire Formats

### RawBlock
```

[ 729 raw bytes ]

```

### FileNode
```

[ child_count: u8 ]
[ CanonHash-81 ] × child_count

```

### Directory
```

[ entry_count: u16 ]
[ (a:u8)(b:u8) CanonRef ] × entry_count

```

### Snapshot
```

CanonHash-81 (81 chars)

````

### Capability
CBOR-encoded token (Appendix B)

---

# 17. Storage Semantics

Implementations MUST:

- use append-only or content-addressed stores  
- never mutate serialized objects  
- verify hashes lazily or eagerly  
- preserve byte-level fidelity  

Deduplication is allowed and encouraged.

---

# 18. Operational Semantics

- **Write →** new objects only  
- **Update →** create new version  
- **Delete →** capability revocation  
- **Read →** capability + hash verification  
- **List →** enumerate directory tensor entries  

---

# 19. Reference Implementation Requirements

The canonical implementation MUST:

- be written in **Rust**  
- implement BLAKE3 → Base-81 hashing  
- support capability issuance and revocation  
- support FUSE mounting and HTTP interfaces  
- verify all hashes on read  
- preserve exact object byte-format  

Target deliverable: **canonfs-rs 1.0** (Q4 2026).

---

# 20. Compatibility Layer

POSIX interop:

- **FUSE mount:** `canonfs-fuse`  
- **Path translator:** pathname → CanonRef  
- **Sidecar store:** `.canon/` for capability tokens  

---

# 21. Future Extensions

Reserved for:

- CanonHash-81 v2 (Poseidon-81, STARK-based)  
- Per-object encryption  
- CanonNet (global peer-to-peer CanonFS fabric)  
- CanonSync (CRDT-based snapshot merging)  
- Ternary-native compression  

---

# Appendix A: CanonHash-81 (BLAKE3-base81)

```rust
fn canonhash81(data: &[u8]) -> String {
    let hash = blake3::hash(data);
    base81_encode(&hash.as_bytes()[..60])
}
````

**60 bytes → 81 Base-81 symbols → ≥ 480-bit entropy.**

---

# Appendix B: Capability Token Format v2

```cbor
{
  "v": 2,
  "obj": "hash81:...",
  "rights": "RWA",              // string of rights
  "parent": "hash81:..." | null,
  "issuer": "hash81:...",
  "exp": uint64 | null,
  "sig": <Ed448 signature>
}
```

---

# Appendix C: Directory Tensor Examples

```
File: "hello.txt" → hash81: A9b3H...

a = decode('A') = 10
b = decode('9') = 9

dir.entries[(10,9)] = CanonRef {
    cap: …,
    hash: "A9b3H..."
}
```

Deterministic, sparse, elegant.

---

# CanonFS 0.3 is now complete.

The ternary filesystem has arrived.
Let’s code.
