# Axion Kernel Architecture Audit

Date: 2026-03-13
Scope: `experimental/ternaryos`
Audit basis: repository source inspection plus local `ctest --test-dir build -R ternaryos --output-on-failure`

## Executive Summary

Axion is an advanced architectural prototype, not yet a fully operational
kernel. The current implementation proves a coherent hosted boot-to-kernel
path, a ternary MMU model, deterministic scheduling, bounded IPC, a
kernel-owned fault pipeline, a narrower but now real typed/wire/C ABI layer,
a supervisor/service control plane, and VirtualBox-shaped device interaction
seams. The strongest theme is still explicit runtime ownership, but the
largest structural weakness from the previous audit has been reduced: the old
kernel monolith has now been decomposed into subsystem-oriented runtime units.
The main remaining weakness is that execution remains hosted and synthetic in
the places that matter most: VM behavior, interrupts, and real device I/O.

Overall maturity: `advanced architectural prototype`

Estimated completion toward a fully operational kernel: `47%`

## Method

This audit was based on:

- direct review of the kernel, HAL, MMU, scheduler, IPC, device, shell, and
  test sources under `experimental/ternaryos`
- review of local status documents in `experimental/ternaryos/docs`
- local verification via:

```sh
ctest --test-dir build -R ternaryos --output-on-failure
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
- Kernel bootstrap/runtime coordinator:
  [kernel_runtime.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_runtime.cpp)
- Kernel lifecycle/bootstrap:
  [kernel_lifecycle.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_lifecycle.cpp)
- Kernel entry surface:
  [kernel_main.hpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_main.hpp)

Architecturally, the kernel is still a centralized runtime kernel with helper
subsystems, not a strongly layered implementation. But the implementation is
no longer concentrated in one translation unit. HAL, MMU, scheduler, IPC,
faults, interrupts, pager policy, service control, views, and lifecycle now
have distinct implementation seams.

## Kernel Structure

`KernelRuntimeState` in
[kernel_runtime_state.hpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_runtime_state.hpp)
owns:

- allocator
- page table
- scheduler
- IPC bus
- device arbitration
- thread/process-group/supervisor/address-space/service state
- page-backed hosted physical storage for mapped pages
- fault, interrupt, pager, and audit queues
- extensive counters and retained diagnostics

This is coherent for an early kernel because ownership is explicit. The main
cost is that the runtime-state contract is still broad. The implementation
boundary is materially better than it was in the previous audit: the system is
now a large runtime object plus multiple policy/control units rather than one
dominant policy file.

Assessment:

- completeness: `3/5`
- coherence: `4/5`
- readiness: `3/5`

## Bootstrap and Initialization

`axion_kernel_bootstrap()`:

- validates memory regions
- constructs the allocator from the HAL memory map
- creates kernel thread/process-group/supervisor/address-space state
- registers kernel IPC
- optionally installs VirtualBox-shaped device arbitration from `platform_id`

References:

- [kernel_lifecycle.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_lifecycle.cpp)
- [kernel_runtime.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_runtime.cpp)

What is proven:

- the kernel owns runtime state immediately after HAL handoff
- the initial kernel control-plane objects are internally consistent
- services can now retain executable entry descriptors and spawn from that
  retained service-owned state through the ABI path

What is not yet present:

- explicit reservation of kernel image/stack pages in allocator policy
- executable loading
- module loading
- real interrupt/trap vector installation
- driver initialization against real hardware/VM targets

Assessment:

- completeness: `4/5`
- coherence: `4/5`
- readiness: `3/5`

## Runtime Execution Flow

The runtime loop in
[kernel_runtime.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_runtime.cpp)
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

- completeness: `4/5`
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
[kernel_pager.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_pager.cpp)
is sophisticated as a deterministic fault-work queue. It includes:

- fault coalescing
- handoff queues
- ready-bypass behavior
- parked worker behavior
- terminal failure policy
- boot-critical auto-resolution

But this is still not a full VM subsystem. The pager is primarily a kernel
fault orchestration mechanism whose resolution condition is effectively that a
mapping exists. The current audit should, however, give credit for one real
step beyond the previous state: mapped pages now have a hosted byte store
behind them, which is enough to support a fixed-block mapped TVA syscall
transport.

Assessment:

- completeness: `3/5`
- coherence: `4/5`
- readiness: `3/5`

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
- no advanced flow control beyond queue depth rejection

The previous audit’s “no capability control” finding is no longer accurate.
IPC and other control paths now sit behind an explicit capability model in the
typed ABI.

Assessment:

- completeness: `2/5`
- coherence: `4/5`
- readiness: `2/5`

## Fault Handling and Recovery

This is one of the strongest subsystems in the kernel.

References:

- fault delivery/quarantine/group blocking:
  [kernel_faults.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_faults.cpp)
- thread/process-group acknowledgement:
  [kernel_faults.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_faults.cpp)
- supervisor acknowledgement:
  [kernel_actions.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_actions.cpp)

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
  [kernel_interrupts.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_interrupts.cpp)

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

The service layer is still a kernel management/control plane, not yet a
service execution substrate. But it is now reachable through a coherent typed
ABI, fixed-size wire blocks, byte bridges, a mapped-TVA bridge, and hosted C
entrypoints rather than only internal helper calls.

References:

- request surface:
  [kernel_queries.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_queries.cpp)
- action surface:
  [kernel_actions.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_actions.cpp)
- ABI dispatch:
  [kernel_abi.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_abi.cpp)

What exists:

- runtime/process-group/supervisor/service/fault/audit/device queries
- service registration/unregistration
- suspend/resume
- health transitions
- device claim/release through the same control path

What is missing:

- service execution model
- kernel IPC-backed service routing
- boot-time activation policy
- persistence beyond current runtime metadata

What is now present beyond the previous audit:

- typed capability-enforced kernel ABI
- supervisor-scoped capability inventory and delegation summaries
- service lifecycle control through typed, wire, and C ABI paths
- service-owned entry descriptors plus `SpawnThreadForService`

Assessment:

- completeness: `4/5`
- coherence: `4/5`
- readiness: `3/5`

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

## ABI and Execution Boundary

The kernel ABI has moved materially since the previous audit. It now includes:

- typed dispatch: `axion_kernel_call(...)`
- fixed-size wire transport: `axion_kernel_call_wire(...)`
- raw byte bridge: `axion_kernel_call_wire_bytes(...)`
- mapped TVA bridge: `axion_kernel_call_wire_tva(...)`
- exported hosted C lifecycle and call entrypoints:
  `ternaryos_kernel_bootstrap_c(...)`,
  `ternaryos_kernel_destroy_c(...)`,
  `ternaryos_kernel_call_c(...)`,
  `ternaryos_kernel_call_tva_c(...)`

Execution control is also broader:

- caller-group spawn
- same-supervisor spawn
- named thread entry registration/spawn
- service-owned thread entry spawn
- thread identity and execution-state inspection

The mapped-TVA bridge is now the clearest sign of architectural progress. It
still is not a real trap/syscall boundary, but it does enforce caller-derived
address-space scope and explicit request/response span validation.

Assessment:

- completeness: `4/5`
- coherence: `4/5`
- readiness: `3/5`

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
- boot test coverage has continued to expand substantially; current local boot
  suite result is `2366 passed, 0 failed`

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
| HAL / boot | 3/5 | 4/5 | 2/5 | Validated hosted handoff, explicit VBox profile, still mostly hosted beyond prototype |
| Kernel core structure | 4/5 | 4/5 | 3/5 | Clear ownership, runtime split into subsystem-oriented units |
| Runtime execution flow | 4/5 | 4/5 | 2/5 | Deterministic loop with cleaner boundaries, still not real interrupt/timer-driven dispatch |
| Memory management | 3/5 | 4/5 | 3/5 | Real allocator/page table plus hosted page-backed byte store, still incomplete VM |
| Scheduling | 2/5 | 4/5 | 2/5 | Clean substrate, minimal features |
| IPC | 2/5 | 4/5 | 2/5 | Narrow mailbox primitive, now capability-enforced through ABI |
| Fault / recovery | 4/5 | 5/5 | 3/5 | Strongest subsystem |
| Interrupts | 2/5 | 3/5 | 1/5 | Simulated controller model |
| Service infrastructure | 4/5 | 4/5 | 3/5 | Control plane present and now ABI-reachable, runtime still absent |
| Device interaction | 2/5 | 4/5 | 1/5 | Device-shaped hosted wrappers |
| Persistence / storage | 4/5 | 4/5 | 3/5 | CanonStore is substantial |
| Shell / user-facing stack | 3/5 | 4/5 | 1/5 | Effective demo, not userland |

## Risk Register

### R1. Broad shared runtime contract

- evidence:
  [kernel_runtime_state.hpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_runtime_state.hpp)
- impact: high
- likelihood: high
- issue: the implementation monolith has been reduced, but the shared runtime
  state/header surface is still large and couples many subsystems tightly
- consequence: scaling and ownership clarity risk
- mitigation: continue narrowing subsystem contracts and reduce cross-unit
  dependency on the full runtime state

### R2. Diagnostics ahead of substrate

- evidence:
  [kernel_queries.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_queries.cpp),
  [kernel_views.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_views.cpp)
- impact: medium-high
- likelihood: high
- issue: there is more detailed status plumbing than low-level execution capability
- consequence: maturity can be misread
- mitigation: prioritize real execution primitives and target bring-up over additional summaries

### R3. Synthetic pager model

- evidence:
  [kernel_pager.cpp](/Users/t81dev/Code/t81-foundation/experimental/ternaryos/kernel/kernel_pager.cpp)
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

### R6. ABI boundary still hosted and fixed-block scoped

- impact: high
- likelihood: high
- issue: a real ABI now exists, but the current transport is still hosted and
  fixed-block scoped rather than a full userspace syscall/object model
- consequence: userland evolution is now unblocked conceptually, but not yet
  backed by a complete memory/object/process boundary
- mitigation: derive caller address spaces from execution context, then evolve
  fixed wire blocks toward real userspace object handling

## Maturity Assessment

Overall maturity: `advanced architectural prototype`

Category estimates:

- boot and HAL scaffolding: `60%`
- kernel runtime ownership: `70%`
- memory management: `50%`
- scheduling: `40%`
- IPC: `35%`
- fault and recovery: `65%`
- interrupts: `25%`
- service infrastructure: `50%`
- device layer: `30%`
- userland readiness: `15%`

Overall completion estimate toward a fully operational kernel: `47%`

## Roadmap to a Fully Operational Kernel

1. Expose service-owned entry descriptor state through the service query/view
   layer so executable service state is visible, not only runnable.
2. Promote address spaces into true execution units with separate page-table
   ownership.
3. Implement real interrupt/trap handling for the target platform.
4. Add a real task/process lifecycle: load, exec, exit, teardown.
5. Convert the pager from fault-workflow policy into a real VM subsystem with
   backing objects and reclaim.
6. Fully implement one real target driver rather than widening hosted wrappers.
7. Add kernel synchronization and wait primitives.
8. Move the shell and higher-level services onto the real process/service/ABI
   boundary.
9. Validate external target execution and use that evidence to drive the next
    hardening cycle.

## Final Judgment

Axion is no longer a conceptual kernel sketch. It has a real vertical slice,
cleaner subsystem decomposition than the previous audit, a meaningful
capability-aware ABI stack, and stronger execution-control coverage than a
pure hosted demo usually has. The remaining distance to a fully operational
kernel is still substantial, but the project has moved from “define the ABI”
to “harden and operationalize the ABI.” The next productive step is no longer
more internal control-plane widening. It is hardening the mapped address-space
boundary, real target interrupt/device bring-up, and a genuine executable
process model.
