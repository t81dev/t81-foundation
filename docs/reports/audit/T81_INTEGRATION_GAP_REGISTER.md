# T81 Foundation — Prioritized Integration Gap Register

**Audit Designation:** T81-SIA-001
**Reference Document:** Comprehensive Register of Identified Architectural & Integration Gaps
**Objective:** Define the minimum viable engineering corrections and suggested acceptance tests to advance T81 toward an independent, fully integrated computing platform.

---

## 1. Summary of Identified Gaps

The audit identifies six primary integration gaps ranked by architectural criticality:

| Priority | Gap Identifier | Title & Description | Affected Environments | Complexity |
| :---: | :--- | :--- | :--- | :---: |
| **P0** | `GAP-001-TISC-IN-AXION` | Absence of TISC Virtual Machine in Freestanding Axion OS | Freestanding Axion OS | **HIGH** |
| **P1** | `GAP-002-DYNAMIC-CANONFS` | Hardcoded Sector Offsets vs Dynamic Content-Addressed CanonFS in Freestanding OS | Freestanding Axion OS | **MEDIUM** |
| **P2** | `GAP-003-FREESTANDING-POLICY` | Static Policy Tables vs Dynamic Axion Policy Evaluation in Freestanding OS | Freestanding Axion OS | **MEDIUM** |
| **P3** | `GAP-004-FREESTANDING-HEAP` | Lack of Freestanding Dynamic Memory Allocator for Complex Runtimes | Freestanding Axion OS | **MEDIUM** |
| **P4** | `GAP-005-PORTABLE-INTERCHANGE` | Merging Unmerged CanonFS Portability Repairs (PR #702 / #703) | Hosted Runtime | **LOW** |
| **P5** | `GAP-006-ROOT-GOVERNANCE` | Root Directory Structure Governance Regression on `origin/main` | Hosted Governance | **LOW** |

---

## 2. Detailed Gap Specifications

### GAP-001-TISC-IN-AXION: Embedding TISC VM into Freestanding Axion OS
- **Missing Capability:**
  The freestanding kernel (`BOOTAA64.EFI`) boots into an interactive shell but cannot execute `.tisc` binaries or T81Lang programs. It only executes raw AArch64 machine-code stubs (`el0_process_stub.S`).
- **Existing Implementation to Reuse:**
  The core TISC interpreter in `vm/vm.cpp`, `vm/decoder.cpp`, and `include/t81/vm/`.
- **Source Files & Interfaces Involved:**
  - `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`
  - `ternaryos/hal/canon_exec_loader.cpp`
  - `include/t81/vm/vm.hpp`
- **Dependency Prerequisites:**
  A minimal freestanding memory allocator (`GAP-004-FREESTANDING-HEAP`) or a static, pre-allocated VM state pool.
- **Minimum Viable Correction:**
  Extract a freestanding, no-STL subset of the TISC bytecode interpreter (`tisc_freestanding_evaluator.cpp`). Mount the interpreter inside the kernel or link it into an EL0 user binary that receives a TISC bytecode buffer from CanonFS and evaluates opcodes in user mode.
- **Suggested Acceptance Test:**
  Embed a small TISC binary (e.g. `accumulator.tisc`) into a CanonFS sector, invoke `run @accumulator` from the `t81>` shell prompt, and assert observable arithmetic output over the PL011 serial console.
- **Estimated Complexity:** **HIGH**
- **Blocks:** Freestanding operation.

---

### GAP-002-DYNAMIC-CANONFS: Dynamic Hash-Indexed CanonFS in Freestanding Kernel
- **Missing Capability:**
  The freestanding Virtio-blk driver loads binaries from hardcoded sector LBAs (LBA 3 for `proc-stub`, LBA 8 for `wait-test`). It lacks hash-indexed object lookup and cannot navigate CanonFS snapshot trees.
- **Existing Implementation to Reuse:**
  `include/t81/canonfs/storage_mapping.hpp` and `fs/persistent_driver.cpp`.
- **Source Files & Interfaces Involved:**
  - `ternaryos/dev/canon_store.cpp`
  - `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`
  - `fs/in_memory_driver.cpp`
- **Dependency Prerequisites:**
  Sha3-256 / Blake3 hashing implementation compiled in freestanding mode.
- **Minimum Viable Correction:**
  Implement a linear table scan of the CanonFS superblock entries (`CST1` at LBA 0) in `canon_store.cpp` matching 32-byte content hashes to sector LBAs, replacing hardcoded LBA tables.
- **Suggested Acceptance Test:**
  Command `cat sha3-256:<hash>` in `t81>` shell successfully resolves the LBA and prints payload bytes.
- **Estimated Complexity:** **MEDIUM**
- **Blocks:** Freestanding operation.

---

### GAP-003-FREESTANDING-POLICY: Dynamic Axion Policy Ingestion in Axion OS
- **Missing Capability:**
  Axion OS checks a static, hardcoded list of nine ethical principles ($\Theta_1$–$\Theta_9$) at boot. It cannot ingest, parse, or evaluate user-defined `.apl` or `.axionb` policy binary trees.
- **Existing Implementation to Reuse:**
  `kernel/axion/policy_serialization.cpp` and `kernel/axion/policy_engine.cpp`.
- **Source Files & Interfaces Involved:**
  - `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`
  - `kernel/axion/policy_validator.cpp`
- **Dependency Prerequisites:**
  Compact binary serialization format for compiled Axion policies (`.axionb`).
- **Minimum Viable Correction:**
  Load compiled binary policies (`.axionb`) from a designated CanonFS object hash into the freestanding kernel, evaluating them during `KernelCallKind` system call dispatch.
- **Suggested Acceptance Test:**
  Load an `.axionb` policy denying device arbitration; verify that `ClaimDevice` returns `CapabilityDenied` over the SVC wire interface.
- **Estimated Complexity:** **MEDIUM**
- **Blocks:** Freestanding operation.

---

### GAP-004-FREESTANDING-HEAP: Freestanding Dynamic Memory Allocator
- **Missing Capability:**
  The freestanding C++ bridge currently disallows all dynamic heap allocation (`-fno-builtin-malloc`, no global operator `new`), relying exclusively on fixed static arrays. This prevents porting the hosted T81 VM or parser directly.
- **Existing Implementation to Reuse:**
  `ternaryos/mmu/ternary_page_alloc.cpp` (already implements 59KB ternary block allocation).
- **Source Files & Interfaces Involved:**
  - `ternaryos/mmu/ternary_page_alloc.hpp`
  - `ternaryos/hal/qemu_slice6_freestanding_crt.c`
- **Dependency Prerequisites:**
  Controlled MMU page mapping in EL1 (`el0_mmu_init`).
- **Minimum Viable Correction:**
  Implement a small buddy allocator or slab allocator over physical memory pools managed by `TernaryPageAllocator`, exposing freestanding `malloc` and `free`.
- **Suggested Acceptance Test:**
  Freestanding unit test dynamically allocates and frees 1,000 variable-sized memory blocks without memory leaks or page boundary corruption.
- **Estimated Complexity:** **MEDIUM**
- **Blocks:** Freestanding operation.

---

### GAP-005-PORTABLE-INTERCHANGE: Merging CanonFS Portability Repairs
- **Missing Capability:**
  On Windows systems and non-POSIX platforms, snapshot paths and portable storage mappings in CanonFS require normalization of directory separators and invalid filename characters (`< > : " / \ | ? *`).
- **Existing Implementation to Reuse:**
  Active unmerged branches `fix/canonfs-portable-storage-mapping` (PR #702) and `fix/canonfs-portable-snapshot-paths` (PR #703).
- **Source Files & Interfaces Involved:**
  - `include/t81/canonfs/storage_mapping.hpp`
  - `tests/python/test_canonfs_cli_portable_storage.py`
  - `experiments/canonfs_portability/audit.py`
- **Dependency Prerequisites:** None.
- **Minimum Viable Correction:**
  Complete maintainer review and merge PR #702 and PR #703 into `origin/main`.
- **Suggested Acceptance Test:**
  `python3 tests/python/test_canonfs_cli_portable_storage.py` passes cleanly on both Linux and Windows runners.
- **Estimated Complexity:** **LOW**
- **Blocks:** Hosted cross-platform portability.

---

### GAP-006-ROOT-GOVERNANCE: Resolving Root Directory Structure Invariant
- **Missing Capability:**
  The repository governance checker (`scripts/governance/check_root_structure.py`) currently fails on `origin/main` because `review/` was merged into the root directory without updating `EXPECTED_TOP_LEVEL`.
- **Existing Implementation to Reuse:**
  `scripts/governance/check_root_structure.py`.
- **Source Files & Interfaces Involved:**
  - `scripts/governance/check_root_structure.py`
  - `review/pr-656/`
- **Dependency Prerequisites:** None.
- **Minimum Viable Correction:**
  Either relocate `review/` under `docs/records/audits/` or add `"review"` to `EXPECTED_TOP_LEVEL` in `check_root_structure.py`.
- **Suggested Acceptance Test:**
  `python3 scripts/governance/check_root_structure.py` reports `root structure check PASSED`.
- **Estimated Complexity:** **LOW**
- **Blocks:** CI pipeline hygiene.
