# RFC-0055: Native Ternary Hardware Target and Interop Contract

**Status:** draft
**Type:** standards-track
**Applies-To:** native ternary hardware targets, hardware ISA interop, HAL promotion, backend equivalence, trace/policy preservation
**Created:** 2026-03-19
**Updated:** 2026-03-19
**Supersedes:** None
**Superseded-By:** None
**Discussion:** Builds on RFC-0002, RFC-0042, RFC-0045, RFC-0046, RFC-0047, RFC-0048, RFC-0051, RFC-00B0, and RFC-00B1

---

## Summary

This RFC defines how T81 may target future native ternary hardware without
creating a second semantic universe.

It establishes:

- the distinction between a general-purpose native ternary hardware target and
  a narrow accelerator target
- the rules for mapping TISC and Axion semantics onto a non-TISC hardware ISA
- the interoperability obligations for boot, memory, interrupts, storage, and
  trace/audit preservation
- the proof requirements before any external ternary hardware target may be
  described as a governed T81 execution target

The purpose of this RFC is to ensure that future ternary hardware support is a
governed interoperability layer, not an architecture fork.

## Motivation

T81 is explicitly designed around balanced ternary execution, but the current
repo runs on binary host hardware and virtualized guest environments.

The project now has:

- a hosted and guest-oriented HAL path via RFC-00B0
- draft backend equivalence and lowering contracts via RFC-0042 and RFC-0047
- a draft heterogeneous acceleration contract via RFC-0051

What is still missing is the specific contract for **native ternary hardware**
itself.

That gap matters because existing and emerging ternary hardware candidates fall
into two different categories:

- general-purpose ternary processors with their own ISA and boot model
- narrow ternary accelerators aimed at tensor or inference kernels only

Without a dedicated RFC:

- any future hardware port risks inventing a parallel semantic layer
- it remains unclear whether TISC is the hardware ISA, the virtual ISA, or only
  one lowering target
- trace, policy, and CanonFS guarantees could be weakened in the name of
  hardware practicality

This RFC closes that gap before hardware integration work begins.

## Proposal

### 1. Hardware Target Classes

T81 recognizes two distinct hardware target classes:

1. **General-purpose native ternary target**
   A hardware platform capable of hosting the T81 runtime, memory model,
   interrupt model, and governed OS-facing surfaces.

2. **Narrow ternary accelerator target**
   A hardware path that executes only selected kernels or tensor operations
   beneath an existing CPU- or VM-governed runtime.

Narrow accelerator targets are governed by RFC-0051.

This RFC governs only the first class unless a section explicitly states
otherwise.

### 2. TISC Remains the Semantic Authority

For any native ternary hardware target, the semantic authority remains:

- TISC instruction semantics
- T81VM observable behavior
- Axion policy and audit meaning
- Canonical data and trace contracts

This means:

- a hardware ISA does not replace TISC semantics by existing
- a non-TISC hardware ISA must be mediated by a verified lowering or
  compatibility layer
- hardware-specific instructions may only be used when they are proven
  observationally equivalent under RFC-0042

T81 may support:

- direct TISC-on-hardware execution, or
- verified lowering from TISC into a hardware-native ISA

It may not support:

- a hardware target that changes user-visible semantics merely because the
  hardware has a different native instruction vocabulary

### 3. Hardware Interop Layer

Any general-purpose native ternary hardware target MUST define an explicit
hardware interop layer above the device ISA and below the governed T81
execution model.

That layer MUST define:

- boot handoff contract
- memory discovery and mapping contract
- interrupt and trap translation
- device I/O exposure rules
- fault classification and fallback rules
- canonical serialization boundaries for any host/hardware exchange

RFC-00B0 HAL is the current architectural starting point for this layer, but it
is not yet sufficient by itself for real hardware promotion.

### 4. Supported Hardware Integration Modes

The following integration modes are allowed:

1. **Direct semantic target**
   Hardware executes a TISC-faithful implementation directly.

2. **Verified lowering target**
   Hardware executes a lower-level ISA while a verified lowering preserves TISC
   semantics.

3. **Hosted compatibility mode**
   Hardware-specific behavior is surfaced only through a guest/host HAL path
   while the main runtime remains governed by the hosted or virtualized T81
   environment.

Mode 1 is the cleanest long-term target.

Mode 2 is acceptable only if the lowering layer satisfies RFC-0042 and
RFC-0047.

Mode 3 is transitional and must not be described as native T81 hardware
execution.

### 5. Boot and Privilege Rules

Any native ternary hardware target MUST define:

- boot entry format
- privilege levels or their absence
- reset and trap entry model
- kernel/user boundary, if any
- ethics-first boot and Axion-first enforcement points

Ethics-first boot and Axion policy meaning must survive the hardware target.

It is forbidden to claim hardware support if the target bypasses or weakens:

- ethics boot gating
- policy trap classification
- governed interrupt meaning

