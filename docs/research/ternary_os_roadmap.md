# Axion: Implementation Roadmap

This document captures the current implementation state, completed milestones, remaining work, and exit criteria for evolving the T81 Foundation stack into a standalone ternary operating system, currently named Axion.

Current working release label: `Axion v0.1.0-alpha`

Naming split for this roadmap:
- `T81 Foundation` remains the umbrella project/ecosystem
- `T81VM` remains the ternary runtime/execution substrate
- `Axion` is the operating-system name
- subsystem names like `CanonFS` and `TISC` remain unchanged

## 1. Current Position

The T81 Foundation provides a determinism-first, ternary-native computing stack (`T81`) that achieves native ternary scaling properties (base-3 / base-81) simulated over standard hardware. Today, Axion runs as a hosted prototype over standard binary platforms, but the standalone operating-system stack is no longer purely aspirational: the boot/HAL layer, ternary MMU prototype, and scheduler/IPC foundation are implemented and tested.

As of **2026-03-11**, the project has completed:

- **Phase 1:** Bootloader & HAL hosted simulation
- **Phase 2:** Ternary MMU prototype
- **Phase 3:** Kernel scheduling and IPC

Kernel integration direction is now tracked explicitly in
[RFC-00B3: Axion Governance Kernel Architecture](../../spec/rfcs/RFC-00B3-axion-kernel-architecture.md).
That RFC defines the current path after subsystem bring-up: the first
kernel-owned runtime entry now exists, checked MMU translation is now consumed
through a kernel-facing fault/reporting path, and the first persistent kernel
runtime state object now owns allocator, MMU, scheduler substrate, IPC bus,
fault-log state. Active device arbitration for the supported VirtualBox
storage/display/network profile is now attached to that same owned boundary,
the runtime now exposes a deterministic kernel-step loop with runtime
accounting, and recorded MMU faults are now delivered through that loop in FIFO
order. That loop-owned path now feeds a real kernel policy boundary: delivered
faults are routed into per-thread runtime state, the faulting thread is
quarantined deterministically, the owning process group enters a blocked fault
state, explicit group acknowledgement gates recovery, and audit-only governance
events are recorded in deterministic order. That boundary now includes a small
audit-only supervisor layer above the process-group gate. Supervisor-facing
recovery/report flows are now exposed through the same service boundary,
including deterministic pending-group, acknowledgement, and recovered-group
views. Deterministic device claim/release requests now exist through that same
boundary as the second narrow action. Request-side and action-side rejection
semantics are now explicit across that contract, and stable audit/device detail
views are exposed through it. A first service-runtime layer now exists above
that boundary, with deterministic service registration, deterministic service
unregister, deterministic service suspend/resume, stable service detail, and
richer supervisor-owned inventory. Same-supervisor process groups can now drive
those suspend/resume transitions over managed services through the same stable
contract. Explicit service health transitions now also exist through that same
stable contract, exposing unhealthy-state diagnostics and deterministic
unavailable-service rejection. Successful service lifecycle transitions now
also flow into the deterministic audit stream through the existing audit
summary surface. Supervisor-owned service inventory now also retains the latest
managed-service lifecycle transition metadata. Compact supervisor status now
also exposes managed-service lifecycle counts and latest-transition metadata.
Supervisor recovery status now also exposes managed-service lifecycle counts
and latest-transition metadata.
Fault summary now also exposes managed-service lifecycle counts and
latest-transition metadata.
Runtime status now also exposes managed-service lifecycle counts and
latest-transition metadata.
Audit summary now also exposes managed-service lifecycle counts and
latest-transition metadata.
Device summary now also exposes managed-service lifecycle counts and
latest-transition metadata.
Service status now also exposes the latest service lifecycle kind and
sequence.
Supervisor inventory entries now also expose each managed service's latest
lifecycle kind and sequence.
Process groups now also bind to explicit kernel-owned address spaces, and the
stable runtime/process-group/supervisor/service diagnostics expose
address-space ownership plus mapped-page counts.
Delivered `Unmapped` faults now also mark the owning address space as
pager-needed, while `PermissionDenied` and `InvalidTva` remain explicit policy
faults. That lifecycle convergence slice is now complete for the current
contract, the first process-memory ownership slice is in place, and the first
pager-groundwork slice is in place as well. Pager-needed address spaces now
also enter a deterministic internal handoff queue, and diagnostics distinguish
handoff-pending from handoff-dispatched state. Once the missing mapping
appears, the kernel loop now also resolves one handed-off pager-needed address
space at a time and exposes that resolved state deterministically. Repeated
unresolved faults on a worker-owned address space now also coalesce instead of
creating duplicate pager work items. Runtime and fault diagnostics now also
retain backlog/load high-water marks plus worker activation counts, and HAL
coverage proves FIFO handling across two queued address spaces. The next steps
are to keep that lifecycle contract stable while moving the kernel toward
richer kernel-owned pager-worker scheduling behavior; FIFO stall cycles and
the narrower backlog-blocked subset are now explicit diagnostics in that path,
along with the ready-behind-active subset when queued work is already
mappable, plus current ready-backlog depth and its retained high-water mark;
the kernel now also retains the stalled active address space alongside the
ready queued address space it was blocking, plus the ordinal of the latest
stall event that produced that relationship, plus the exact retained stall
ordinal for the blocked ready queued address space and the ready-backlog depth
observed at that same event; the kernel now also retains the last activated
address space and activation ordinal after the worker goes idle, plus the last
completed address space and resolution ordinal after the worker goes idle, and
the last received address space and handoff ordinal after the inbox drains,
plus the active handoff ordinal while work remains in flight and the queued-head
address/ordinal while work remains in the inbox, plus a bounded deterministic
ready-bypass selection rule when the worker is idle and the FIFO head is still
unresolved, followed by parked deferral once that single bypass has been used,
plus retained parked-cycle diagnostics while that blocked head remains
unresolved, with deferral counts now tracking parked episodes instead of every
parked loop tick, plus live parked-ready backlog diagnostics while that head
still blocks the worker, plus explicit parked-resumption diagnostics once that
head becomes ready again, including how much ready work still remained queued
behind it at resumption time, which queued handoff it was, and the resumed
blocked head's own handoff ordinal, plus when that resumed blocked head later
resolves and what queued work still remained behind it at that point, plus the
first deterministic queued activation that follows that parked resolution, and
that successor's own deterministic completion. A once-bypassed parked head now
also becomes terminal after a fixed number of repeated parked cycles, is
removed from the worker queue, and remains visible through retained terminal
diagnostics. Explicitly marked boot-critical address spaces now also
auto-resolve their missing page through a kernel-owned policy path before
later pager integration. Runtime and fault summaries now also expose explicit
boot-progress/fail state for that internal policy. That closes the current
internal boot-ready kernel slice. The local external boot-lane packaging lane
is now active and effectively complete as well: the staged ARM developer-lane
guest exports explicit boot-ready and boot-progress state through
`startup-status.txt`, the QEMU ARMv8 guest probe validates those fields end to
end, and the `x86_64` handoff package now carries aligned contract files,
recovered-artifact templates, shipped validators, and positive/negative smoke
checks. The next step is no longer more local packaging work; it is actual
external `x86_64` VirtualBox host execution and evidence return against that
contract.

