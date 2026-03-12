# RFC-00B5: Governed Event Interrupt Model

**Status:** accepted
**Type:** standards-track
**Applies-To:** Axion HAL interrupt translation, kernel event delivery, TISC ISA interrupt and trap model
**Created:** 2026-03-12
**Updated:** 2026-03-12
**Author:** @t81dev
**Depends on:** RFC-00B0 (HAL), RFC-00B1 (MMU), RFC-00B3 (Axion Kernel Architecture)
**Blocks:** Later interrupt and timer integration beyond the current kernel bootstrap path

---

## 1. Summary

This RFC formalizes Axion interrupt handling as a governed kernel event model
instead of a traditional hardware-style trap-and-return architecture.

Interrupts are not treated as implicit CPU control transfers that require a
dedicated architectural return opcode. The HAL translates hardware-visible
interrupts into explicit kernel events, the kernel dispatches them through its
deterministic event pipeline, and execution resumes through the scheduler's
normal context restore path.

This is the architectural consequence of two facts already true in the current
stack:

- the frozen TISC ISA has no interrupt or trap-return opcode
- the current HAL path already relies on a shadow dispatch table rather than a
  native ISA interrupt return sequence

This RFC makes that governed event model the intended forward path instead of
treating it as a temporary workaround.

## 2. Motivation

Traditional interrupt designs assume hidden CPU state transitions:

1. a device asserts an interrupt
2. the CPU saves machine state implicitly
3. control jumps to a vector handler
4. the handler executes on an implicit interrupt frame
5. a special return instruction restores the interrupted context

That model conflicts with Axion's current design priorities:

- deterministic execution ordering
- explicit kernel-owned state transitions
- policy-aware event delivery
- replayable and auditable control flow
- scheduler-mediated continuation

Axion already leans away from conventional interrupt semantics because TISC does
not expose a trap-return instruction and the HAL already mediates interrupt
delivery through a software dispatch layer. The missing step is to describe that
approach as architecture, not workaround.

## 3. Proposal

### 3.1 Goals

- Define interrupts as explicit kernel-governed events.
- Preserve deterministic kernel loop semantics under interrupt delivery.
- Keep continuation on the scheduler restore path rather than a special return
  opcode.
- Make interrupt delivery auditable, policy-visible, and replayable.
- Align HAL interrupt translation with the kernel boundary defined in
  RFC-00B3.

### 3.2 Non-Goals

- binary compatibility with x86, ARM, or Unix interrupt semantics
- legacy nested interrupt-stack emulation
- public userspace interrupt ABI design
- priority-class policy or full interrupt-controller architecture
- widening into syscall or capability design

### 3.3 Interrupts as Kernel Events

Interrupts enter Axion as explicit runtime events, not hidden machine-control
transfers.

Conceptually:

```text
device or platform signal
        ->
HAL interrupt translation
        ->
kernel event queue
        ->
validated handler dispatch
        ->
kernel state transition
        ->
scheduler selects next runnable context
```

This unifies interrupts with the other event sources already consistent with the
current kernel direction:

- device interrupts
- timer ticks
- faults
- IPC delivery
- supervisor or governance actions

### 3.4 HAL Responsibilities

The HAL is responsible for translating platform-specific interrupt sources into
canonical Axion events.

For the current architecture, that means:

- no mutation of the frozen TISC ISA to add interrupt return semantics
- no direct exposure of platform interrupt controller behavior to TISC code
- normalization through the existing shadow dispatch table or equivalent kernel
  entry shim
- handoff of explicit interrupt provenance into kernel-owned state

This refines RFC-00B0 rather than replacing it. RFC-00B0 defines the initial
HAL workaround; this RFC defines that workaround as the forward interrupt model.

### 3.5 Kernel Responsibilities

The kernel is responsible for:

- receiving translated interrupt events
- validating and dispatching those events deterministically
- recording resulting state transitions as explicit kernel-owned data
- deciding continuation through the scheduler path
- exposing interrupt handling to audit, governance, and replay facilities as
  those layers mature

Interrupt completion is therefore a scheduler decision, not an architectural
trap return.

### 3.6 Continuation Model

Under this model, continuation is conceptually:

```text
save current thread context
dispatch interrupt handler through kernel event path
update kernel state
select next runnable context
restore that context
```

The interrupted thread may resume immediately, later, or not at all, depending
on the resulting kernel state and policy decisions. That is intentional. The
interrupt path does not imply a mandatory "return to exactly where we were"
machine primitive.

### 3.7 Governance and Replay

Because interrupts enter the kernel as explicit events, they can participate in
the same governance and observability model as other kernel events.

This RFC does not require a fully implemented interrupt audit format today, but
it does require the architecture to remain compatible with:

- interrupt provenance tracking
- device authorization checks
- rate limiting or quarantine decisions
- deterministic replay of interrupt ordering

### 3.8 ISA Impact

The TISC ISA does not need a trap-return instruction for the current Axion
direction.

This RFC therefore establishes a negative requirement:

- Axion does not require a new architectural `TRAPRET`, `IRET`, or equivalent
  instruction in this phase

If later ISA evolution proposes richer interrupt primitives, that work must be
argued against the deterministic event model defined here rather than assumed by
default.

## 4. Determinism / Safety Considerations

The governed event model is preferred because it keeps interrupt delivery inside
explicit kernel state and policy boundaries.

Determinism benefits:

- interrupt ordering can be recorded as event ordering
- handler execution remains visible to the deterministic kernel loop
- continuation remains a normal scheduler outcome

Safety and governance benefits:

- device interrupt provenance can be checked before downstream effects
- abnormal interrupt rates can be surfaced to quarantine or policy layers
- hidden CPU interrupt frames do not become an ungoverned side channel

The main tradeoff is software complexity and possible dispatch overhead. That is
acceptable in the current Axion phase because correctness, auditability, and
architectural coherence matter more than legacy interrupt performance parity.

## 5. Compatibility

This RFC is compatible with the current implementation direction:

- RFC-00B0 already documents a shadow interrupt dispatch path
- RFC-00B3 already centers the kernel on a deterministic event loop and
  scheduler-mediated continuation
- no shipped TISC contract depends on a trap-return opcode

This RFC is intentionally incompatible with expectations of conventional
hardware-style interrupt semantics. That is not a regression. Axion is defining
its own interrupt model rather than promising x86 or ARM kernel behavior.

## 6. Implementation Plan

1. Promote the interrupt model from experimental note to official RFC.
2. Update TernOS status docs so interrupt semantics are tracked against this RFC
   rather than as a generic open question.
3. Converge the first interrupt-summary surfaces on that governed event model:
   intake, delivery, queue state, source accounting, and audit provenance.
4. When interrupt work continues beyond that convergence slice, require new
   kernel or HAL behavior to follow the governed event model instead of
   introducing ad hoc trap-return behavior.

## 7. Open Questions

- Should interrupt events eventually gain explicit priority classes?
- How much interrupt nesting, if any, should be representable in kernel state?
- Which interrupt records must become part of the stable audit surface?
- Should later user-facing tooling observe interrupt events directly, or only
  through summarized supervisor and audit views?

## 8. Acceptance Criteria

This RFC can move from `accepted` to `integrated` when:

- HAL and kernel interrupt handling are documented normatively against this
  event model
- any remaining interrupt-related open questions are narrowed to policy,
  prioritization, and handler behavior, not basic continuation semantics
