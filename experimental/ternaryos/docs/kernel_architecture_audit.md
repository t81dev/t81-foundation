# Axion Kernel Architecture Audit

Date: 2026-03-12
Scope: `experimental/ternaryos`
Audit basis: repository source inspection plus local `ctest --test-dir build -R ternaryos -V`

## Executive Summary

Axion is an advanced architectural prototype, not yet a fully operational
kernel. The current implementation proves a coherent hosted boot-to-kernel
path, a ternary MMU model, deterministic scheduling, bounded IPC, a
kernel-owned fault pipeline, a narrow supervisor/service control plane, and
VirtualBox-shaped device interaction seams. The strongest theme is explicit
runtime ownership. The largest architectural weakness is over-centralization:
policy, pager logic, service control, fault routing, and diagnostics are all
concentrated in one kernel runtime translation unit.

Overall maturity: `advanced architectural prototype`

Estimated completion toward a fully operational kernel: `35%`

## Method

This audit was based on:

- direct review of the kernel, HAL, MMU, scheduler, IPC, device, shell, and
  test sources under `experimental/ternaryos`
- review of local status documents in `experimental/ternaryos/docs`
- local verification via:

```sh
ctest --test-dir build -R ternaryos -V
```

Observed local verification result:

- `8/8` ternaryos test binaries passed

## System Shape

The boot chain is simple and explicit:

1. `hal_main()` validates `BootContext`
2. the ethics gate is evaluated
3. control transfers to `axion_kernel_main()`

Key implementation anchors:

