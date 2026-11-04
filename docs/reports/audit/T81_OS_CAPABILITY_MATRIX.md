# T81 Foundation — Operating System Capability Matrix

**Audit Designation:** T81-SIA-001
**Reference Document:** 18-Point Conventional OS Capability Assessment of Freestanding Axion OS
**Evaluation Scope:** Standalone bare-metal/EFI binaries (`BOOTAA64.EFI`, `BOOTX64.EFI`), HAL implementations, and kernel drivers under `ternaryos/`.

---

## 1. Operating System Capability Evaluation Matrix

Each capability is classified into one of the following strict categories:
- **IMPLEMENTED AND DEMONSTRATED**: Functional code exists, builds, and is verified by automated execution in tests or CI.
- **IMPLEMENTED BUT NOT END-TO-END VERIFIED**: Code is written in the repository, but full end-to-end functionality is unverified or blocked by missing host environments.
- **PARTIAL**: Subsystem provides minimal primitives or hardcoded mocks, lacking complete real-world facilities.
- **DESIGN ONLY**: Specified in RFC documents, but lacking functional implementation in the source tree.
- **ABSENT**: Not provided and not currently designed in the architecture.

| Capability Area | Classification | Direct Source Evidence | Detailed Findings & Limitations |
| :--- | :--- | :--- | :--- |
| **1. Boot and Initialization** | **IMPLEMENTED AND DEMONSTRATED** | `ternaryos/hal/qemu_armv8_efi_stub.c`, `qemu_slice6_bare_kernel.c`, `.github/workflows/qemu-boot.yml` | UEFI EDK2/OVMF PE32+ loader successfully boots in QEMU (cortex-a57) and VirtualBox, executes ExitBootServices, and establishes EL1/Ring 0 hardware access. |
| **2. CPU Privilege Levels** | **IMPLEMENTED AND DEMONSTRATED** | `ternaryos/hal/axion_el0_init.S`, `canon_exec_loader.cpp:210-280` | Kernel initializes in EL1 (supervisor), drops to EL0 (user) via `ERET` with `SPSR_EL1=0x3C0`, handles `SVC` synchronous exceptions, and restores EL1 state upon exit. |
| **3. Virtual Memory & Address Space Isolation** | **IMPLEMENTED AND DEMONSTRATED** | `ternaryos/hal/qemu_slice6_el0_mmu.cpp`, `tests/mmu_test.cpp` | Programs AArch64 TTBR0_EL1 translation tables (4 levels: L0-L3, 4KB granularity). Manages separate per-thread L3 tables for memory isolation across user tasks (RFC-00C6). |
| **4. Physical Memory Allocation** | **PARTIAL** | `ternaryos/mmu/ternary_page_alloc.cpp`, `tests/ternary_page_alloc_test.cpp` | Implements a ternary-granularity physical page allocator (`3^10`-tryte blocks, ~59,049 bytes). However, the freestanding C++ bridge uses static preallocated buffers rather than a dynamic heap allocator. |
| **5. Process & Thread Scheduling** | **IMPLEMENTED AND DEMONSTRATED** | `ternaryos/sched/scheduler.cpp`, `qemu_slice6_cpp_bridge.cpp:600-640` | Dual-mode scheduler: cooperative polling tick or preemption driven by ARM GICv3 generic timer PPI 30 interrupt ticks (~100Hz). Supports Ready, Running, Waiting, and Faulted thread states. |
| **6. Interrupts and Exceptions** | **IMPLEMENTED AND DEMONSTRATED** | `ternaryos/hal/aarch64_exception_vectors.S`, `qemu_slice6_bridge_irq.cpp` | Installs AArch64 vector table (`VBAR_EL1`). Configures ARM GICv3 Distributor/Redistributor, unmasks physical timer PPI 30, and catches Data Abort / Undefined Instruction traps. |
| **7. System Calls** | **IMPLEMENTED AND DEMONSTRATED** | `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp`, `kernelcall_abi.inc` | Implements wire-format `KernelCall` over `SVC #1` (RFC-00BD). Dispatches GetThreadIdentity, Yield, ExitThread, ClaimDevice, ReleaseDevice, SubmitEpoch, and QueryDevice. |
| **8. Device Drivers** | **PARTIAL** | `ternaryos/hal/qemu_slice6_cpp_bridge.cpp:200-350`, `dev/virtio_blk_mmio.cpp` | Implements minimal polling drivers for ARM PL011 UART, x86 COM1 16550A, ARM GICv3, and Virtio-blk (modern MMIO and legacy PCI). Does not support USB, NVMe, real network controllers, or sound. |
| **9. Persistent Filesystems** | **PARTIAL** | `ternaryos/dev/canon_store.cpp`, `hal/canon_exec_loader.cpp` | Interacts with raw block storage via Virtio-blk. Reads and writes CanonFS superblock (`CST1` magic) at LBA 0, and loads flat binaries from fixed sectors. Lacks hierarchical file trees, directories, or inode metadata. |
| **10. Interprocess Communication (IPC)** | **IMPLEMENTED AND DEMONSTRATED** | `ternaryos/ipc/canon_message.cpp`, `tests/ipc_test.cpp` | Implements synchronous and asynchronous canonical message passing between threads (`CanonMessage`). Verified in freestanding bridge via `canon_ipc_load_and_run()`. |
| **11. Networking** | **DESIGN ONLY / ABSENT** | `spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md` | Zero network protocol stack (IPv4, IPv6, TCP, UDP, ARP, or base-81) is implemented. Virtio-net driver is absent; `virtualbox_e1000_dev.cpp` is a mock fixture. |
| **12. Application Loading** | **PARTIAL** | `ternaryos/hal/canon_exec_loader.cpp` | Reads 512-byte raw `T81X` sector images from fixed LBAs into physical RAM. Lacks ELF/Mach-O/PE dynamic loaders, relocations, or symbols. Cannot load `.tisc` bytecode files. |
| **13. Executable Formats** | **PARTIAL** | `ternaryos/hal/canon_exec_loader.cpp:140-190` | Defines custom flat binary format `T81X` (magic, version, code_size, FNV-1a hash, entry offset, max 448 bytes code). Standard executable formats are unsupported. |
| **14. User Identity & Permissions** | **IMPLEMENTED AND DEMONSTRATED** | `kernel/axion/userenv/user_env.cpp`, `tests/user_env_test.cpp` | Session manager allocates monotonic session IDs, validates principal credentials, updates CanonFS audit records, and isolates shell environments across cognitive tiers. |
| **15. Fault Containment** | **IMPLEMENTED AND DEMONSTRATED** | `ternaryos/hal/aarch64_trap_entry.cpp`, `tests/hal_boot_test.cpp` | Hardware faults in EL0 (illegal memory access, undefined instruction) trap to EL1, terminate the faulting thread without crashing the kernel, and record diagnostic telemetry (RFC-00C7/RFC-00C8). |
| **16. System Recovery** | **PARTIAL** | `ternaryos/kernel/kernel_lifecycle.cpp`, `tests/epoch_syscall_test.cpp` | Supervisor threads can detect child thread faults and reinitialize failed tasks. However, full system-level crash dumps and persistent journal recovery are absent. |
| **17. Logging and Diagnostics** | **IMPLEMENTED AND DEMONSTRATED** | `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`, `drivers/qemu/sample-boot-log.txt` | Direct MMIO UART logging provides transparent boot traces, status reports, and execution metrics. Validated by automated regex log checkers in GitHub CI. |
| **18. Multi-Core Operation (SMP)** | **ABSENT** | `ternaryos/hal/qemu_slice6_cpp_bridge.cpp` | Single-core uniprocessor execution only. CPU cores beyond Core 0 remain parked in firmware spin-tables or PSCI dormancy. No SMP locking primitives (spinlocks, ticket locks) are implemented. |

---

## 2. Capability Audit Synthesis

1. **Microkernel Primitives are Sound:**
   Axion OS has successfully implemented the foundational primitives of an embedded microkernel: CPU privilege separation (EL1 vs EL0), page table virtualization (TTBR0), interrupt-driven scheduling (GICv3 PPI 30), and synchronous IPC.
2. **Specialized Computing Platform Scope:**
   The absence of conventional general-purpose facilities (dynamic linking, POSIX libc, socket networking, arbitrary user filesystem hierarchies) is consistent with a specialized high-assurance computational appliance.
3. **The Core Integration Gap:**
   The critical missing capability is **Application Loading & Runtime Hosting**. Because the microkernel cannot load and execute the T81VM or TISC bytecode, it cannot currently function as the native operating system for T81 applications.
