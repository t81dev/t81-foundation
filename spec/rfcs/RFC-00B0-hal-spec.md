# RFC-00B0: Hardware Abstraction Layer (HAL) for TernOS

**Status:** Draft
**Date:** 2026-03-10
**Author:** @t81dev
**Depends on:** TISC ISA (frozen, `spec/tisc-spec.md`), Axion Policy Kernel (`kernel/axion/`)
**Blocks:** TernOS Phase 1 (bootloader PoC, v1.5 gate)

---

## 1. Summary

This RFC defines the scope, interface contract, and implementation strategy for the Hardware Abstraction Layer (HAL) that bridges binary host hardware to the ternary T81VM runtime. The HAL is the mandatory first deliverable for TernOS Phase 1 and now serves two distinct purposes:

1. the current hosted simulation path used by the existing prototype, and
2. the next promotion target: a VirtualBox-bootable guest image that runs TernOS without a Linux/macOS process boundary inside the guest.

---

## 2. Problem Statement

The T81VM currently executes as a hosted process on a binary OS (Linux/macOS). Promotion beyond hosted execution requires:

1. A mechanism to initialize hardware (CPU, memory, storage, interrupts) without a host OS.
2. A translation boundary that converts binary hardware signals (interrupts, I/O port reads, DMA) into canonical TISC-addressable representations.
3. A design that does **not** modify the frozen TISC ISA (`spec/tisc-spec.md`) or violate Axion governance contracts.

The roadmap now narrows the immediate target: before bare metal, TernOS should boot as a **VirtualBox guest** on x86_64. That gives the project a reproducible, externally shareable VM boundary while keeping later portability open.

---

## 3. Decision: Execution Model

Three candidate approaches are evaluated below. The decision is no longer "pick one forever before any implementation" because hosted HAL work already exists. Instead, this RFC must choose the **first non-hosted promotion path** for the current prototype.

| Option | Description | Pros | Cons |
| :--- | :--- | :--- | :--- |
| **A. UEFI Application** | T81VM compiled as a UEFI PE binary; boots via firmware directly | Minimal footprint; no hypervisor dependency; runs on bare x86_64/ARM | No process isolation; limited interrupt handling; less convenient for externally shareable VM demos |
| **B. VirtualBox Guest** | T81VM boots as an x86_64 VirtualBox VM guest using virtual firmware/devices | Repeatable environment; concrete VM boundary; easy external distribution; virtual storage/display/NIC available | Couples early promotion work to one hypervisor/device model; still requires guest image/boot flow work |
| **C. Type-1 Hypervisor Guest (e.g., Xen/KVM dom0-less)** | T81VM runs as a privileged guest with direct hardware passthrough | Full hardware access; established interrupt model; memory isolation available | Complex setup; harder local reproducibility; larger operational burden |
| **D. Unikernel (e.g., Unikraft / custom)** | T81VM + minimal libc compiled as a single-address-space kernel image | Tiny TCB; no host OS; deterministic boot sequence | Requires porting libc/POSIX subset; more engineering effort upfront |

**Recommendation:** Option B (VirtualBox Guest) as the first non-hosted promotion target. It gives the project a bootable, reproducible x86_64 image that others can run locally, while still exercising a real VM hardware boundary. Option A (UEFI Application) remains a plausible later path for hardware-near promotion, and Option D (Unikernel) remains a longer-term production-oriented direction.

### 3.1 Selected first-target VirtualBox device profile

The first supported guest profile should be intentionally narrow:

- **Firmware:** VBox EFI
- **Storage:** AHCI
- **Network:** E1000
- **Display:** VMSVGA/VGA
- **Timer / interrupt path:** HPET + IOAPIC
- **Architecture:** x86_64 only

This profile is chosen because the upstream VirtualBox source tree already carries these components explicitly:

- `src/VBox/Devices/EFI/DevEFI.cpp`
- `src/VBox/Devices/Storage/DevAHCI.cpp`
- `src/VBox/Devices/Network/DevE1000.cpp`
- `src/VBox/Devices/Graphics/DevVGA-SVGA.cpp`
- `src/VBox/Devices/PC/DevHPET.cpp`
- `src/VBox/Devices/PC/DevIoApic.cpp`

`DevNVMe.cpp` exists and should remain a Phase 4 follow-on once AHCI-backed persistence is stable.

### 3.2 Promotion strategy

The intended sequence is:

1. **Hosted HAL (already implemented):** synthetic memory map and in-process boot handoff.
2. **VirtualBox guest HAL:** guest image enumerates VM-visible memory/interrupt/storage/display/network surfaces and invokes `hal_main`.
3. **Later portability work:** adapt the same HAL contract to UEFI, other hypervisors, or bare metal without changing upper layers.

---

## 4. HAL Interface Contract

The HAL exposes the following surface to the T81VM. All types are in the `t81::hal` namespace.

### 4.1 Memory Map

```cpp
namespace t81::hal {

// Describes a contiguous region of physical memory available to T81VM.
struct MemoryRegion {
  uint64_t base_phys;   // Physical base address (binary)
  uint64_t size_bytes;  // Region size in bytes
  bool     writable;
  bool     executable;
};

// Returns all usable memory regions discovered at boot.
std::vector<MemoryRegion> enumerate_memory();

} // namespace t81::hal
```

Ternary address mapping: each `MemoryRegion` is subsequently carved into ternary pages (3¹⁰ = 59,049 tryte pages) by the Ternary MMU layer (Phase 2). The HAL does not perform this translation — it exposes raw physical layout only.