- HAL entry: [hal_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/hal/hal_main.cpp#L30)
- Kernel bootstrap: [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L1502)
- Kernel entry: [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L3080)

Architecturally, the kernel is a centralized runtime kernel with helper
subsystems, not a strongly layered implementation. HAL, MMU, scheduler, IPC,
and device wrappers have reasonably clean seams. The kernel core does not.

## Kernel Structure

`KernelRuntimeState` in
[kernel_main.hpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.hpp#L116)
owns:

- allocator
- page table
- scheduler
- IPC bus
- device arbitration
- thread/process-group/supervisor/address-space/service state
- fault, interrupt, pager, and audit queues
- extensive counters and retained diagnostics

This is coherent for an early kernel because ownership is explicit. The main
cost is that the implementation boundary is weak. The current structure is
effectively one large runtime object plus a policy engine.

Assessment:

- completeness: `3/5`
- coherence: `3/5`
- readiness: `2/5`

## Bootstrap and Initialization

`axion_kernel_bootstrap()`:

- validates memory regions
- constructs the allocator from the HAL memory map
- creates kernel thread/process-group/supervisor/address-space state
- registers kernel IPC
- optionally installs VirtualBox-shaped device arbitration from `platform_id`

Reference:

- [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L1502)

What is proven:

- the kernel owns runtime state immediately after HAL handoff
- the initial kernel control-plane objects are internally consistent

What is not yet present:

- explicit reservation of kernel image/stack pages in allocator policy
- executable loading
- module loading
- real interrupt/trap vector installation
- driver initialization against real hardware/VM targets

Assessment:

- completeness: `3/5`
- coherence: `4/5`
- readiness: `2/5`

## Runtime Execution Flow

The runtime loop in
[kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L1709)
uses a fixed priority order:

1. pending faults
2. pending interrupts
3. pending pager handoffs
4. pager worker state/progress
5. scheduler tick

This is one of the clearer parts of the design. The event ordering is explicit
and deterministic. It also makes testing straightforward.

The limit is that this is still a software policy loop, not a real
interrupt/timer-driven kernel execution model.

Assessment:

- completeness: `3/5`
- coherence: `4/5`
- readiness: `2/5`

## Memory Management

The memory subsystem has three clear pieces:

- TVA model: [tva.hpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/mmu/tva.hpp)
- physical allocator: [ternary_page_alloc.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/mmu/ternary_page_alloc.cpp)
- radix page table: [page_table.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/mmu/page_table.cpp#L115)

### Strengths

- the ternary virtual address model is explicit and internally consistent
- physical page allocation is real
- translation faults are classified as `InvalidTva`, `Unmapped`, and
  `PermissionDenied`
- page-table diagnostics are available

### Gaps

- allocator is simple global first-fit state
- no dedicated kernel heap or object allocator
- no real per-process page-table ownership model in active runtime behavior
- no reclaim, copy-on-write, swapping, shared memory model, or TLB policy

### Pager Status

The pager logic in
[kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L1806)
is sophisticated as a deterministic fault-work queue. It includes:

- fault coalescing
- handoff queues
- ready-bypass behavior
- parked worker behavior
- terminal failure policy
- boot-critical auto-resolution

But this is still not a full VM subsystem. The pager is primarily a kernel
fault orchestration mechanism whose resolution condition is effectively that a
mapping exists.

Assessment:

- completeness: `3/5`
- coherence: `4/5`
- readiness: `2/5`

## Scheduling

The scheduler is compact and reasonably clean.

References:

- run queue: [run_queue.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/sched/run_queue.cpp)
- scheduler: [scheduler.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/sched/scheduler.cpp#L18)
- context switching: [context_switch.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/sched/context_switch.cpp)

What exists:

- deterministic round-robin queue
- context save/restore against TISC thread state
- thread sleep/wake transitions

What is missing:

- true timer-enforced preemption in operational practice
- priority scheduling
- SMP support
- wait queues and synchronization primitives
- robust running-thread termination semantics

Assessment:

- completeness: `2/5`
- coherence: `4/5`
- readiness: `2/5`

## IPC

IPC is implemented as bounded per-thread mailboxes carrying `CanonRef` plus
small scalar/tag metadata.

Reference:

- [canon_message.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/ipc/canon_message.cpp#L15)

This matches the project’s determinism and content-addressing direction well.
It is clear and deliberately narrow.

Main gaps:

- no blocking semantics
- no reply correlation
- no endpoint namespace
- no capability control
- no advanced flow control beyond queue depth rejection

Assessment:

- completeness: `2/5`
- coherence: `4/5`
- readiness: `1/5`

## Fault Handling and Recovery

This is one of the strongest subsystems in the kernel.

References:

- fault delivery/quarantine/group blocking:
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L1713)
- thread/process-group acknowledgement:
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L2971)
- supervisor acknowledgement:
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L3030)

Behavior proven in code:

- faults route into thread-local inboxes
- faulting threads are quarantined
- owning process groups become blocked/faulted
- supervisors receive pending-group visibility
- recovery is gated by acknowledgement
- audit events retain fault/recovery transitions

This is architecturally strong and coherent. The main limitation is scope. The
current machinery mostly governs MMU-access and pager-related fault classes,
not a full trap/exception universe.

Assessment:

- completeness: `4/5`
- coherence: `5/5`
- readiness: `3/5`

## Interrupt Model

The interrupt layer currently consists of:

- HAL-side registry and dispatch:
  [interrupt_table.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/hal/interrupt_table.cpp#L51)
- hosted synthetic boot use:
  [hosted_stub.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/hal/hosted_stub.cpp)
- kernel interrupt recording/delivery:
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L1583),
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L1774)

What exists:

- source classes
- deterministic pending queue
- per-source counters
- audit correlation

What is missing:

- real interrupt controller programming
- masks and priorities
- nesting policy
- EOI behavior
- timer-driven scheduling from actual IRQs

Assessment:

- completeness: `2/5`
- coherence: `3/5`
- readiness: `1/5`

## Service Infrastructure

The service layer is a kernel management/control plane, not yet a service
execution substrate.

References:

- request surface:
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L2161)
- action surface:
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L2578)

What exists:

- runtime/process-group/supervisor/service/fault/audit/device queries
- service registration/unregistration
- suspend/resume
- health transitions
- device claim/release through the same control path