That near-term kernel slice is now tracked explicitly in:

- `experimental/ternaryos/docs/kernel_execution_plan.md`

The roadmap is now centered on promotion of those layers from `experimental/` into mainline, plus delivery of the Phase 4 driver layer needed for a reboot-persistent CanonFS system. The concrete promotion environment is a **VirtualBox-first virtual machine target**: Axion should graduate from hosted process simulation into a bootable guest image that runs under VirtualBox before any real-hardware push.

---

## 2. Implemented Inventory

The foundational computational, storage, and process primitives already exist in-repo.

### Compute Layer

| Component | Location | Status |
| :--- | :--- | :--- |
| TISC ISA (frozen) | `spec/tisc-spec.md`, `core/` | Normative / Frozen |
| T81VM interpreter | `core/vm/vm.cpp` | Beta |
| T81Lang compiler | `lang/frontend/` | Beta |
| T81BigInt / T81Float | `core/types/` | Beta |

### Storage & Governance Layer

| Component | Location | Status |
| :--- | :--- | :--- |
| CanonFS | `src/canonfs/`, `include/t81/canonfs/` | Beta |
| CanonHash-81 / CanonHash384 | `include/t81/tracing/canonhash.hpp`, `runtime/tracing/canonhash81.cpp` | Beta |
| Reed-Solomon 3+2 (GF(3^9)) | `include/t81/canonfs/rs_repair.hpp`, `gf3_9.hpp` | Beta |
| Axion Policy Kernel | `kernel/axion/` | Beta |

