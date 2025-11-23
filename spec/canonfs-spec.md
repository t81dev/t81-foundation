Below is the **fully formatted, GitHub-ready `canonfs-spec.md`**, clean, complete, and ready to drop directly into your `spec/` folder.

This is the **canonical Markdown implementation** of **CanonFS Master Specification v0.2**, with headings, anchors, tables, and formatting optimized for GitHub’s renderer.

No filler. No drift. No compromise.

---

# `canonfs-spec.md`

```markdown
---
title: CanonFS Master Specification
version: 0.2
status: Draft — Standards Track
author: T81 Foundation
---

# CanonFS — Canonical File System for Ternary Machines  
**Version:** 0.2  
**Category:** Standards Track  
**Status:** Integrated Draft  
**Applies to:** T81 Architecture, HanoiVM, Axion Kernel

---

# Table of Contents

1. [Abstract](#1-abstract)  
2. [Conventions and Terminology](#2-conventions-and-terminology)  
3. [Architectural Overview](#3-architectural-overview)  
4. [CanonBlock](#4-canonblock)  
5. [CanonHash-81](#5-canonhash-81)  
6. [CanonRef](#6-canonref)  
7. [Capability System](#7-capability-system)  
8. [CanonFile](#8-canonfile)  
9. [Merkle-81 Construction](#9-merkle-81-construction)  
10. [CanonDirectory Tensor](#10-canondirectory-tensor)  
11. [Indexing Function `f(hash)`](#11-indexing-function-fhash)  
12. [CanonSnapshot](#12-canonsnapshot)  
13. [CanonGraph](#13-canongraph)  
14. [Security Model](#14-security-model)  
15. [Wire Formats](#15-wire-formats)  
16. [Encoding and Storage Semantics](#16-encoding-and-storage-semantics)  
17. [Operational Semantics](#17-operational-semantics)  
18. [Reference Implementation Requirements](#18-reference-implementation-requirements)  
19. [Compatibility](#19-compatibility)  
20. [Future Extensions](#20-future-extensions)  
21. [Appendix A: CanonHash-81 Pseudocode](#appendix-a-canonhash-81-pseudocode)  
22. [Appendix B: Capability Token Format](#appendix-b-capability-token-format)  
23. [Appendix C: Directory Tensor Examples](#appendix-c-directory-tensor-examples)  
24. [Appendix D: Distributed CanonFS](#appendix-d-distributed-canonfs)  

---

# 1. Abstract

CanonFS is the canonical file system for T81-class ternary computers. It is:

- immutable  
- content-addressed  
- capability-secured  
- Merkle-81 structured  
- block-aligned to 729-byte tensors  

This specification defines the full CanonFS architecture: blocks, hashing, references, capabilities, directories, snapshots, graph semantics, security model, wire formats, and implementation requirements.

---

# 2. Conventions and Terminology

The keywords **MUST**, **MUST NOT**, **SHOULD**, and **MAY** are interpreted as in RFC 2119.

**Terms:**

- **Base-81 Alphabet** — canonical 81-symbol set  
- **CanonBlock** — 729-byte atomic storage unit  
- **CanonRef** — (capability, hash) pair  
- **CanonFile** — immutable Merkle-81 tree  
- **CanonDirectory** — deterministic 81×81 grid of references  
- **Capability Mask** — bitmask of R/W/A  
- **Snapshot** — hash of root directory  
- **CanonGraph** — global immutable DAG of all objects  

---

# 3. Architectural Overview

CanonFS is defined by three invariants:

1. **Immutability** — No data structure ever mutates in place.  
2. **Content Addressing** — Identity is derived from CanonHash-81.  
3. **Capability Control** — Access requires possessing an unforgeable token.

This produces a functional, deterministic, reproducible filesystem with perfect lineage and cryptographic integrity.

---

# 4. CanonBlock

**Size:** Exactly **729 bytes (3⁶)**  
**Structure:** 9 × 9 × 9 ternary tensor  
**Identity:** CanonHash-81(block)

### Requirements

- MUST be exactly 729 bytes.  
- MUST NOT be compressed, padded, or transformed.  
- MUST be immutable.  
- MUST hash to an 81-character Base-81 string.  

---

# 5. CanonHash-81

A ternary-native hash function.

```

CanonHash-81(Block) → Base81[81 symbols]

```

### Properties

- MUST be collision-resistant.  
- MUST be deterministic across architectures.  
- MUST support Merkle inclusion proofs.  
- SHOULD use a sponge construction.  

See [Appendix A](#appendix-a-canonhash-81-pseudocode) for pseudocode.

---

# 6. CanonRef

A CanonRef binds identity to authority:

```

CanonRef := {
cap: Capability,
hash: CanonHash-81
}

