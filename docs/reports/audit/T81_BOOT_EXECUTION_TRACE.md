# T81 Foundation — Freestanding Axion OS Boot Execution Trace

**Audit Designation:** T81-SIA-001
**Reference Document:** Boot Sequence & Bare-Metal Execution Analysis
**Evaluated Target Architectures:** AArch64 (QEMU virt, cortex-a57) and x86_64 (QEMU q35 / VirtualBox)
**Primary Source Paths:** `ternaryos/hal/`, `ternaryos/kernel/`, `ternaryos/dev/`, `ternaryos/mmu/`, `ternaryos/sched/`

---

## 1. Complete Boot Sequence Trace

The freestanding Axion OS implementation relies on a three-phase transition model: UEFI Boot Services → Bare-Metal C Probe → Freestanding C++ Bridge & Microkernel.

```
+-----------------------------------------------------------------------------------+
| Phase 1: UEFI Firmware (EDK2 / OVMF)                                              |
| Entry: efi_main() [qemu_armv8_efi_stub.c / qemu_x86_64_efi_stub.c]               |
| Actions: Initialize EFI console, query memory map, call ExitBootServices()        |
+-----------------------------------------------------------------------------------+
                                         │
                                         ▼ (Physical Execution, Ring 0 / EL1)
+-----------------------------------------------------------------------------------+
| Phase 2: Bare-Metal Entry Probe                                                   |
| Entry: qemu_bare_kernel_entry() [qemu_slice6_bare_kernel.c] (AArch64)             |
|        qemu_x86_64_bare_kernel_entry() [qemu_x86_64_bare_kernel.c] (x86_64)       |
| Actions: Raw MMIO/Port-IO init (PL011 UART / COM1 16550A); diagnostic banner      |
+-----------------------------------------------------------------------------------+
                                         │
                                         ▼ (Freestanding C++, -nostdlib, -ffreestanding)
+-----------------------------------------------------------------------------------+
| Phase 3: Freestanding C++ Bridge & Microkernel                                    |
| Entry: qemu_cpp_bridge_entry() [qemu_slice6_cpp_bridge.cpp] (AArch64)             |
|        qemu_x86_64_cpp_bridge_entry() [qemu_x86_64_cpp_bridge.cpp] (x86_64)       |
| Actions: GICv3 / PIC setup, TTBR0 MMU tables, Virtio-blk probe, Cooperative Sched|
+-----------------------------------------------------------------------------------+
                                         │
                                         ▼ (Exception Level Drop via ERET)
+-----------------------------------------------------------------------------------+
| Phase 4: EL0 User-Space Roundtrip & Process Execution                             |
| Entry: axion_el0_entry() [axion_el0_init.S] & proc_entry [el0_process_stub.S]     |
| Actions: Execute position-independent AArch64 stub; issue SVC #1/#3; SVC #2 exit  |
+-----------------------------------------------------------------------------------+
```

---

## 2. Detailed Stage-by-Stage Entry Trace

### Stage 1: Firmware & Bootloader Entry
- **Source Files:**
  - AArch64: `ternaryos/hal/qemu_armv8_efi_stub.c` (`efi_main`)
  - x86_64: `ternaryos/hal/qemu_x86_64_efi_stub.c` (`efi_main`)
  - VirtualBox Shim: `ternaryos/hal/virtualbox_x86_64_efi_shim.c`, `ternaryos/hal/virtualbox_armv8_efi_shim.c`
- **Mechanism:**
  - The binary is compiled as a PE32+ executable (`BOOTAA64.EFI` or `BOOTX64.EFI`) using Clang with `--target=aarch64-pc-windows-msvc` or `--target=x86_64-pc-windows-msvc`.
  - UEFI firmware locates the EFI system partition (ESP) at `\EFI\BOOT\BOOTAA64.EFI` and transfers execution to `efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table)`.
  - `efi_main` queries `system_table->ConOut` to emit the initial boot banner: `"Axion QEMU AArch64 EDK2 slice6\r\n"`.

### Stage 2: ExitBootServices Transition
- **Source Code:**
  `ternaryos/hal/qemu_armv8_efi_stub.c:338–365`
- **Mechanism:**
  - Calls `bs->GetMemoryMap` to capture the current descriptor layout and acquire `map_key`. Handles `EFI_BUFFER_TOO_SMALL` by dynamically expanding the buffer.
  - Executes `bs->ExitBootServices(image_handle, map_key)`.
  - Upon return with `EFI_SUCCESS`, the EFI runtime is completely terminated. Firmware interrupt handlers, boot service tables, and standard memory allocators are invalidated.
  - The stub immediately invokes the bare-metal kernel entry function: `qemu_bare_kernel_entry()` on AArch64 or `qemu_x86_64_bare_kernel_entry(tsc_freq_hz)` on x86_64.

### Stage 3: Bare-Metal Hardware Initialization
- **Source Files:**
  - `ternaryos/hal/qemu_slice6_bare_kernel.c`
  - `ternaryos/hal/qemu_x86_64_bare_kernel.c`