### Process & Memory Foundations

| Component | Location | Notes |
| :--- | :--- | :--- |
| Hanoi 81-slot scheduler | `experimental/hanoi/in_memory_kernel.cpp` (`kMaxSlots = 81`) | Ethics-first boot; `SchedulerFull` guard |
| TieredMemoryPool (10-phase) | `include/t81/memory/advanced_memory_manager.hpp` (`TieredMemoryPool`, line 57) | Nanosecond allocation timing |
| RegisterFrame / frame stack | `include/t81/vm/state.hpp` (`RegisterFrame`, line 204) | Infrastructure for context-switch discipline |
| Cognitive Tiers 1–6 | `experimental/tiers/cog/` | Tier6 distributed monads, Θ₇ containment |

### TernOS Prototype Layers

| Component | Location | Status |
| :--- | :--- | :--- |
| HAL interface + hosted boot stub | `experimental/ternaryos/hal/` | Complete for Phase 1 hosted gate |
| Ternary page allocator | `experimental/ternaryos/mmu/ternary_page_alloc.*` | Complete |
| TVA + ternary radix page table MMU | `experimental/ternaryos/mmu/` | Complete for Phase 2 |
| TISC context switch machinery | `experimental/ternaryos/sched/context_switch.*` | Complete |
| 81-slot run queue + scheduler | `experimental/ternaryos/sched/` | Complete for Phase 3 |
| CanonRef-based IPC bus | `experimental/ternaryos/ipc/` | Complete |

---

## 3. Phase Status

TernOS still follows a five-phase dependency chain, but the first three phases are now implemented in hosted form.

### Promotion Track — Hosted to VirtualBox

The practical near-term execution path is no longer "hosted prototype -> immediate bare metal". It is:

1. **Hosted simulation (current):** macOS/Linux process validates HAL, MMU, scheduler, IPC, and Phase 4 wrappers.
2. **VirtualBox guest image:** bootable x86_64 image with synthetic-but-real VM hardware boundaries (RAM map, timer, storage, framebuffer, NIC) and no host OS process boundary inside the guest.
3. **Bare-metal / alternate hypervisor promotion:** only after the VirtualBox gate is stable.

Validation lanes:

1. **Primary acceptance lane:** an `x86_64` VirtualBox host that can boot-validate the official guest target.
2. **Primary local developer lane:** QEMU AArch64 + EDK2 on Apple Silicon, used for observable EFI execution and early guest bring-up; current local probes now reach the staged `BOOTAA64.EFI` developer-lane guest image directly and recover a Phase 4 startup artifact derived from the real guest bootstrap path.
3. **Secondary diagnostic lane:** ARMv8/Apple Silicon VirtualBox hosts used for artifact generation, host-capability checks, and narrow VirtualBox-specific boot-pipeline investigation only.
   That diagnostic lane now reaches compiled EFI-stub objects, a developer-lane `BOOTAA64.EFI`, packaged guest artifacts, and a headless boot probe that confirms VBox EFI can see the staged AHCI disk even though it is still not the acceptance target.

The roadmap target does not change because a developer workstation lacks `x86_64` guest validation. The host mismatch is treated as a program-execution constraint, not as an architectural reason to retarget TernOS.

Why VirtualBox first:

- It provides a repeatable x86_64 environment with well-understood virtual devices.
- It is materially closer to the roadmap's "no host OS residual" goal than the current hosted stub.
- It reduces early hardware-portability noise while forcing the HAL and driver layers to cross a real VM boundary.
- It gives a concrete demo/distribution target: a VM image others can boot locally.

Initial VirtualBox hardware profile (confirmed against the upstream source tree):

