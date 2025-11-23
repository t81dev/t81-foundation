Below is a **full, professionally structured RFC** for CanonFS, modeled on IETF style and ready for GitHub, standards-track submission, or integration into the T81 Foundation’s formal spec suite.

It is written as a complete normative document: abstract, status, terminology, normative grammar, operational semantics, security considerations, and IANA-like placeholders.

This is **CanonFS-RFC-0001** — the canonical specification.

---

# **RFC 0001 — CanonFS: Canonical File System for Ternary Machines**

**Category:** Standards Track
**Author:** T81 Foundation
**Status:** Draft
**Updates:** None
**Replaces:** None

---

# **Table of Contents**

1. Abstract
2. Conventions and Terminology
3. Architectural Overview
4. CanonBlock (Base Unit)
5. CanonHash-81 (Hash Function)
6. CanonRef (Reference Type)
7. File Semantics (CanonFile, Versioning)
8. Directory Semantics (CanonDirectory Tensor)
9. Capability System
10. Snapshot Semantics
11. CanonGraph Model
12. Wire Formats
13. Algorithms
14. Operational Semantics
15. Security Considerations
16. Interoperability and Compatibility
17. Reference Implementation Notes
18. IANA Considerations (Reserved)
19. Acknowledgments

---

# **1. Abstract**

CanonFS is a **ternary-native, immutable, content-addressed filesystem** designed for the T81 computational substrate. It provides:

* fixed-size **729-byte blocks**
* **Base-81 Merkle trees**
* immutable files and directories
* capability-secured references
* deterministic **81×81 directory tensors**
* **automatic snapshots** for every write

CanonFS replaces mutable inodes and POSIX permission models with a mathematically coherent, functional, deterministic architecture aligned to ternary arithmetic and Base-81 identity.

This RFC defines CanonFS semantics, structure, wire formats, algorithms, and security model.

---

# **2. Conventions and Terminology**

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **MAY** are interpreted as in RFC 2119.

**Terms:**

* **CanonBlock** — fixed 729-byte atomic unit of storage.
* **CanonHash-81** — deterministic Base-81 hash used as global content identity.
* **CanonRef** — (capability, hash) pair; replaces paths and permissions.
* **CanonFile** — Merkle-81 tree representing immutable file content.
* **CanonDirectory** — deterministic 81×81 tensor of CanonRefs.
* **Capability** — cryptographic token granting controlled access.
* **Snapshot** — root directory hash representing filesystem state.
* **CanonGraph** — global immutable DAG formed by all stored objects.

---

# **3. Architectural Overview**

CanonFS is a *pure functional filesystem* grounded in three principles:

### **3.1 Immutability**

No object MAY ever be modified in place.

### **3.2 Canonical Addressing**

All objects are addressed by their **content**, not a mutable pathname.

### **3.3 Capability-Based Access**

Authority is held in unforgeable tokens that bind operations to identities.

Together these properties yield perfect reproducibility, deterministic behavior, infinite snapshots, and cryptographic integrity guarantees.

---

# **4. CanonBlock**

## **4.1 Definition**

A CanonBlock is the atomic unit of storage:

* **Size:** EXACTLY **729 bytes**
* **Structure:** 9 × 9 × 9 ternary tensor
* **Identity:** CanonHash-81(block)

## **4.2 Requirements**

Implementations MUST:

* accept only 729-byte payloads
* ensure immutability
* map hash → block reliably
* preserve block boundaries exactly

Implementations MUST NOT:

* compress, pad, or otherwise modify block contents
* permit variable block size
* mutate existing blocks

---

# **5. CanonHash-81 (Hash Function)**

## **5.1 Definition**

```
CanonHash-81(Block) → Base81Hash[81 symbols]
```

## **5.2 Requirements**

The hash function MUST:

* emit EXACTLY 81 Base-81 symbols
* be collision-resistant
* be deterministic across platforms
* operate natively on ternary values
* support Merkle inclusion proofs

A sponge-based design optimized for ternary arithmetic is RECOMMENDED.

---

# **6. CanonRef**

A **CanonRef** is the fundamental addressing structure:

```
CanonRef := (Capability, CanonHash-81)
```

CanonRef simultaneously encodes:

* *identity* (hash)
* *authority* (capability)

It replaces traditional paths, permissions, and inodes.

## **6.1 Rules**

A CanonRef MUST:

* be opaque to application code
* be unforgeable
* be safely passable across processes

---

# **7. CanonFile**

## **7.1 Structure**

A CanonFile is a **Merkle-81 tree**:

```
RootNode
├── 81 children
│    ├── 81 grandchildren
│    ...
```

## **7.2 Write Semantics**

Writes MUST:

1. segment content into 729-byte blocks
2. hash each block
3. recursively build Merkle-81 nodes
4. compute root hash
5. return new CanonRef(root)

There is **no in-place mutation**.

## **7.3 Versioning**

Every write creates a new **CanonVersion**:

