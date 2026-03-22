# RFC-0045: Deterministic Memory Model

- **RFC-ID:** RFC-0045
- **Title:** Deterministic Memory Model
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** T81VM, TISC-visible state, DPE staging/commit, CanonFS-backed state artifacts, backend-sensitive execution surfaces
- **Created:** 2026-03-19
- **Updated:** 2026-03-21
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
- `spec/rfcs/RFC-0042-deterministic-backend-equivalence-contract.md`
- `spec/rfcs/RFC-0043-deterministic-conformance-validation-framework.md`
- `core/vm/memory_segments.cpp`
- `include/t81/vm/state.hpp`
- `include/t81/vm/traps.hpp`
- `tests/cpp/vm_fault_test.cpp`
- `tests/cpp/test_vm_deterministic_fault.cpp`
- `tests/cpp/vm_fault_family_determinism_matrix_test.cpp`

## Implementation Record (2026-03-21)

All acceptance criteria are satisfied as of this date.

**AC1 — Segment model and canonical visibility referenced from RFC-0002 and governance docs:**
`spec/rfcs/RFC-0002-deterministic-execution-contract.md §5` ("Deterministic Memory Model")
explicitly enumerates the five canonical segment families (Code, Stack, Heap, Tensor, Meta)
and states the canonical read, canonical write, deterministic bounds, and handle-identity
requirements.  The implementation at `core/vm/memory_segments.cpp` defines `MemorySegmentKind`
with the same five entries and `segment_for_address()` which maps every VM address to
exactly one segment at the observation boundary.  `DETERMINISM_SURFACE_REGISTRY §3.1`
lists RFC-0045 as `accepted`, making the memory model a fully governed determinism surface.

**AC2 — Immediate vs deferred visibility consistent with DPE epoch commit behavior:**
RFC-0045 §5 defines Immediate Visibility (interpreter execution; write is canonical
immediately after the governed operation) and Deferred Visibility (DPE epoch execution;
write enters the staged delta and is invisible to canonical readers until commit).
`spec/rfcs/RFC-DPE-0003-epoch-execution-and-canonical-commit.md §2.2` specifies that
the staging area is a copy-on-write snapshot updated by delta application and the
original input_snapshot is unmodified until the epoch completes successfully — exactly
consistent with the deferred-visibility rule.  RFC-DPE-0003 §9 explicitly notes the
interaction with RFC-0045.

**AC3 — Deterministic memory fault classes mapped to executable tests:**
`include/t81/vm/traps.hpp` defines the fault taxonomy: `BoundsFault` (out-of-bounds
access), `StackFault` (stack overflow/underflow), `TypeFault`, `DecodeFault`,
`DivisionFault`, `SecurityFault`, `TierFault`, `ShapeFault`.  Executable test coverage
includes `tests/cpp/vm_fault_test.cpp`, `tests/cpp/test_vm_deterministic_fault.cpp`,
`tests/cpp/vm_fault_family_determinism_matrix_test.cpp`, and
`tests/cpp/axion_policy_segment_event_test.cpp`.  `spec/rfcs/RFC-0002-deterministic-execution-contract.md §11`
lists `tisc/bounds-fault-contract.t81` as a conformance fixture for the deterministic
BoundsFault contract.

**AC4 — Handle identity and compaction rules consistent with current VM/GC behavior:**
RFC-0045 §7 specifies handle-centric (not pointer-centric) object identity: logical
identity is based on stable handle indices and canonical content, not host pointer values.
RFC-0045 §11 prohibits compaction from changing visible value semantics, logical handle
identity, canonical serialization, or deterministic fault outcomes.  `core/vm/memory_segments.cpp`
implements `update_memory_stats()` and `contract_memory_pool()` using segment-based
logical accounting without exposing host pointer addresses at the DCP surface.
`spec/rfcs/RFC-0006-deterministic-gc.md` provides complementary GC constraints that
preserve handle identity across collection cycles.

**AC5 — Backend-sensitive surfaces cite RFC-0045 for memory and aliasing assumptions:**
`spec/rfcs/RFC-0047-deterministic-jit-and-lowering-rules.md` lists RFC-0045 in its
Discussion field and §5 ("Memory Visibility") explicitly cites "the memory visibility
model of RFC-0045" as a constraint on all JIT lowering transformations.  RFC-0047 §4
forbids any JIT transformation that changes canonical commit order or write visibility
rules.  `spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md`
lists `spec/rfcs/RFC-0045-deterministic-memory-model.md` as a normative reference and
uses RFC-0045 as the memory semantics authority for DCP classification.  RFC-0042
(backend equivalence) §5 (Observable Boundaries) extends the equivalent list to include
"canonical bytes" and "canonical tags" — both defined by RFC-0045.