What is missing:

- service execution model
- kernel IPC-backed service routing
- capability/syscall ABI
- boot-time activation policy
- persistence beyond current runtime metadata

Assessment:

- completeness: `3/5`
- coherence: `4/5`
- readiness: `2/5`

## Device Interaction Layers

The device layer is meaningful as a target-shaping abstraction, but still
mostly hosted.

References:

- block device abstraction:
  [block_device.hpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/block_device.hpp)
- hosted block device:
  [hosted_block_dev.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/hosted_block_dev.cpp)
- AHCI wrapper:
  [virtualbox_ahci_dev.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/virtualbox_ahci_dev.cpp)
- E1000 wrapper:
  [virtualbox_e1000_dev.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/virtualbox_e1000_dev.cpp)
- VMSVGA wrapper:
  [virtualbox_vmsvga_dev.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/virtualbox_vmsvga_dev.cpp)
- content-addressed store:
  [canon_store.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/canon_store.cpp)

Strengths:

- first-target VBox profile is explicit in code
- storage/network/display seams are fixed and testable
- CanonStore is materially more complete than the wrappers around it

Gaps:

- wrappers are not hardware drivers
- no DMA, descriptors, MMIO programming, or IRQ handling against real devices
- no discovery/probe framework

Assessment:

- completeness: `2/5`
- coherence: `4/5`
- readiness: `1/5`

## Persistence and Storage

CanonStore is one of the strongest concrete components in the stack.

Reference:

- [canon_store.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/canon_store.cpp)

It provides:

- deduplicated content-addressed block storage
- flush/rebuild behavior
- overflow metadata handling
- torn-header fallback scanning
- corruption detection on read

Within the hosted device model, this is substantive and well-covered by tests.

Assessment:

- completeness: `4/5`
- coherence: `4/5`
- readiness: `3/5`

## Shell and User-Facing Layer

The shell is a hosted durable-session demo, not yet userland.

Reference:

- [shell_session.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/shell/shell_session.cpp)

What it proves:

- durable transcript/object persistence over CanonStore
- guest bootstrap integration
- framebuffer-based text rendering
- deterministic command history and object operations

What it does not prove:

- user processes
- syscalls
- executable runtime
- service IPC on top of a user/kernel ABI

Assessment:

- completeness: `3/5`
- coherence: `4/5`
- readiness: `1/5`

## Test Evidence

Local verification executed:

```sh
ctest --test-dir build -R ternaryos -V
```

Observed result:

- `8/8` test binaries passed

Covered suites:

- HAL boot
- page allocator
- context switching
- MMU
- scheduler
- IPC
- device drivers
- shell session

This gives strong evidence for the implemented hosted and deterministic
behavior. It does not yet provide equivalent evidence for real external target
execution.

## Audit Matrix

| Subsystem | Completeness | Coherence | Readiness | Notes |
| --- | ---: | ---: | ---: | --- |
| HAL / boot | 3/5 | 4/5 | 2/5 | Validated hosted handoff, explicit VBox profile, mostly stubbed beyond prototype |
| Kernel core structure | 3/5 | 3/5 | 2/5 | Clear ownership, monolithic implementation |
| Runtime execution flow | 3/5 | 4/5 | 2/5 | Deterministic loop, not real interrupt/timer-driven dispatch |
| Memory management | 3/5 | 4/5 | 2/5 | Real core pieces, incomplete VM |
| Scheduling | 2/5 | 4/5 | 2/5 | Clean substrate, minimal features |
| IPC | 2/5 | 4/5 | 1/5 | Narrow mailbox primitive |
| Fault / recovery | 4/5 | 5/5 | 3/5 | Strongest subsystem |
| Interrupts | 2/5 | 3/5 | 1/5 | Simulated controller model |
| Service infrastructure | 3/5 | 4/5 | 2/5 | Control plane present, runtime absent |
| Device interaction | 2/5 | 4/5 | 1/5 | Device-shaped hosted wrappers |
| Persistence / storage | 4/5 | 4/5 | 3/5 | CanonStore is substantial |
| Shell / user-facing stack | 3/5 | 4/5 | 1/5 | Effective demo, not userland |