```
v0 → v1 → v2 → ...
```

Versions MUST be stored as full canonical hashes, not deltas.

---

# **8. CanonDirectory**

## **8.1 Structure**

A CanonDirectory is an **81×81 tensor**:

```
dir[a][b] = CanonRef | ∅
```

Indices MUST be Base-81 integers from 0 to 80 inclusive.

## **8.2 Requirements**

Implementations MUST:

* maintain the fixed grid size
* enforce deterministic indexing
* forbid in-place edits
* treat every modification as a new directory version

## **8.3 Indexing Function**

Object placement MUST be determined by:

```
f(hash) → (a,b)
```

Where `f` is:

* deterministic
* collision-resistant
* injection-preserving over hash space

---

# **9. Capability System**

## **9.1 Capability Types**

The following capabilities MUST exist:

* **READ**
* **WRITE** (append-only version creation)
* **ADMIN** (grant/revoke other capabilities)

## **9.2 Rules**

A CanonFS implementation MUST guarantee:

* capabilities are unforgeable
* capabilities MAY be delegated
* capabilities MAY be revoked
* revocation MUST deny access even if underlying data persists

Permissions do not exist outside capabilities.

---

# **10. Snapshot Semantics**

## **10.1 Definition**

A **Snapshot** is a CanonHash-81 of the root directory.

## **10.2 Behavior**

Because CanonFS is immutable:

* EVERY WRITE automatically creates a new snapshot
* snapshots are cheap (constant-size)
* snapshots are self-verifying
* snapshots form a DAG of historical states

---

# **11. CanonGraph Model**

CanonFS produces a global, immutable graph:

* **Nodes** are CanonBlocks, CanonNodes, CanonDirectories
* **Edges** are CanonRefs
* **Snapshots** are pinned roots

This graph constitutes the complete filesystem.

## **11.1 Rules**

Implementations MUST:

* preserve acyclicity
* guarantee determinism
* maintain referential integrity
* never mutate existing nodes

---

# **12. Wire Formats**

Below are RECOMMENDED canonical encodings.

## **12.1 CanonBlock Wire Format**

```
[ 721 bytes payload ][ 8 bytes metadata tag ]
```

## **12.2 CanonNode Wire Format**

```
[ 81 × CanonHash-81 entries ]
```

## **12.3 CanonDirectory Wire Format**

```
[ 81 × 81 CanonHash-81 entries ]
```

## **12.4 Snapshot Wire Format**

```
[ CanonHash-81(root) ]
```

## **12.5 Capability Wire Format**

Capabilities MAY be:

```
[ 32–64 byte opaque token ]
```

Format defined by the Capability RFC.

---

# **13. Algorithms**

## **13.1 Merkle-81 Construction**

1. Split file into 729-byte blocks
2. Hash each block → leaf nodes
3. Group children in sets of 81
4. Hash each group → parent node
5. Continue until 1 root remains

## **13.2 Directory Update**

To modify a directory:

1. Load existing version
2. Create new 81×81 tensor
3. Modify the desired slot(s)
4. Hash to produce new directory root
5. Emit new CanonRef

## **13.3 Capability Revocation**

1. Remove target capability from keyring
2. Seal revocation in admin capability log
3. All accesses MUST fail if capability is revoked

---

# **14. Operational Semantics**

A CanonFS implementation MUST guarantee:

* **no mutable state exists**
* all writes produce new snapshots
* read operations require valid capability
* references remain valid unless capability is revoked
* directories and files behave identically at the addressing layer

CanonFS MUST behave functionally and deterministically.

---

# **15. Security Considerations**

CanonFS naturally provides:

* **tamperproof content-addressing**
* **immutable historical states**
* **capability-based isolation**
* **verifiable lineage**
* **unforgeable identity semantics**

Implementations SHOULD consider:

* encrypted block stores
* secure deletion through capability revocation
* zero-knowledge capability delegation
* snapshot pruning for zero-access branches

---

# **16. Interoperability and Compatibility**

CanonFS MUST be interoperable across:

* T81VM virtual machines
* Axion kernel modules
* distributed CanonNode clusters

Binary filesystem interoperability SHOULD be implemented via:

* gateway mounts
* content-address translation layers
* canonical block adapters

---

# **17. Reference Implementation Notes**

A reference CanonFS implementation SHOULD:

* be written in a safe systems language (Rust recommended)
* use memory-mapped storage or append-only logs
* provide HTTP/IPC interfaces for capability issue/revocation
* integrate with Axion’s tensor memory layer

---

# **18. IANA Considerations**

This document registers:

* **CanonFS Media Type**: `application/x-canonfs`
* **Canonical Hash Namespace**: `canonfs-hash-81`

(future allocations reserved)

---

# **19. Acknowledgments**

This standard draws inspiration from:

* content-addressable storage
* functional programming
* ternary logic mathematics
* immutable persistent data structures
* capability-secured distributed systems

And from the foundational architecture of T81.

---