- **Firmware:** VBox EFI firmware (`src/VBox/Devices/EFI/DevEFI.cpp`, `Firmware/`)
- **Storage:** AHCI first, NVMe second (`src/VBox/Devices/Storage/DevAHCI.cpp`, `DevNVMe.cpp`)
- **Network:** E1000 first (`src/VBox/Devices/Network/DevE1000.cpp`)
- **Display:** VMSVGA / VGA path (`src/VBox/Devices/Graphics/DevVGA-SVGA.cpp`, `DevVGA.cpp`)
- **Timers / interrupts:** HPET and IOAPIC (`src/VBox/Devices/PC/DevHPET.cpp`, `DevIoApic.cpp`)

The first bootable guest should deliberately target this conservative profile rather than trying to support every VirtualBox device model on day one.

### Phase 1 — Bootloader & HAL

**Status:** Complete in hosted simulation.  
**Gate:** v1.5 hosted equivalent satisfied; VirtualBox guest promotion is the next concrete target, with bare-metal UEFI still deferred.

- **Binary-to-Ternary Bootstrap:** A minimalist Type-1 hypervisor guest or UEFI bootloader that initializes hardware (CPU, memory, storage, network) and transfers full control to `T81VM` with no host OS residual.
- **Hardware Abstraction Layer (HAL):** Translates binary CPU interrupts, I/O port signals, and physical memory addresses into canonical TISC representations. See [RFC-00B0: HAL Specification](../../spec/rfcs/RFC-00B0-hal-spec.md) for the scoping decision between unikernel, Type-1 hypervisor, and raw UEFI approaches.

Implemented outcome:

- `hal_main` validates `BootContext`, performs ethics-first boot checks, and now hands off to the first Axion kernel-owned runtime entry.
- A hosted macOS/Linux boot stub provides a synthetic memory map and invokes the HAL entrypoint.
- Shadow binary interrupt dispatch is implemented without modifying the frozen TISC ISA.
- `axion_kernel_main(...)` now exists as the first kernel-owned runtime handoff target, bootstraps runtime state from `BootContext`, and now drives deterministic scheduler dispatch plus CanonRef-safe IPC through that owned runtime state.

VirtualBox promotion deliverables:

- Replace the purely hosted stub with a bootable guest entry path suitable for a VirtualBox VM.
- Enumerate the VM memory map and timer/interrupt surfaces from the guest environment rather than synthesizing them in-process.
- Produce a guest image and documented VirtualBox boot recipe for repeatable external testing.
- Lock the first supported VM configuration to VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC.

### Phase 2 — Ternary Memory Management

**Status:** Complete for the flat page-table prototype.  
**Gate:** v1.6 satisfied in hosted tests.

- **Ternary Paging:** Virtual memory where page boundaries are powers of 3 (e.g., 3¹⁰ = 59,049 trytes per page). Requires an address translation design that maps the host's 48-bit virtual address space to a trit-addressed space — this is non-trivial and warrants a dedicated RFC.
- **Process Isolation:** Axion engine extended to enforce rigid memory sandboxing between isolated TISC processes. `RegisterFrame` (line 204, `vm/state.hpp`) provides the per-thread register snapshot; wiring it to a context-switch path is the primary implementation target.
- **Allocator Integration:** `TieredMemoryPool` (`advanced_memory_manager.hpp:57`) becomes the kernel allocator; its 10-phase tier model maps naturally to Cognitive Tier boundaries.

Implemented outcome:

- RFC-00B1 defines a 30-trit TVA layout with a 10-trit page offset and 20-trit VPN.
- `TernaryPageAllocator` manages physical pages using balanced-ternary page states.
- The current MMU now uses a 20-trit ternary radix page table behind the stable `mmu_*` API.
- The current MMU also classifies `InvalidTva`, `Unmapped`, and `PermissionDenied`
  faults through a checked translation path.

### Phase 3 — Kernel Scheduling & IPC

**Status:** Complete.  
**Gate:** v1.7 satisfied with deterministic multi-thread scheduling tests.

