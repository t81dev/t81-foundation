# RFC-0045: Deterministic Memory Model

- **RFC-ID:** RFC-0045
- **Title:** Deterministic Memory Model
- **Status:** draft
- **Type:** standards-track
- **Applies-To:** T81VM, TISC-visible state, DPE staging/commit, CanonFS-backed state artifacts, backend-sensitive execution surfaces
- **Created:** 2026-03-19
- **Updated:** 2026-03-19
- **Supersedes:** None
- **Discussion:** Builds on RFC-0002, RFC-0004, RFC-0006, RFC-DPE-0002, and RFC-DPE-0003

## Summary

This RFC defines the deterministic memory model for T81.

It specifies:

- the observable memory state and segment model
- read and write visibility rules
- canonical state-transition requirements
- aliasing and object-identity constraints
- the relationship between direct execution and epoch-committed execution

The purpose of this RFC is to make memory behavior an explicit deterministic contract instead of an implementation side effect.

## Motivation

T81 already relies on strong memory assumptions:

- segment-aware VM execution
- canonical tensor and value encoding
- deterministic GC expectations
- DPE snapshot and commit semantics
- backend-sensitive packed-trit execution

But those assumptions are spread across code and multiple RFCs rather than defined by one memory contract.

Without a dedicated memory model RFC:

- deterministic execution remains under-specified at the state-transition layer
- DPE and non-DPE execution can drift conceptually
- backend equivalence lacks a stable statement about visibility and aliasing
- future JIT and heterogeneous execution would inherit ambiguity

## Proposal

### 1. Scope of the Memory Model

This RFC governs the observable memory semantics of T81.

Observable memory includes:

- VM registers when they hold addressable handles or values
- segment-backed memory words
- memory tags
- canonical heap, tensor, and meta state
- DPE staging and committed state

This RFC does not govern:

- host pointer values
- allocator implementation details that remain outside the DCP surface
- real-time latency of memory operations

### 2. Canonical State Principle

At every deterministic observation boundary, T81 memory MUST have a canonical state representation.

A canonical memory state is one in which:

1. segment membership is unambiguous
2. each addressable word has a deterministic value
3. each addressable word has a deterministic tag where tags apply
4. object and handle relationships are reconstructible without host pointer identity

Host representation may vary internally, but canonical observed state may not.

### 3. Segment Semantics

The deterministic memory model recognizes the existing logical segment families:

- code
- stack
- heap
- tensor
- meta

Each addressable location MUST belong to exactly one logical segment at the observation boundary.

Segment membership MUST determine:

- legal access classes
- bounds behavior
- fault behavior
- canonical serialization rules where applicable

### 4. Read Semantics

A read is deterministic if and only if it returns:

1. the canonical last committed value for the addressed location, or
2. a deterministic fault if the read is invalid

A read MUST NOT depend on:

- host allocation order
- stale freed storage
- unspecified container iteration order
- race timing

Uninitialized-read behavior on a verified surface is forbidden. If a location may not be read, the implementation MUST trap or otherwise fail deterministically.

### 5. Write Semantics

A write is deterministic if and only if:

1. the target location is valid for mutation
2. the resulting value and tag are canonical
3. visibility of the write is defined by the active execution mode

There are two visibility modes:

#### Immediate Visibility

Used by normal interpreter-visible execution when no epoch buffering is active.

The write becomes the canonical state immediately after the governed operation completes.

#### Deferred Visibility

Used by DPE-style epoch execution.

The write enters a staged delta representation and is not visible to canonical state readers until commit.

The implementation may use internal buffering in either mode, but the observation boundary must preserve these semantics exactly.

### 6. Fault Semantics

Memory faults are part of the deterministic contract.

The memory model MUST define deterministic outcomes for:

- out-of-bounds reads
- out-of-bounds writes
- segment-violation reads
- segment-violation writes
- malformed handle dereference
- unmapped region access in DPE-governed contexts

