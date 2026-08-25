# T81 Governed AI Operating System (DAIOS) Roadmap

This roadmap defines the engineering path for T81 as an Operating System for Governed, Deterministic AI Execution.
Architecture map: [RFC-00D2: DAIOS Target Architecture](../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md).

---

## Near-Term Milestones (0–6 Months): Core OS Unification & Bare-Metal Hardening

### Milestone 0.1: Unified Kernel & Process Execution Substrate (Layer 1 & 2)
* **Objective:** Merge freestanding `ternaryos/` kernel primitives into a unified `kernel/` hierarchy and standardize the T81 Process Model.
* **OS Deliverables:**
  - Consolidate `ternaryos/kernel` and `kernel/axion` into `kernel/core` and `kernel/governance`.
  - Implement TISC Task Process Control Block (PCB) supporting state saved context, capability masks, and memory region bounds.
  - Freeze Kernelcall ABI Ordinals (RFC-00BD) for process creation, memory allocation, and Axion policy validation.
  - Verification: `ctest --test-dir build -R kernel_process_suite` passing on GCC/Clang/MSVC and QEMU ARM64 target.

### Milestone 0.2: CanonFS as Immutable OS Root File System (Layer 3)
* **Objective:** Elevate CanonFS from a userland CLI tool into the canonical root file system (CanonVFS).
* **OS Deliverables:**
  - Implement read-only VFS driver for CanonFS objects, mounting executable bundles into process address spaces.
  - Establish content-addressed executable image verification: kernel verifies binary `CanonHash81` prior to page table mapping.
  - Harden RFC-00D1 foreign filesystem interchange with bit-exact import/export negative testing.

---

## Mid-Term Milestones (6–18 Months): System Services, GHAL & AI Runtime Integration

### Milestone 1.1: Governed Hardware Abstraction Layer (GHAL) & Driver Boundary (Layer 6)
* **Objective:** Establish formal driver and interrupt mediation interfaces under Axion policy.
* **OS Deliverables:**
  - Standardize GHAL device driver interfaces (`drivers/char`, `drivers/block`, `drivers/net`).
  - Implement hardware-interrupt-driven `WaitForDevice` and concurrent device wait filtering (RFC-00C2, RFC-00C5).
  - Integrate ternary-native hardware/accelerator interposer seams behind a deterministic GHAL backend API.

### Milestone 1.2: Base81 Governed IPC & Microkernel System Services (Layer 6)
* **Objective:** Replace ad-hoc FFI/host calls with deterministic, policy-gated Inter-Process Communication (IPC).
* **OS Deliverables:**
  - Implement Base81-aware IPC channel primitive for kernel-mediated message passing between EL0 userland services.
  - Build core system services: Governed Service Resolver (RFC-00D0), Audit Logging Daemon, and CanonFS Storage Daemon.
  - Enforce EL0 fault containment and supervisor fault recovery (RFC-00C7, RFC-00CD).

### Milestone 1.3: Governed AI Object Runtime & Model Paging (Layer 4 & 5)
* **Objective:** Expose ternary weight execution and model execution as OS-native task pipelines.
* **OS Deliverables:**
  - Implement bounded AI task process templates (`assess-fixed`, `route-fixed`, `classify-fixed`) running in isolated EL0 tasks.
  - Develop demand-paging mechanism for CanonFS weight tensors into T81VM address space with pre-side-effect policy gates.

---

## Longer-Term Milestones (18+ Months): Multi-Core DAIOS, Hardware Ecosystem & Release

### Milestone 2.1: Deterministic Multi-Core & Parallel Execution (DPE)
* **Objective:** Scale DAIOS execution across multi-core systems while preserving bit-exact reproducibility.
* **OS Deliverables:**
  - Implement Deterministic Parallel Execution (DPE) kernel scheduler (RFC-DPE-0001 through RFC-DPE-0009).
  - Enforce DAG-ordered epoch commitments across parallel worker threads with deterministic fault recovery.

### Milestone 2.2: DAIOS Standalone Bare-Metal Platform & Hardware Certification
* **Objective:** Deliver self-contained, bare-metal ISO/UEFI images for real hardware and emulated targets.
* **OS Deliverables:**
  - Provide complete UEFI/bootloader image booting directly into the Axion Governance Kernel and CanonVFS root.
  - Achieve formal Deterministic Core Profile (DCP) verification and release DAIOS v2.0-LTS.