## Risk Register

### R1. Kernel monolith

- evidence:
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L1502)
- impact: high
- likelihood: high
- issue: too much policy and too many retained state concerns live in one file
- consequence: scaling and maintenance risk
- mitigation: extract runtime, pager, interrupt, fault, service, and diagnostics managers

### R2. Diagnostics ahead of substrate

- evidence:
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L2161)
- impact: medium-high
- likelihood: high
- issue: there is more detailed status plumbing than low-level execution capability
- consequence: maturity can be misread
- mitigation: prioritize real execution primitives and target bring-up over additional summaries

### R3. Synthetic pager model

- evidence:
  [kernel_main.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.cpp#L1806)
- impact: high
- likelihood: high
- issue: pager logic is advanced as queue policy but incomplete as VM implementation
- consequence: memory subsystem maturity is overstated if read as full paging support
- mitigation: introduce backing objects, reclaim, and actual demand-paging semantics

### R4. Simulated interrupts

- evidence:
  [interrupt_table.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/hal/interrupt_table.cpp#L51)
- impact: high
- likelihood: high
- issue: no real interrupt-controller behavior exists yet
- consequence: preemption and device IRQ handling remain unproven on target
- mitigation: implement target interrupt-controller path before widening policy

### R5. Hosted wrappers mistaken for drivers

- evidence:
  [virtualbox_ahci_dev.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/virtualbox_ahci_dev.cpp),
  [virtualbox_e1000_dev.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/virtualbox_e1000_dev.cpp),
  [virtualbox_vmsvga_dev.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/dev/virtualbox_vmsvga_dev.cpp)
- impact: high
- likelihood: high
- issue: adapters preserve device boundaries but not hardware behavior
- consequence: large promotion gap remains between passing tests and real guest execution
- mitigation: fully implement one hardware lane end-to-end

### R6. No syscall/capability boundary

- impact: high
- likelihood: certain
- issue: service actions are not a syscall ABI
- consequence: userland evolution path is blocked
- mitigation: define the minimum kernel ABI and capability model

## Maturity Assessment

Overall maturity: `advanced architectural prototype`

Category estimates:

- boot and HAL scaffolding: `60%`
- kernel runtime ownership: `55%`
- memory management: `45%`
- scheduling: `40%`
- IPC: `30%`
- fault and recovery: `65%`
- interrupts: `25%`
- service infrastructure: `35%`
- device layer: `30%`
- userland readiness: `15%`

Overall completion estimate toward a fully operational kernel: `35%`

## Roadmap to a Fully Operational Kernel

1. Refactor `kernel_runtime.cpp` into subsystem managers without widening behavior.
2. Promote address spaces into true execution units with separate page-table
   ownership.
3. Implement real interrupt/trap handling for the target platform.
4. Define the minimum syscall/capability ABI.
5. Add a real task/process lifecycle: load, exec, exit, teardown.
6. Convert the pager from fault-workflow policy into a real VM subsystem with
   backing objects and reclaim.
7. Fully implement one real target driver rather than widening hosted wrappers.
8. Add kernel synchronization and wait primitives.
9. Move the shell and higher-level services onto the real process/service/ABI
   boundary.
10. Validate external target execution and use that evidence to drive the next
    hardening cycle.

## Final Judgment

Axion is no longer a conceptual kernel sketch. It has a real vertical slice,
strong deterministic observability, meaningful subsystem implementations, and
good test evidence for the currently implemented scope. The remaining distance
to a fully operational kernel is still substantial, and most of that distance
is foundational rather than incremental. The next productive step is not more
surface-area growth inside the monolith. It is disciplined subsystem
extraction, real ABI definition, and real target bring-up.