```

A CanonRef replaces:

- file paths  
- inode numbers  
- POSIX permissions  

Canonical addressing is capability + content, never strings.

---

# 7. Capability System

### Capabilities

| Bit | Meaning |
|----|---------|
| R  | Read    |
| W  | Write (new versions only) |
| A  | Admin (grant/revoke) |

### Rules

- Capabilities MUST be unforgeable.  
- Capabilities MAY be delegated.  
- Revocation MUST deny access without deleting data.  

Token format: see [Appendix B](#appendix-b-capability-token-format).

---

# 8. CanonFile

A CanonFile is a Merkle-81 tree:

- leaves: blocks (729 bytes)  
- interior nodes: groups of 81 child hashes  
- root: CanonHash-81  

### Versioning

Writes produce:

```

v0 → v1 → v2 → ...

```

Never deltas. Never mutation.

---

# 9. Merkle-81 Construction

Algorithm:

1. Split file into 729-byte blocks  
2. Hash each block  
3. Group hashes in sets of 81  
4. Hash each group  
5. Repeat until a single root node exists  

Branching factor MUST be 81.

---

# 10. CanonDirectory Tensor

A CanonDirectory is always:

```

81 × 81 tensor
dir[a][b] = CanonRef | ∅

```

### Requirements

- MUST have 6561 slots.  
- MUST be immutable.  
- MUST use deterministic placement via `f(hash)`.

---

# 11. Indexing Function `f(hash)`

```

f(hash) → (a, b)

```

Where:

- `a` = Base-81 decode of hash[0]  
- `b` = Base-81 decode of hash[1]  

Function MUST be deterministic and stable.

Examples: [Appendix C](#appendix-c-directory-tensor-examples).

---

# 12. CanonSnapshot

A snapshot is:

```

Snapshot = CanonHash-81(root_directory)

```

Snapshots:

- MUST be produced for every write.  
- MUST be immutable.  
- MUST be globally addressable.  

Optionally track a **CanonEpoch** counter.

---

# 13. CanonGraph

CanonFS forms a single immutable DAG:

- nodes = blocks, nodes, directories, snapshots  
- edges = CanonRefs  

Rules:

- MUST remain acyclic.  
- MUST maintain referential integrity.  
- MUST be cryptographically sealed.

---

# 14. Security Model

### Threats

- forgery of references  
- capability theft  
- tampering with stored data  
- rollback attacks  

### Guarantees

- immutability  
- content-address integrity  
- capability isolation  
- lineage verification  
- zero mutable global state  

### Requirements

- MUST zeroize revoked capabilities.  
- MUST validate hashes on read.  
- MUST reject malformed CanonRefs.  

---

# 15. Wire Formats

### Block Wire Format

```

[721-byte payload][8-byte metadata tag]

```

### Node Wire Format

```

[81 × CanonHash-81]

```

### Directory Wire Format

```

[81 × 81 CanonHash-81]

```

### Snapshot Wire Format

```

[ CanonHash-81(root) ]

````

### Capability Wire Format

See Appendix B.

---

# 16. Encoding and Storage Semantics

Implementations MUST:

- store objects in an append-only log or content-addressed store  
- preserve 729-byte block boundaries  
- avoid transformations that change identity  

---

# 17. Operational Semantics

Implementations MUST:

- guarantee deterministic read/write behavior  
- produce a new snapshot on each write  
- reject mutated objects  
- verify CanonHash-81 eagerly  

---

# 18. Reference Implementation Requirements

A reference CanonFS SHOULD:

- be written in Rust  
- use memory-mapped append-only storage  
- support capability issuance/revocation APIs  
- support directory tensor operations  
- provide a canonical HTTP/IPC interface  
- verify hashes on read  

Optional:

- encrypted block stores  
- deduplication  
- distributed CanonFS (Appendix D)

---

# 19. Compatibility

Binary filesystems (ext4, ZFS, APFS) MUST interact through:

- a FUSE mount layer  
- path-to-CanonRef adapters  
- POSIX emulation wrappers  

---

# 20. Future Extensions

Reserved for:

- CanonNet (networked replication)  
- CanonSync (snapshot convergence)  
- CanonChain (global append-only fabric)  
- Ternary-safe compression  

---

# Appendix A: CanonHash-81 Pseudocode

```text
state = init_sponge()

for chunk in ternary_chunks(input):
    state = absorb(state, chunk)

return squeeze(state, 81_symbols)
````

---

# Appendix B: Capability Token Format

```text
cap = {
  bits: [R,W,A],
  issuer: CanonHash-81,
  nonce: 128-bit random,
  signature: Ed448 or T81-native scheme
}
```

---

# Appendix C: Directory Tensor Examples

```text
hash = "A9b3H..."

a = decode_base81('A')
b = decode_base81('9')

dir[a][b] = CanonRef(hash)
```

---

# Appendix D: Distributed CanonFS

Future model includes:

* CRDT-based snapshot merging
* gossip dissemination
* capability revocation propagation
* canonical event ordering without timestamps

```

---

Just indicate which one you want next.
```
