# RFC-00B0: Hardware Abstraction Layer (HAL) for TernOS

**Status:** Draft
**Date:** 2026-03-10
**Author:** @t81dev
**Depends on:** TISC ISA (frozen, `spec/tisc-spec.md`), Axion Policy Kernel (`kernel/axion/`)
**Blocks:** TernOS Phase 1 (bootloader PoC, v1.5 gate)

---

## 1. Summary

This RFC defines the scope, interface contract, and implementation strategy for the Hardware Abstraction Layer (HAL) that bridges binary host hardware to the ternary T81VM runtime. The HAL is the mandatory first deliverable for TernOS Phase 1 and must be ratified before bootloader work begins.

---

## 2. Problem Statement

The T81VM currently executes as a hosted process on a binary OS (Linux/macOS). Bare-metal execution requires:

1. A mechanism to initialize hardware (CPU, memory, storage, interrupts) without a host OS.
2. A translation boundary that converts binary hardware signals (interrupts, I/O port reads, DMA) into canonical TISC-addressable representations.
3. A design that does **not** modify the frozen TISC ISA (`spec/tisc-spec.md`) or violate Axion governance contracts.

---

## 3. Decision: Execution Model

Three candidate approaches are evaluated below. **This RFC must resolve to exactly one before implementation begins.**

| Option | Description | Pros | Cons |
| :--- | :--- | :--- | :--- |
| **A. UEFI Application** | T81VM compiled as a UEFI PE binary; boots via firmware directly | Minimal footprint; no hypervisor dependency; runs on bare x86_64/ARM | No process isolation; limited interrupt handling; firmware APIs are binary-only |
| **B. Type-1 Hypervisor Guest (e.g., Xen/KVM dom0-less)** | T81VM runs as a privileged guest with direct hardware passthrough | Full hardware access; established interrupt model; memory isolation available | Complex setup; hypervisor itself is a significant binary dependency |
| **C. Unikernel (e.g., Unikraft / custom)** | T81VM + minimal libc compiled as a single-address-space kernel image | Tiny TCB; no host OS; deterministic boot sequence | Requires porting libc/POSIX subset; more engineering effort upfront |

**Recommendation:** Option A (UEFI Application) for the v1.5 PoC milestone. It has the lowest barrier to a first boot and cleanly validates the minimal viable boot target. Option C (Unikernel) is the preferred long-term target for v2.0 bare-metal production.

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

// Entry point called by the UEFI stub / hypervisor boot shim.
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

## 6. Files to Be Created

| File | Purpose |
| :--- | :--- |
| `include/t81/hal/hal.hpp` | HAL interface contract (§4) |
| `src/hal/uefi_stub.cpp` | UEFI PE entry point + `hal_main` implementation |
| `src/hal/interrupt_table.cpp` | Shadow binary interrupt dispatch table |
| `tests/cpp/hal_boot_test.cpp` | Unit test: `BootContext` construction + ethics-first gate |
| `CMakeLists.txt` | Add `t81_hal` library target; wire `hal_boot_test` |

---

## 7. Acceptance Criteria

- [ ] `t81_hal` library compiles cleanly under AppleClang 17, Clang 18, GCC 14.
- [ ] `hal_boot_test` passes: `BootContext` with `ethics_boot_required = true` triggers Θ₁–Θ₉ evaluation before any TISC dispatch.
- [ ] UEFI stub image (`t81_uefi.efi`) boots in QEMU (`-bios OVMF`) and executes a single TISC `NOP` + `HALT` sequence with no host OS.
- [ ] All existing 335 tests continue to pass (HAL is additive; no ISA or Axion changes).

---

## 8. Open Questions

1. **UEFI toolchain:** UEFI PE compilation requires `gnu-efi` or `EDK2`. Which is preferred given the existing CMake setup?
2. **QEMU vs. real hardware for CI:** Can the v1.5 gate be satisfied by QEMU alone, or do we need a physical test node?
3. **ARM support:** Is the v1.5 bootloader target x86_64-only, or must it also cover AArch64?