### 6. Memory and Addressing Contract

Native ternary hardware support MUST define:

- physical and virtual address model
- ternary page or allocation granularity
- canonical endianness and packing rules
- alignment and padding semantics
- DMA and device-visible memory rules

RFC-00B1 remains authoritative for Ternary MMU direction. A hardware target may
specialize implementation strategy, but may not redefine the public memory
semantics without an RFC update.

### 7. Interrupt and Device Semantics

For a native ternary hardware target, interrupt and device handling must remain
governed rather than hardware-opaque.

Requirements:

- interrupt classes map to canonical T81/Axion-visible meaning
- device completion timing may not change deterministic observation boundaries
- unhandled interrupts must remain auditable
- device ownership, capability, and policy semantics must survive hardware
  integration

Hardware convenience mechanisms may not silently bypass Axion mediation.

### 8. Trace, Audit, and CanonFS Preservation

A native ternary hardware target is not eligible for governed promotion unless
it preserves:

- canonical trace meaning
- Axion audit meaning
- CanonFS-visible object identity semantics
- deterministic error/fault classification at the observation boundary

Hardware performance counters, internal pipelines, or implementation details may
be recorded as diagnostics only if they remain outside the deterministic core
surface unless separately governed.

### 9. External ISA Compatibility Targets

If T81 targets an external ternary ISA rather than a TISC-faithful machine,
that target MUST define an ISA compatibility profile.

The compatibility profile MUST state:

- target ISA name and revision
- lowering boundary from TISC into the target ISA
- unsupported TISC features, if any
- exact fault/fallback behavior for unsupported features
- conformance corpus proving semantic equivalence on the supported subset

Unsupported features may result in deterministic rejection.

Silent subset execution is forbidden.

### 10. Promotion Rules

Any native ternary hardware target is **experimental by default**.

It may be promoted to governed non-DCP only if:

- the hardware interop layer is documented
- boot, memory, interrupt, and fault contracts are executable
- semantic equivalence to the scalar/interpreter oracle is proven on a bounded
  conformance corpus
- Axion and trace semantics are preserved
- public docs clearly distinguish supported vs unsupported hardware behavior

It may become DCP-eligible only after:

- RFC-0042 backend equivalence obligations are met
- RFC-0043 conformance obligations are met across the supported hardware matrix
- RFC-0045 and RFC-0046 memory and ordering obligations are met
- RFC-0048 surface boundaries are updated

### 11. Relation to Existing RFCs

- RFC-00B0 defines the current HAL starting point for boot and hardware
  translation.
- RFC-0051 governs narrow accelerator backends and does not by itself authorize
  general-purpose native hardware support.
- RFC-0042 and RFC-0047 define the equivalence and lowering constraints that any
  hardware target must inherit.

## Determinism / Safety Considerations

Determinism considerations:

- a ternary hardware target can still violate determinism if it changes fault
  order, memory visibility, or trace meaning
- hardware-native execution is not evidence of semantic correctness by itself
- a proprietary or externally defined ISA increases the need for an explicit
  compatibility layer rather than reducing it

Safety considerations:

- policy, interrupt, and trap mediation must remain fail-closed
- hardware support must not create a privileged bypass around Axion
- unsupported hardware capabilities must be rejected explicitly rather than
  partially enabled

## Compatibility

This RFC is additive.

It does not require:

- immediate support for any real ternary hardware target
- promotion of the current experimental HAL to stable status
- adoption of a hardware-specific ISA

It does require that future hardware efforts use one governed contract instead
of ad hoc integration logic.

## Implementation Plan

1. Keep RFC-00B0 HAL work separate from claims about native ternary hardware
   support.
2. Define a hardware target profile template for any future candidate platform.
   - [x] Template: [RFC-0055-hardware-target-profile-template.md](RFC-0055-hardware-target-profile-template.md)
   - [x] Schema: [RFC-0055-hardware-target-profile-schema.json](RFC-0055-hardware-target-profile-schema.json)
3. Require conformance matrices for any first hardware proof-of-concept.
4. Promote only after the hardware interop layer, fault model, and trace/policy
   preservation are executable and documented.

## Open Questions

- Should T81 ever target an external ternary ISA directly, or only target
  TISC-faithful hardware?
- What is the minimum supported subset for a first external ternary hardware
  compatibility profile?
- Should hardware-profile qualification be per device family, per board, or per
  ISA revision?
- Is a VirtualBox or QEMU guest still required as the proving ground before any
  real-hardware promotion?

## Acceptance Criteria

- Native ternary hardware support is explicitly separated from accelerator-only
  support.
- TISC semantic authority and hardware interop obligations are normatively
  defined.
- Boot, memory, interrupt, fault, trace, and policy preservation rules are
  specified for native hardware targets.
- External hardware ISA compatibility is governed by an explicit profile and
  equivalence requirements.
- No native ternary hardware target may be promoted without conformance,
  boundary, and audit updates.