- **Hardware Initialized:**
  - **AArch64:** Direct 32-bit MMIO access to ARM PL011 UART at physical base address `0x09000000` (offsets: Data Register `0x000`, Flag Register `0x018`). Polls `TXFF` flag before writing characters. Emits `"[axion] bare-metal EL1 kernel entry\r\n"`.
  - **x86_64:** Standard x86 port I/O via inline assembly (`outb`/`inb`). Programs COM1 (16550A UART at port `0x3F8`): disables interrupts, sets DLAB (divisor 1 = 115200 baud), configures 8-N-1 line control, and clears FIFO buffer. Emits `"[axion] bare-metal x86_64 entry\r\n"`.

### Stage 4: Freestanding C++ Bridge Handoff
- **Source Files:**
  - `ternaryos/hal/qemu_slice6_cpp_bridge.cpp` (`qemu_cpp_bridge_entry`)
  - `ternaryos/hal/qemu_x86_64_cpp_bridge.cpp` (`qemu_x86_64_cpp_bridge_entry`)
- **Execution Constraints:**
  - Compiled with `-ffreestanding -fno-exceptions -fno-rtti -fno-use-cxa-atexit -nostdlib`.
  - No C++ runtime library (`libc++` / `libstdc++`), no heap allocations (`new`/`malloc`), no global static constructors relying on dynamic initialization.
- **Actions Performed:**
  - **Interrupt Architecture:** Calls `bridge_hw_init_aarch64()` in `qemu_slice6_bridge_irq.cpp`. Configures ARM GICv3 Distributor and Redistributor interfaces, unmasks generic timer PPI 30, and programs physical timer countdown (`CNTP_TVAL_EL0`).
  - **Storage Probe:** Probes Virtio-blk MMIO transport at base `0x0A003C00` (AArch64) or Virtio-blk-pci legacy I/O ports via PCI configuration space scan (x86_64). Verifies device magic `0x74726976` (`virt`) and device ID `2`. Performs a 3-step sector check: reads LBA 0 to assert CanonFS superblock magic `CST1`, writes test pattern to LBA 1, and reads back to verify match.
  - **Memory Virtualization:** Invokes `el0_mmu_init()` (`qemu_slice6_el0_mmu.cpp`). Constructs static identity-mapped page tables for kernel code/MMIO, and allocates controlled user pages in `TTBR0_EL1`.

### Stage 5: User-Space (EL0) Execution & System Call Handling
- **Source Files:**
  - `ternaryos/hal/axion_el0_init.S`, `ternaryos/hal/el0_process_stub.S`
  - `ternaryos/hal/aarch64_trap_entry.cpp`, `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp`
  - `ternaryos/hal/canon_exec_loader.cpp` (`canon_exec_load_and_run`)
- **Mechanism:**
  - Kernel sets `ELR_EL1` to process entry PA, sets `SP_EL0` to allocated stack top (`el0_mmu_proc_stack_top()`), sets `SPSR_EL1 = 0x3C0` (EL0t with DAIF masked), and executes `ERET`.
  - EL0 code runs native AArch64 machine instructions. It invokes system services via `SVC` instructions:
    - `SVC #0`: GetThreadIdentity (legacy direct).
    - `SVC #1`: KernelCall wire-format ABI (encodes `KernelCallKind`).
    - `SVC #2`: ExitThread (patches `ELR_EL1` back to `g_axion_el1_return_pc` and ERETs back to kernel).
    - `SVC #3`: WriteSerial (raw PL011 string output).
  - Synchronous exception vector at `aarch64_exception_vectors.S` routes `SVC` traps through `aarch64_sync_el0_trap_handler()` to `axion_svc_handler_aarch64()`.

### Stage 6: Interactive Shell & Cooperative Scheduler
- **Source Code:** `ternaryos/hal/qemu_slice6_cpp_bridge.cpp:1600–1680`
- **Mechanism:**
  - Enters a polling loop on PL011 UART / COM1 serial input.
  - Schedules background threads cooperative-round-robin either via polling counter ticks or hardware timer PPI 30 ticks (`bridge_timer_irq_tick()`).
  - Implements an interactive terminal parser responding to commands: `help`, `version`, `status`, `policy`, `canonfs`, `ls`, `cat`, and `run`.

---

## 3. Critical Boot Architecture Findings

1. **Independent Kernel Verification:**
   The image boots without Linux, Windows, or another guest operating system. It executes directly on bare physical hardware / hypervisor virtual hardware.
2. **Missing TISC Execution in Freestanding Kernel:**
   The freestanding kernel does **not** host the T81VM. It cannot parse `.tisc` binaries or evaluate TISC opcodes.
3. **Synthetic Machine-Code Workloads:**
   The processes executed in EL0 (`canon_exec_load_and_run`, `canon_ipc_load_and_run`, etc.) are flat ARM64 binary stubs assembled from `.S` files and written to raw disk sectors. They test CPU privilege drops and MMU table swaps, not ternary computation.
4. **Shutdown & Recovery:**
   Axion OS has no ACPI power-management driver or UEFI runtime reset wrapper. Shutdown commands loop into CPU halt (`WFI` / `HLT`).
