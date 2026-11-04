# T81 Foundation — Application Lifecycle Matrix

**Audit Designation:** T81-SIA-001
**Reference Document:** Application Lifecycle Analysis (Hosted vs. Freestanding Axion OS)
**Evaluated Application Workflows:** Compilation, Bytecode Emission, Binary Packaging, Model Ingestion, Policy Evaluation, Execution, Persistent Side Effects, Verification, and Failure Recovery.

---

## 1. Application Lifecycle Comparative Matrix

| Stage in Application Lifecycle | Hosted Runtime (Linux, macOS, Windows) | Freestanding Axion OS (Bare-Metal / QEMU) | Integration & Architectural Disconnect |
| :--- | :--- | :--- | :--- |
| **1. Source Program Authoring** | Supported: High-level T81Lang source files (`.t81`) with ternary types, pattern matching, cognitive tiers, and tensor operations. | Not Supported: No on-target text editors, compilers, or source-level interpreters exist in the image. | Application authoring must occur on a conventional host machine. |
| **2. Compilation & Lowering** | Fully Supported: `t81 lang build file.t81 -o file.tisc` parses AST, validates types, and lowers to TISC bytecode. | Not Supported: The compiler frontend (`src/lang/`) depends on standard C++ STL and is omitted from `BOOTAA64.EFI`. | T81Lang programs cannot be built within Axion OS. |
| **3. Artifact Packaging** | Fully Supported: TISC binaries (`.tisc`) with 4-byte magic `TISC`, 16-byte version headers, symbol tables, and opcode sections. | Diverged Format: Freestanding loader expects `T81X` raw binary images (magic `T81X`, 64-byte header, max 448-byte native code). | TISC artifacts cannot be executed by the freestanding loader; `T81X` is native binary machine code. |
| **4. Storage Ingestion & Identity** | Fully Supported: `t81 canonfs put-file` hashes file with SHA3-256, assigns canonical ID, writes immutable content object. | Partial / Emulated: Virtio-blk driver accesses raw sector LBAs; superblock verifies `CST1` magic, but no hash-based directory lookup. | Freestanding environment accesses storage by hardcoded sector offsets (LBA 3, LBA 4, etc.) rather than content hashes. |
| **5. Policy Pre-Flight Evaluation** | Fully Supported: Axion policy compiler evaluates `.apl` rules; intercepts commands; enforces fail-closed execution barriers. | Simulated / Statically Initialized: `cmd_policy` prints static policy readiness; ethics gate evaluates hardcoded $\Theta_1$–$\Theta_9$ rules. | Freestanding policy engine does not evaluate external `.axionb` policy bundles dynamically. |
| **6. Execution Engine** | Fully Supported: `t81 vm run file.tisc` spins up `t81_vm` interpreter, evaluates TISC opcodes, maintains 243-register bank. | Disconnected: Axion OS kernel executes native AArch64 machine instructions via `ERET` to EL0; no TISC VM exists. | The fundamental computational runtime of T81 (TISC/T81VM) does not run inside Axion OS. |
| **7. System Call Interface** | Standard Host OS: Host filesystem, memory allocation (`mmap`), threading (pthreads/std::thread), process isolation. | Freestanding Wire ABI: Native CPU `SVC` instructions (`SVC #0`–`#3`) dispatched through `axion_svc_handler_aarch64`. | Completely incompatible ABI between hosted CLI tools and freestanding microkernel. |
| **8. Persistent Output** | Fully Supported: CanonFS export, snapshots (`snapshot.json`), manifest chaining, rollback to historical states. | Minimal: Virtio-blk sector writes to LBA 1; transcript logging over serial console. | Freestanding system lacks portable snapshot creation and content-addressed recovery trees. |
| **9. Post-Execution Audit & Hash Trace** | Fully Supported: Traces exported via `t81 trace export`; state hash checked against deterministic profile registry. | Limited: PL011 UART string logging captured by host test harness (`qemu-boot.yml`). | Verification in freestanding mode depends on host-side log scraping rather than self-verifying digital artifacts. |
| **10. Fault Containment & Recovery** | Handled via C++ exceptions, process exit codes, and Axion policy event aborts. | Implemented for Native EL0: Hardware traps (Data Abort, Undefined Instruction) caught in EL1; thread terminated cleanly. | EL0 fault isolation works for native machine instructions, but cannot isolate TISC bytecode faults on target. |

---

## 2. End-to-End Application Trace: Hosted Runtime

The hosted T81 application lifecycle represents a fully working, highly cohesive, spec-compliant pipeline.

### Step 1: Program Definition
An application program is authored in T81Lang (`examples/core-language/hello_world.t81`):
```t81
fn main() -> i64 {
    println("Hello, World!");
    return 0;
}
```