### 4.2 Interrupt Dispatch

TISC has no interrupt or trap-return opcode in the frozen ISA. The HAL handles this via a **shadow binary dispatch table** — a small C++ trampoline registered at boot that catches hardware interrupts, translates them to a canonical `AxionTrap` event, and injects them into the Axion policy engine for enforcement before forwarding to the VM.

```cpp
namespace t81::hal {

enum class InterruptSource : uint8_t {
  Timer    = 0,
  Storage  = 1,
  Network  = 2,
  Keyboard = 3,
  Unknown  = 0xFF,
};

struct HardwareInterrupt {
  InterruptSource source;
  uint64_t        timestamp_ns;
  uint64_t        payload;      // device-specific data (e.g., sector ID for storage)
};

// Register a handler. Called from TISC context via Axion trap injection.
using InterruptHandler = std::function<void(const HardwareInterrupt&)>;
void register_interrupt_handler(InterruptSource, InterruptHandler);

} // namespace t81::hal
```

This design preserves ISA immutability: the interrupt path never touches TISC opcodes directly.

### 4.3 I/O Port Abstraction

```cpp
namespace t81::hal {

// Read/write to binary I/O ports; results are byte-width at HAL boundary.
// Higher layers are responsible for ternary encoding.
uint8_t  io_read8 (uint16_t port);
uint16_t io_read16(uint16_t port);
void     io_write8 (uint16_t port, uint8_t  val);
void     io_write16(uint16_t port, uint16_t val);

} // namespace t81::hal
```

### 4.4 Boot Handoff

```cpp
namespace t81::hal {

struct BootContext {
  std::vector<MemoryRegion> memory_map;
  uint64_t                  kernel_load_address;
  uint64_t                  stack_top;
  bool                      ethics_boot_required;  // Always true in production.
};

// Entry point called by the guest boot shim.
// Initializes HAL, runs ethics-first boot via Axion (Θ₁–Θ₉),
// then transfers control to T81VM::run().
[[noreturn]] void hal_main(const BootContext&);

} // namespace t81::hal
```

`hal_main` mirrors the `InMemoryKernel::boot()` ethics-first pattern (`experimental/hanoi/in_memory_kernel.cpp:25`): it evaluates Θ₁–Θ₉ before any TISC dispatch begins.

---

## 5. Binary ↔ Ternary Address Translation (Deferred)

The precise mapping of 48-bit x86_64 virtual addresses to trit-addressed TISC space is **out of scope for this RFC** and is deferred to a follow-on RFC (RFC-00B1: Ternary MMU). The HAL's `enumerate_memory()` output feeds directly into that design. Key open questions:

- Padding strategy: widen trit-address space to cover 2^48, or narrow it?
- Alignment: require all allocations to be 3^k-aligned?
- Supervisor vs. user space split in trit-space?

---

## 6. Files / Deliverables

| File | Purpose |
| :--- | :--- |
| `include/t81/hal/hal.hpp` | HAL interface contract (§4) |
| `src/hal/virtualbox_guest_stub.cpp` or equivalent promotion target | Guest entry point + `hal_main` handoff for the first bootable VM image |
| `src/hal/interrupt_table.cpp` | Shadow binary interrupt dispatch table |
| `src/hal/virtualbox_platform.cpp` or equivalent | Enumerate VBox EFI memory map, HPET/IOAPIC interrupt surfaces, and first-target devices |
| `tests/cpp/hal_boot_test.cpp` | Unit test: `BootContext` construction + ethics-first gate |
| VM image packaging / boot recipe docs | Repeatable VirtualBox build-and-run flow for external validation |
| `CMakeLists.txt` | Add `t81_hal` library target; wire `hal_boot_test` and guest build targets |

---

## 7. Acceptance Criteria

- [ ] `t81_hal` library compiles cleanly under AppleClang 17, Clang 18, GCC 14.
- [ ] `hal_boot_test` passes: `BootContext` with `ethics_boot_required = true` triggers Θ₁–Θ₉ evaluation before any TISC dispatch.
- [ ] A VirtualBox guest image boots, constructs a real `BootContext`, and executes a single TISC `NOP` + `HALT` sequence with no Linux/macOS host process involved inside the guest.
- [ ] Guest-visible memory map, HPET/IOAPIC interrupt path, and AHCI storage surface are plumbed through HAL interfaces rather than synthetic in-process stubs.
- [ ] The first supported guest profile is documented as VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC on x86_64.
- [ ] All existing TernOS tests continue to pass (HAL promotion is additive; no ISA or Axion changes).

---

## 8. Open Questions

1. **Guest boot format:** What is the cleanest build artifact for VBox EFI boot in this project: raw disk image, ISO, or another VirtualBox-friendly package?
2. **AHCI first vs. NVMe first:** Upstream VirtualBox exposes both, but should TernOS intentionally defer NVMe until AHCI-backed CanonFS persistence is stable?
3. **Display depth:** Is a minimal VGA/VMSVGA text-capable output sufficient for the first guest, or is any accelerated SVGA functionality required at all?
4. **CI strategy:** Is a headless VirtualBox run acceptable for CI, or should the first automation target still be QEMU with VirtualBox reserved for developer/demo validation?
5. **Portability boundary:** How do we keep VirtualBox-specific details isolated so later UEFI, QEMU, or bare-metal promotion does not require an HAL contract redesign?
