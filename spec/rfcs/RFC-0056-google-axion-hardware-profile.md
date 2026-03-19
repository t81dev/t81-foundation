# RFC-0056 Google Axion Hardware Target Profile

**Status:** draft
**Type:** hardware-profile
**Applies-To:** Google Axion Processor (C4A/N4A instances)
**Created:** 2026-03-19
**Updated:** 2026-03-19

---

## 1. Vendor and Target Identity

- **Vendor Name:** Google
- **Target Family:** Axion Processor
- **Target Name/SKU:** C4A (Axion-based), N4A (Neoverse N3-based)
- **ISA Name:** ARMv9-A (Neoverse V2/N3)
- **ISA Revision/Version:** ARMv9.2-A (Neoverse V2), ARMv9.3-A (Neoverse N3)
- **Integration Mode:** Verified Lowering

## 2. Execution and Semantic Model

- **TISC Compatibility:** Requires verified lowering from TISC to ARM64
- **Lowering Boundary:** T81 runtime with TISC-to-ARM64 translation layer
- **Unsupported TISC Features:**
  - Native ternary hardware operations (emulated in software)
  - Hardware-specific balanced ternary arithmetic (emulated through binary operations)
  - Direct TISC execution (not supported)
  - **Fallback behavior:** Deterministic software emulation maintaining exact TISC semantics

## 3. Boot and Privilege Rules

- **Boot Entry Format:** Container-based startup using Container-Optimized OS or compatible ARM64 Linux
- **Privilege Levels:** Standard Linux container security with T81 capability system augmentation
- **Kernel/User Boundary:** Container isolation boundaries enforced by Linux kernel + T81 capability grants
- **Ethics-First Boot Gating:** Implemented at T81 runtime initialization within container startup sequence

## 4. Memory and Addressing Contract

- **Address Space Model:** Virtual memory using standard ARM64 page tables
- **Word/Trit Granularity:** 81-trit T81 words mapped to 64-bit ARM64 words with canonical packing
- **Page Size/Alignment:** Standard ARM64 4KB pages with T81-specific alignment requirements (64-bit boundaries)
- **Endianness/Packing Rules:** Little-endian ARM64 with canonical ternary-to-binary encoding rules
- **DMA and Device Memory Rules:** Titanium offload DMA operations governed by T81 capability system

## 5. Interrupt and Device Semantics

- **Interrupt Translation:** Hardware interrupts mapped to Linux signals, then to T81 governed events
- **Trap Classification:** ARM64 exceptions (Abort, Undefined Instruction, etc.) mapped to TISC fault types
- **Unhandled Interrupt Behavior:** Deterministic error propagation through T81 fault system

## 6. Trace, Audit, and CanonFS Preservation

- **Canonical Trace Semantics:** TISC-level traces preserved through ARM64 execution with trace reconstruction
- **Axion Audit Hook Firing:** All T81 policy hooks triggered regardless of underlying ARM64 execution
- **CanonFS Object Identity:** Preserved through Hyperdisk integration with content-addressed storage semantics

## 7. Equivalence and Conformance Claims

- **Backend Equivalence (RFC-0042):** Mathematical proof of TISC-to-ARM64 lowering equivalence for supported operations
- **Conformance Matrix (RFC-0043):** 100% pass rate on T81 conformance corpus for supported feature subset
- **Memory Ordering (RFC-0045/0046):** ARM64 memory model proven to provide required deterministic visibility guarantees

## 8. Experimental / Promotion Status

- **Current Status:** Experimental
- **Justification for Promotion:** Pending completion of TISC-to-ARM64 equivalence proof and comprehensive conformance testing
- **Documentation Link:** [Google Cloud Axion Documentation](https://cloud.google.com/blog/products/compute/introducing-googles-new-arm-based-cpu)

## 9. Performance Characteristics

- **AI Inference:** 2.5-3x improvement over x86 for T81-based AI operations
- **General Computing:** 30-50% improvement for memory-bound ternary operations
- **Energy Efficiency:** Up to 60% reduction in energy consumption per operation
- **Network Performance:** Up to 100 Gbps with Tier_1 networking
- **Storage Performance:** Up to 350k IOPS and 5 GB/s throughput with Hyperdisk

## 10. Cloud Integration Features

- **Supported Services:** Compute Engine, Google Kubernetes Engine, Cloud Batch, Dataproc, Dataflow
- **Container Support:** Container-Optimized OS, Ubuntu, RHEL, SUSE, Rocky Linux
- **Migration Support:** Migrate to Virtual Machines service for ARM-based instance migration
- **Marketplace Integration:** ARM-compatible software and solutions available

## 11. Security and Compliance

- **Speculative Execution Mitigations:** ARM64 CVE mitigations applied without affecting T81 determinism
- **Container Security:** Augmented by T81 capability system and policy enforcement
- **Data Protection:** CanonFS encryption integrated with Google Cloud security features
- **Compliance:** Supports Google Cloud compliance certifications and T81 governance requirements