- **Pre-emptive Ternary Scheduler:** Generalize the Hanoi 81-slot cooperative scheduler (`kMaxSlots = 81`, `in_memory_kernel.cpp:125`) into a pre-emptive, priority-aware dispatcher managing multiple concurrent `T81VM` instances.
- **Ternary Context Switching:** Pause a TISC thread mid-execution, snapshot ternary registers and flags via `RegisterFrame`, and restore them on reschedule.
- **IPC via CanonFS:** Define canonical message-passing contracts using CanonFS object identity — processes exchange CanonRef handles rather than raw pointers, preserving determinism and Axion audit trails.

Implemented outcome:

- The run queue preserves deterministic insertion-order round-robin behavior across 81 slots.
- `Scheduler::tick()` performs save/preempt/select/restore and returns `true` only for genuine thread switches.
- IPC is implemented as CanonRef-safe FIFO inboxes with per-recipient depth caps.
- RFC-00B3 now defines the active kernel path after the first implemented
  handoff: move beyond the thin runtime bootstrap and start routing MMU faults,
  scheduler state, IPC, and device arbitration through a kernel-owned state
  object.

### Phase 4 — Device Drivers & I/O

**Status:** Not complete. This is the active major roadmap item.

- **Binary Protocol Wrappers:** PCIe, NVMe, and Ethernet drivers receive binary streams and translate them into canonical ternary structs at the system boundary.
- **Ternary Text Format (TTF):** Character encoding standard bridging ASCII/UTF-8 into packed trytes for framebuffer and terminal rendering.

Near-term deliverables:

- NVMe wrapper and block-device bridge for CanonFS
- Framebuffer and TTF terminal path
- Ethernet wrapper and ternary packet representation
- Reboot-cycle validation for CanonFS durability

VirtualBox-specific promotion scope:

- Block storage should first target the VirtualBox AHCI controller, with NVMe treated as a follow-on optimization once the CanonFS reboot gate is stable.
- Framebuffer output should first target the VirtualBox VMSVGA/VGA guest display path before any physical GPU/MMIO strategy.
- Network promotion should first target the VirtualBox E1000 device with deterministic frame translation at the guest boundary.
- Timer and interrupt wiring should first target the HPET + IOAPIC path the guest can observe consistently.

### Phase 5 — Userland Ecosystem (early v2.x)

**Status:** Started as a minimal hosted scaffold; still deferred for real interactive userland until the driver layer is stable.

- **Ternary Shell (TUI):** Deterministic CLI running pure TISC code for process, volume, and policy management.
- **Network Stack:** Canonical TCP/IP translation layer for deterministic network routing over binary hardware.

Current starting point:

- A first hosted shell/TUI demo now exists over the VirtualBox guest-bootstrap path.
- That shell path now also has an FTXUI frontend using the repo's standard TUI stack, with a deterministic snapshot mode for review and future automation.
- It now executes a minimal typed shell model (`help`, `profile`, `name set <label> <ref>`, `name ls`, `object pin <kind> <name> <ref>`, `object ls`, `object show <name>`, `show profile`, `session status`, `session checkpoint`, `session export`, `session import <ref>`, `session diff <ref>`, `session run <ref>`, `session show durable`, `show session`, `session refs`, `store put <text>`, `store put script <line>|<line>|...>`, `store put ref <ref>`, `store cp <ref>`, `store ls`, `store get <ref>`, `show ref <canonref>`, `store rm <ref>`, `history`, `history show session`, `history show object <ref>`, `history use <ref>`, `history show durable`, `clear`), persists shell history through CanonStore, reboots, recovers that history, and renders the resulting shell page through the VMSVGA-backed ternary framebuffer.
- The live TUI now accepts typed command input directly, including quoted store payloads, session-state reporting, shell-local durable ref inspection, explicit session checkpointing/export into canonical objects, store ref listing/retrieval/removal, object-history lookup, first object-native read/write commands, and transcript clearing. It also now separates shell-local transcript state from durable CanonStore state, but it still does not claim TISC userland execution or a broad process model; it establishes the first user-facing seam on top of the Phase 4 storage/display path without pretending the full shell exists yet.

---

## 4. Phased Delivery Plan