### Step 2: Compilation to TISC
Executed via CLI:
```bash
./build/t81 lang build examples/core-language/hello_world.t81 -o hello.tisc
```
- Frontend lexer, recursive-descent parser, and type-checker process the source.
- AST is lowered into intermediate representation (IR) and serialized into frozen TISC ISA v1.9.0 bytecode.

### Step 3: Canonical Storage Registration
Executed via CanonFS CLI:
```bash
./build/t81 canonfs put-file hello.tisc --canonfs-root /tmp/canonfs_store
```
- Computes canonical SHA3-256 digest (`sha3-256:2sw>ymQ4≈>Y9qpwjwσ≥ONPeτωμoK<Yt>hg3ZBS2Zl`).
- Stores immutable blob under `.canonfs/objects/` with strict mode and permission normalization.

### Step 4: Policy-Governed Execution
Executed via VM runtime:
```bash
./build/t81 vm run hello.tisc
```
- Program bytes are loaded into `T81VM`.
- Axion runtime observes opcode dispatch; records execution events (10 Axion events observed).
- Outputs:
  ```text
  Hello, World!
  Program terminated normally (10 axion event(s))
  ```

### Step 5: Policy Denial Verification
When evaluated against an unauthorized policy profile (e.g. `deny-all`):
```bash
./build/t81 canonfs import hello.tisc --policy-profile deny-all --json
```
- Engine evaluates policy rules before write side effects.
- Fails closed immediately:
  ```json
  {
    "status": "error",
    "errors": [
      {"kind": "policy-failure", "message": "policy denied import of hello.tisc: policy-profile deny-all denies canonfs.import before CanonFS storage writes"}
    ],
    "policy_result": "denied"
  }
  ```

---

## 3. End-to-End Application Trace: Freestanding Axion OS

The freestanding application lifecycle demonstrates low-level hardware orchestration but diverges completely from T81Lang/TISC:

### Step 1: Program Authoring in Native Assembly
Because no compiler runs in Axion OS and TISC is not supported, user code must be authored in native AArch64 assembly (`ternaryos/hal/el0_process_stub.S`):
```assembly
    .include "kernelcall_abi.inc"
    .section .text, "ax"
    .global proc_entry
proc_entry:
    sub sp, sp, #64
    // Issue SVC #1 to retrieve thread ID
    mov x0, #1
    svc #1
    // Issue SVC #3 to write debug string to serial
    adr x0, .Lmsg
    svc #3
    // Issue SVC #2 to exit thread
    mov x0, #0
    svc #2
.Lmsg:
    .asciz "[axion] el0: process loaded from CanonFS (tid=2)\r\n"
```

### Step 2: Cross-Compilation & Flat Binary Extraction
Compiled on host with Clang:
```bash
clang-18 --target=aarch64-unknown-elf -ffreestanding -nostdlib -c el0_process_stub.S -o stub.o
clang-18 --target=aarch64-unknown-elf -ffreestanding -nostdlib -Wl,-Ttext=0x0 -Wl,--omagic stub.o -o stub.elf
llvm-objcopy-18 -O binary --only-section=.text stub.elf stub.bin
```

### Step 3: T81X Sector Packaging & Disk Injection
Wrapped in a 64-byte `T81X` header and written to raw sector offset 3 (byte 1536) of `canon_store.img`:
```python
# T81X Header: Magic (4B) + Version (1B) + Entry Offset (4B) + Code Size (4B)
header = b'T81X' + bytes([1, 0, 0, 0, 0, 0, 0]) + struct.pack('<I', len(code)) + bytes(48)
sector = (header + code).ljust(512, b'\x00')
disk.seek(1536)
disk.write(sector)
```

### Step 4: Boot & Dispatch in QEMU
1. QEMU boots `BOOTAA64.EFI`.
2. Firmware transitions to `qemu_bare_kernel_entry()`.
3. `canon_exec_load_and_run()` reads LBA 3 from virtio-blk MMIO transport.
4. Header magic `T81X` is verified.
5. Code bytes are copied into EL0 physical code page; instruction cache is invalidated (`ic iallu`).
6. Kernel executes `ERET` to EL0.
7. Stub executes on bare CPU, invokes `SVC #3` (emits serial message), and exits via `SVC #2`.
8. Kernel catches `SVC #2` in EL1 exception vector and resumes kernel shell.

---

## 4. Lifecycle Assessment Summary

1. **Hosted Workflow Completeness:** 100% operational, self-contained, and integrated across all five principal systems.
2. **Freestanding Workflow Completeness:** 25% operational. While kernel boot, MMU paging, virtio-blk sector I/O, and EL0 context switching function as a microkernel, **zero percent** of the T81 computational layer (T81Lang/TISC/T81VM/Axion policy parser) currently executes in the freestanding environment.
