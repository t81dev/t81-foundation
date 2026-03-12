
RFC-00BX: Governed Event Interrupt Model for Axion

Status: Draft
Type: Standards Track
Applies-To: TISC ISA, Axion Kernel, HAL interrupt layer
Author: T81 Foundation
Created: 2026-03-12
Related: RFC-00B0 (HAL), RFC-00B1 (MMU), RFC-00B3 (Axion Kernel Architecture)

⸻

1. Abstract

This RFC proposes formalizing Axion’s current interrupt workaround into a governed event-based interrupt architecture rather than introducing a traditional hardware-style trap-return instruction.

Instead of modeling interrupts as implicit CPU control transfers that require a dedicated return opcode (e.g., IRET), Axion treats interrupts as kernel-governed events entering the deterministic kernel event pipeline. Interrupt handlers are dispatched through a kernel-managed table and execution resumes through the normal scheduler context restore path.

This approach aligns interrupt handling with Axion’s architectural principles:
	•	deterministic execution
	•	governance-enforced state transitions
	•	auditable kernel behavior
	•	scheduler-mediated continuation

The goal is to unify interrupts, faults, scheduler ticks, and kernel signals under a single event-driven execution model.

⸻

2. Motivation

Traditional interrupt architectures originate from early hardware CPU designs where devices needed a way to preempt execution immediately.

Typical behavior:
	1.	device triggers interrupt
	2.	CPU saves execution state
	3.	CPU jumps to vector handler
	4.	handler executes
	5.	IRET restores interrupted context

This design introduces several issues for Axion:
	•	asynchronous nondeterminism
	•	hidden CPU state transitions
	•	difficulty in reproducing execution ordering
	•	limited governance control over interrupt delivery
	•	complex interrupt nesting semantics

Axion’s architecture emphasizes deterministic and governed execution, which conflicts with uncontrolled asynchronous interrupt semantics.

The current implementation already routes interrupts through a shadow dispatch table, meaning interrupt handling is effectively managed by kernel runtime logic.

This RFC proposes making that model the official interrupt architecture.

⸻

3. Goals

The interrupt architecture must support the following properties:

Determinism

Interrupt delivery must not introduce nondeterministic execution order.

Governance Integration

Interrupt delivery must pass through Axion policy boundaries.

Replayability

Interrupt ordering should be recordable and replayable for debugging or verification.

Explicit Kernel State

Interrupt state must exist as kernel-managed data structures rather than hidden CPU frames.

Scheduler Continuation

Execution should resume through the normal scheduler context restore path rather than a special trap-return opcode.

⸻

4. Non-Goals

This proposal does not attempt to:
	•	emulate legacy x86 or ARM interrupt semantics
	•	provide binary compatibility with conventional OS kernels
	•	support arbitrary nested hardware interrupt stacks

Axion intentionally defines a new interrupt model aligned with deterministic system design.

⸻

5. Architecture Overview

Interrupts are modeled as kernel events entering the runtime event pipeline.

Conceptual flow:

device or HAL signal
        ↓
HAL interrupt event
        ↓
kernel event queue
        ↓
dispatch handler
        ↓
state transition
        ↓
scheduler selects next runnable context

No architectural TRAPRET or IRET instruction exists.

Instead, handler completion returns control to the scheduler.

⸻

6. Kernel Event Model

Interrupts are represented using a unified event structure.

Example conceptual model:

KernelEvent
 ├ DeviceInterrupt
 ├ TimerTick
 ├ PageFault
 ├ IPCDelivery
 ├ GovernanceSignal
 └ SupervisorAction

Events are processed through the deterministic kernel loop.

Example:

loop:
  event = next_event()
  validate_event(event)
  dispatch_handler(event)
  update_kernel_state()
  schedule_next_thread()


⸻

7. Interrupt Dispatch

Handlers are registered through a kernel-managed dispatch table.

Conceptual interface:

register_interrupt_handler(vector, handler)
dispatch_interrupt(vector)
fire_simulated_interrupt(vector)

Handlers execute as kernel routines within the controlled execution pipeline.

⸻

8. Handler Execution Semantics

Handlers run within the kernel scheduling framework.

Execution properties:
	•	handlers run in deterministic order
	•	handlers may enqueue additional kernel events
	•	handlers may modify kernel state
	•	handlers may wake blocked threads

Completion of handler execution triggers a scheduler decision rather than a trap return.

⸻

9. Scheduler Continuation Model

Traditional interrupt handling restores execution through a trap-return instruction.

Axion instead performs:

context_save(interrupted_thread)
dispatch_interrupt_handler()
update_kernel_state()
next = scheduler.next_runnable()
context_restore(next)

The scheduler becomes the universal continuation mechanism for:
	•	interrupt handlers
	•	system calls
	•	faults
	•	thread yields

⸻

10. Governance Integration

Interrupt events pass through Axion policy checks.

Possible enforcement points include:
	•	interrupt rate limiting
	•	device authorization
	•	interrupt provenance verification
	•	anomaly detection

Example policy rule:

if device.interrupt_rate > allowed_threshold:
    quarantine_device()

This integrates device activity with Axion’s governance model.

⸻

11. Deterministic Replay

Because interrupts are processed through the event queue, their ordering can be recorded.

Example log entry:

event_id
timestamp
source_device
vector
handler
resulting_state_transition

This allows replay of system execution for debugging or verification.

⸻

12. Comparison With Traditional Interrupt Architecture

Feature	Traditional Interrupts	Governed Event Interrupts
Control transfer	CPU trap vector	Kernel event dispatch
Return mechanism	IRET / trap-return	scheduler continuation
Determinism	nondeterministic	deterministic ordering
Interrupt state	implicit CPU stack	explicit kernel objects
Governance	minimal	policy integrated
Replayability	difficult	straightforward


⸻

13. ISA Impact

The TISC ISA does not require a trap-return instruction under this model.

Interrupts remain outside the core instruction semantics.

The ISA continues to focus on deterministic execution primitives.

⸻

14. Compatibility Considerations

This model requires that:
	•	the HAL convert hardware interrupts into kernel events
	•	the scheduler maintain consistent context restore behavior
	•	the kernel event queue remain deterministic

Device drivers must use the governed dispatch mechanism.

⸻

15. Risks

Potential concerns include:
	•	performance overhead from software dispatch
	•	complexity in high-frequency interrupt scenarios
	•	integration challenges with certain hardware interrupt controllers

Mitigations include:
	•	prioritized event queues
	•	interrupt coalescing
	•	hardware-level batching where available

⸻

16. Open Questions
	1.	Should interrupt events have priority classes?
	2.	How should nested interrupts be modeled?
	3.	Should interrupt ordering guarantees be strict or partially ordered?
	4.	Should interrupt events be visible to user-space auditing tools?

⸻

17. Conclusion

Axion’s architecture prioritizes deterministic, governed, and auditable execution.

A traditional trap-return interrupt model conflicts with these goals.

The governed event interrupt architecture provides a coherent alternative that:
	•	aligns with Axion’s scheduler model
	•	integrates interrupt handling with governance policy
	•	enables deterministic replay
	•	simplifies kernel continuation semantics

The existing shadow dispatch table implementation demonstrates the viability of this approach.

This RFC proposes formalizing that design as the canonical interrupt architecture for Axion.