Equivalent computations must fault identically, not merely “fail somehow.”

### 7. Object Identity and Handles

The deterministic memory model is handle-centric, not pointer-centric.

Object identity visible to the VM or serialized state MUST be based on governed logical identity, such as:

- stable handle indices
- canonical object content
- canonical object placement rules where placement is part of the surface

Host pointer addresses MUST NOT be part of the deterministic meaning of the state.

### 8. Aliasing Constraints

Aliasing MUST be explicit at the deterministic surface.

The memory model distinguishes:

- non-overlapping regions
- overlapping but non-exclusive regions
- exclusive regions in DPE-governed execution

Implementations MUST NOT rely on implicit host aliasing behavior to derive semantics.

If overlapping writes are allowed, their result must be resolved by a separately governed ordering rule.

### 9. DPE Interaction

For DPE-governed execution:

- each epoch reads from an immutable input snapshot
- writes are accumulated as deterministic delta records
- canonical visibility occurs only at commit

RFC-DPE-0003 defines canonical commit ordering. This RFC defines the underlying visibility rule:

> no staged write is part of canonical memory state until the epoch commits successfully

Aborted epochs MUST leave canonical memory unchanged.

### 10. Backend Interaction

Backend changes may alter how memory is processed internally, but they may not alter:

- canonical bytes
- canonical tags
- visibility timing at the governed boundary
- fault class

This applies to scalar, SWAR, SIMD, JIT, and future heterogeneous backends alike.

### 11. GC and Compaction

GC, relocation, or compaction are allowed only if the canonical state meaning is preserved.

Compaction MUST NOT change:

- visible value semantics
- logical handle identity
- canonical serialization
- deterministic fault outcomes

If physical relocation occurs, it must be invisible at the deterministic boundary.

### 12. Serialization Boundary

When memory-derived structures are serialized into governed artifacts, serialization MUST derive from canonical memory meaning rather than host layout accidents.

This includes:

- binary IO surfaces
- CanonFS-backed objects
- deterministic traces where memory state contributes to the hash

### 13. Out of Scope

This RFC does not define:

- the precise scheduler
- the CI proof model
- backend substitution rules

Those belong to companion RFCs.

## Acceptance Criteria

This RFC is ready for `accepted` when all of the following are true:

1. the segment model and canonical visibility rules are explicitly referenced from the Deterministic Execution Contract or governance docs
2. immediate vs deferred visibility is documented consistently with DPE epoch commit behavior
3. deterministic memory fault classes are mapped to executable tests or existing trap coverage
4. handle identity and compaction rules are stated consistently with current VM/GC behavior
5. backend-sensitive surfaces cite this RFC rather than relying on implicit memory assumptions

## Impact

### Backward Compatibility

This RFC should not require user-facing semantic change.

It makes existing expectations explicit and may force cleanup where code currently relies on incidental host-memory behavior.

### Performance

The RFC does not prohibit optimization, but it constrains optimization to preserve canonical visibility and fault semantics.

### Security

A formal memory model reduces UB exposure, stale-state ambiguity, and backend-sensitive divergence.

## Alternatives Considered

### Leave memory semantics distributed across VM, DPE, and GC RFCs

Rejected because shared assumptions become impossible to audit centrally.

### Define only segment layout and not visibility

Rejected because determinism failures often appear at visibility boundaries, not just address layout.

### Use host object identity as part of semantics

Rejected because it is fundamentally incompatible with cross-platform determinism.

## References

- `spec/rfcs/RFC-0002-deterministic-execution-contract.md`
- `spec/rfcs/RFC-0004-canonical-tensor-semantics.md`
- `spec/rfcs/RFC-0006-deterministic-gc.md`
- `spec/rfcs/RFC-DPE-0002-tisc-task-graph-primitives.md`
- `spec/rfcs/RFC-DPE-0003-epoch-execution-and-canonical-commit.md`
- `core/vm/memory_segments.cpp`