| Phase | Milestone | Key Deliverable | Gate Condition | Target |
| :---: | :--- | :--- | :--- | :--- |
| 1 | HAL RFC + Bootloader PoC | Hosted HAL, boot validation, interrupt shim | TISC `NOP`/`HALT` executes with no host OS-equivalent supervisor path | v1.5 |
| 2 | Ternary MMU | TVA model, page allocator, ternary radix page table | T81VM allocates from ternary page boundaries | v1.6 |
| 3 | Pre-emptive Scheduler | 81-slot run queue, context-switch, IPC | Two concurrent TISC threads run deterministically | v1.7 |
| 4 | Driver Layer | VirtualBox guest storage + framebuffer + network wrappers | CanonFS read/write survives a reboot cycle inside a VirtualBox guest | v2.0 |
| 5 | Userland | Ternary shell + network stack | Interactive TISC session over bare metal | v2.x |

VirtualBox promotion checkpoint:

- **v1.8 / pre-v2.0 promotion goal:** a bootable VirtualBox guest image executes the existing Phase 1-3 stack and exposes the Phase 4 hosted wrappers through VM devices.
- **First supported VM profile:** x86_64, VBox EFI, AHCI boot/storage, E1000 NIC, VMSVGA display, HPET/IOAPIC timing.

Current completion snapshot:

- **Phases 1-3:** Implemented and passing in hosted tests
- **Phase 4:** In progress / next major milestone
- **Phase 5:** Started as a hosted built-in shell/TUI scaffold

---

## 5. Current Success Criteria

The original v1.5 boot criterion has been met in hosted form. The next material success criteria are:

> **Phase 4 (v2.0): CanonFS read/write survives a reboot cycle through the device-wrapper layer inside a VirtualBox guest.**

That gate turns the current in-memory TernOS substrate into a minimally persistent operating system. In parallel, a promotion track remains open for the stricter bare-metal interpretation of the Phase 1 gate:

> **A VirtualBox-bootable guest image loads the T81VM binary, executes a TISC program (`NOP` loop or `HALT`), and terminates cleanly with no Linux/macOS host process involved inside the guest.**

---

## 6. Open Questions & Risks

1. **Phase 1 promotion target is now concrete but still incomplete:** the roadmap now assumes a VirtualBox x86_64 guest using VBox EFI, AHCI, E1000, VMSVGA, and HPET/IOAPIC, but the guest image format and boot packaging flow still need to be implemented.
   The local Apple Silicon VirtualBox host can help stage artifacts, but final boot proof still requires an `x86_64`-capable validation host.
2. **The physical/virtual address gap is resolved only for the current prototype:** RFC-00B1 adopts a "narrow virtual" TVA design. If future requirements exceed the current 30-trit/205 TB space, a wider VPN design will be needed.
3. **TISC interrupt semantics are now narrowed but still not fully integrated:** RFC-00B5 formalizes the current governed event interrupt model and confirms the frozen ISA does not require a trap-return opcode for the current Axion path, but later priority, nesting, and audit-surface choices remain open.
4. **Determinism under pre-emption is not fully closed:** Scheduling is deterministic today, but Axion governance has not yet been fully extended to model async interleavings.
5. **Driver correctness becomes the next systems risk:** AHCI/E1000/VMSVGA-facing wrappers must preserve canonical ternary representations at the VirtualBox guest boundary without leaking binary host assumptions upward.
6. **VirtualBox coupling must stay tactical:** the VM target should accelerate promotion, not become a permanent architectural dependency. The HAL and driver contracts must remain portable to QEMU, other hypervisors, and eventual bare metal.

---

## 7. Conclusion

The most mathematically complex components — TISC ISA, compiler toolchain, CanonHash-81, Reed-Solomon parity, and the Axion policy engine — are already shipped and Beta-stable. On the Axion path specifically, the foundational OS substrate is now in place: hosted boot/HAL, ternary paging, deterministic scheduling, context switching, and CanonRef-safe IPC are implemented and tested.

The critical path has moved. The next milestone is no longer an abstract "driver layer" in isolation; it is a **VirtualBox-bootable Axion guest** with reboot-persistent CanonFS validation. That makes the remaining work concrete: close the VM boot path, bind Phase 4 wrappers to VirtualBox-visible devices, and preserve portability so later promotion to other hypervisors or bare metal remains viable.
