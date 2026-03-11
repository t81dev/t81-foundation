# RFC 00B4: Axion Userland Service Contract

- **Author(s):** T81 Foundation
- **Status:** Draft
- **Created:** 2026-03-11
- **Supersedes:** None
- **Depends on:** RFC-00B3 (Axion Kernel Architecture), RFC-00B1 (Ternary MMU), RFC-00B2 (Device Drivers)

## Summary

This RFC formally defines the target Application Binary Interface (ABI) and service boundaries for the T81 Foundation's Ternary OS (Axion). It outlines the primary identities, deterministic lifecycle, memory pager contract, and diagnostic views that form the "Service Contract" for writing "ternary-native" services.

## Motivation

As the T81 Foundation's technical stack stabilizes, a primary focus is Developer Experience (DX). The adoption of the Ternary OS (Axion) depends directly on how seamlessly developers can write "ternary-native" services. Although actual syscall and capability semantics are currently deferred in the kernel execution plan, creating this Service Contract RFC now helps shape the eventual userland ABI and aligns community expectations around kernel boundaries.

## Proposal

### Technical Details

Entities within the Axion OS are identified by stable, deterministic handles. A ternary-native service must interact with these primary identities:

*   **`Tid` (Thread ID)**: The fundamental scheduling unit.
*   **`ProcessGroupId`**: A collection of threads that share fault policy and lifecycle state.
*   **`AddressSpaceId`**: kernel-owned memory objects bound to a process group.
*   **`SupervisorId`**: An auditing and management layer above process groups.
*   **`ServiceId`**: A registered service actor mapped to a process group.

#### The Service Lifecycle

Ternary-native services do not "just run"; they exist within a deterministic lifecycle managed by their Supervisor and the Kernel. A service communicates its state changes via an explicit Kernel Action contract (currently tracked internally as `KernelServiceActionKind`):

1.  **`RegisterService`**: Introduce a new service identity to the kernel, binding it to a `ProcessGroupId` and `SupervisorId`.
2.  **`SuspendService` / `ResumeService`**: Temporarily halt or resume processing for a service.
3.  **`MarkServiceUnhealthy` / `MarkServiceHealthy`**: Report degraded states to the supervisor without destroying the service.
4.  **`UnregisterService`**: Gracefully terminate and remove the service identity.

All lifecycle transitions strictly adhere to the Deterministic Execution Contract (RFC-0002) and Axion Safety Model (RFC-0003).

*Note: Requests from a faulted process group are deterministically rejected. A service must be in a healthy, active state to perform most operations.*

#### Faults and Memory (The Pager Boundary)

Axion separates explicit policy failures from pager-eligible misses, as defined in RFC-00B1 (Ternary MMU). While the internal kernel pager worker handles memory resolution automatically (e.g., dispatching handoffs and coalescing faults), services and supervisors must be aware of the fault contract:

*   **`Unmapped`**: A pager-eligible fault. The kernel automatically marks the address space as `pager_needed` and places it in the FIFO handoff queue.
*   **`PermissionDenied` / `InvalidTva`**: Explicit policy violations resulting in a delivered fault.
*   **`AcknowledgeSupervisorFaultGroup`**: Supervisors must explicitly acknowledge faulted groups, clearing the pipeline for recovery or teardown.

#### Device Arbitration

Device access is strictly arbitrated, aligning with the HAL (RFC-00B0) and Device Driver (RFC-00B2) models:

*   **`ClaimDevice`**: Request exclusive ownership of a device segment (identified by name/profile) and its IRQ line.
*   **`ReleaseDevice`**: Relinquish ownership, allowing other services or the kernel to reassign it.
*   *Rejections*: Requests are rejected if the device is already claimed (`DeviceConflict`), the process group lacks arbitration rights (`MissingDeviceArbitration`), or the service is not the owner (`DeviceNotOwned`).

#### Diagnostic Views

The ABI guarantees stable diagnostic windows. Services and diagnostic tools can request structured views of the system (currently modeled as `KernelServiceRequestKind`):

*   **Runtime Status**: Overall system memory, pager load, and IPC iterations.
*   **Process Group / Supervisor Status**: Ownership, mapped page counts, and fault tallies.
*   **Service Inventory**: A Supervisor's view of all registered, suspended, or unhealthy services.
*   **Audit Summary**: A deterministic log of fault deliveries, thread quarantines, and lifecycle transitions.

### Corner Cases

*   **Faulted Process Groups**: A process group in a `faulted` state has its operations deterministically rejected until the Supervisor acknowledges the fault.
*   **Worker Backlog**: If the kernel memory pager is backlogged, unmapped faults may queue instead of resolving immediately, leading to deterministic stalls logged in diagnostics.
*   **Device Conflicts**: When multiple services race to claim the same device, the first succeeds and others immediately receive `DeviceConflict` objections.

## Impact

### Backward Compatibility

This RFC establishes the first official Service Contract for userland. Since userland is being built from the ground up on the Ternary OS, this adds new API surface rather than breaking existing functionality.

### Performance

The deterministic state transitions and explicitly constrained lifecycle model are designed to avoid latency spikes and non-deterministic behavior inside the kernel, aligning with Axion's philosophy. 

### Security

The introduction of Process Groups, Supervisors, and explicit device arbitration enforces separation of concerns. Access rights tied to the kernel-side entities inherently restrict services from reading arbitrary memory or claiming unassigned devices.

## Alternatives Considered

*   **POSIX-style Syscalls**: Using POSIX-style APIs was rejected due to its non-deterministic, side-effect heavy nature and fundamental mismatch with Axion's ternary architecture.
*   **Implicit Service Running**: Allowing raw threads without a Supervisor or Process Group abstraction was rejected because it bypassed kernel audit logs and complicated strict fault quarantine constraints.

## References

*   [RFC-0001: Architecture Principles](RFC-0001-architecture-principles.md)
*   [RFC-0002: Deterministic Execution Contract](RFC-0002-deterministic-execution-contract.md)
*   [RFC-0003: Axion Safety Model](RFC-0003-axion-safety-model.md)
*   [RFC-00B0: HAL Spec](RFC-00B0-hal-spec.md)
*   [RFC-00B1: Ternary MMU](RFC-00B1-ternary-mmu.md)
*   [RFC-00B2: Device Drivers](RFC-00B2-device-drivers.md)
*   [RFC-00B3: Axion Kernel Architecture](RFC-00B3-axion-kernel-architecture.md)
*   [Kernel Execution Plan](../../docs/kernel_execution_plan.md)
