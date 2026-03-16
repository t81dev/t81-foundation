# RFC-00B8: Governed Foreign Function Interface

**Status:** proposed
**Type:** standards-track
**Applies-To:** T81VM runtime, T81Lang compiler, Axion governance kernel
**Created:** 2026-03-16
**Author:** @t81dev
**Depends on:** RFC-00B5 (Governed Event Interrupt Model), RFC-0015 (AgentInvoke), RFC-0002 (Deterministic Execution Contract)
**Blocks:** Production FFI integration without governance controls

---

## 1. Summary

This RFC defines a governed Foreign Function Interface (FFI) for T81 that enables safe interoperability with external code while preserving deterministic execution guarantees and governance oversight. The FFI system treats external function calls as governed events subject to Axion policy enforcement, audit logging, and provenance tracking.

## 2. Motivation

T81 needs to integrate with existing ecosystems (filesystems, hardware accelerators, cryptographic libraries, system services) while maintaining its core value proposition of deterministic, auditable computation. Traditional FFI designs create uncontrolled escape hatches that bypass governance and determinism guarantees.

Current gaps:
- No mechanism to call existing C libraries from T81Lang
- No integration path for GPU acceleration in governed inference mode
- Limited ability to interface with OS-level services
- Missing bridge to established mathematical and scientific computing ecosystems

## 3. Proposal

### 3.1 Goals

- Enable safe external function calls with governance oversight
- Preserve deterministic execution guarantees where possible
- Provide clear provenance for all external interactions
- Enable performance-critical integrations (GPU, specialized hardware)
- Maintain auditability of all FFI operations

### 3.2 Non-Goals

- Direct memory access to external processes
- Unrestricted system calls without policy mediation
- Breaking determinism guarantees without explicit labeling
- Complex ABI compatibility with legacy systems

### 3.3 Architecture Overview

```
T81Lang Source Code
        ↓
   T81Lang Compiler (FFI-aware)
        ↓
   TISC Bytecode + FFI Metadata
        ↓
    T81VM Runtime
        ↓
   Axion Policy Engine ← FFI Call Request
        ↓
    Governed FFI Dispatcher
        ↓
   External Function (C library, GPU, etc.)
        ↓
    FFI Result + Audit Trail
        ↓
    T81VM Continuation
```

### 3.4 FFI Function Classification

#### 3.4.1 Deterministic FFI Functions
- **Pure Functions**: No external state, same input → same output
- **Stateless Libraries**: Mathematical operations, cryptographic primitives
- **Deterministic System Calls**: Clock functions, deterministic I/O
- **Marked with**: `@deterministic` annotation in T81Lang

#### 3.4.2 Governed FFI Functions  
- **Stateful Operations**: File I/O, network operations
- **Hardware Acceleration**: GPU kernels, specialized processors
- **System Services**: Timer management, process control
- **Marked with**: `@governed` annotation in T81Lang

#### 3.4.3 Quarantined FFI Functions
- **High-Risk Operations**: Direct memory access, privileged system calls
- **Network Operations**: Raw socket access, uncontrolled network I/O
- **Marked with**: `@quarantined` annotation in T81Lang

### 3.5 T81Lang Language Extensions

#### 3.5.1 FFI Declaration Syntax
```t81
// Deterministic mathematical library
foreign deterministic {
    fn sin(x: Float) -> Float;
    fn cos(x: Float) -> Float;
    fn exp(x: Float) -> Float;
}

// Governed file operations
foreign governed {
    fn open(path: String, mode: Int) -> FileHandle;
    fn read(handle: FileHandle, buffer: Bytes, count: Int) -> Int;
    fn write(handle: FileHandle, buffer: Bytes, count: Int) -> Int;
    fn close(handle: FileHandle) -> Int;
}

// Quarantined system operations
foreign quarantined {
    fn raw_socket(domain: Int, type: Int, protocol: Int) -> Socket;
    fn mmap(addr: Pointer, length: Int, prot: Int, flags: Int) -> Pointer;
}
```

#### 3.5.2 FFI Call Syntax
```t81
// Direct call with governance
let result = foreign.sin(input_value);

// Explicit policy check
let handle = foreign.open("data.txt", READ_MODE) if policy.allow_file_read();

// Quarantined call requires explicit override
let socket = foreign.raw_socket(AF_INET, SOCK_STREAM, 0) 
    override policy.quarantine_reason("GPU acceleration required");
```

### 3.6 Runtime Implementation

#### 3.6.1 FFI Call Flow
1. **Call Request**: T81VM encounters FFI instruction
2. **Policy Check**: Axion evaluates call against current policy
3. **Audit Logging**: Call attempt logged with provenance
4. **Dispatch**: Governed FFI dispatcher executes call
5. **Result Processing**: Result and side effects audited
6. **Continuation**: VM resumes with deterministic or governed result

#### 3.6.2 Governance Integration Points

**Pre-Call Policy Checks:**
- Function permission validation
- Argument type safety verification
- Resource quota enforcement
- Quarantine status verification

**Post-Call Auditing:**
- Function execution time
- Resource consumption metrics
- Side effect detection
- Error condition handling

**Provenance Tracking:**
- External library version and hash
- Call site location in T81Lang source
- Policy decision and reasoning
- Result hash and state changes

### 3.7 Determinism Preservation

#### 3.7.1 Deterministic FFI Mode
- External functions must be pure and stateless
- Same inputs always produce same outputs
- No hidden dependencies on system state
- Results included in deterministic trace hash

#### 3.7.2 Governed FFI Mode
- Side effects allowed but audited
- Non-deterministic results flagged
- Execution time bounded by policy
- Results excluded from deterministic trace hash

#### 3.7.3 Quarantine Mode
- Calls blocked by default
- Requires explicit policy override
- Full audit trail required
- Results marked as non-reproducible

## 4. Implementation Plan

### 4.1 Phase 1: Core FFI Infrastructure (2026-Q2)
- FFI call instruction in TISC ISA
- Basic FFI dispatcher in T81VM
- T81Lang compiler FFI syntax support
- Axion policy integration for FFI calls

### 4.2 Phase 2: Governance Integration (2026-Q3)
- Policy engine extensions for FFI governance
- Audit trail generation for FFI operations
- Provenance tracking for external libraries
- Quarantine mode implementation

### 4.3 Phase 3: Ecosystem Integration (2026-Q4)
- Standard library FFI bindings (libc, POSIX)
- Hardware acceleration FFI (CUDA, OpenCL)
- Cryptographic library FFI (OpenSSL, libsodium)
- Performance optimization and caching

## 5. Determinism / Safety Considerations

### 5.1 Determinism Guarantees
- Deterministic FFI calls preserve bit-exact reproducibility
- Governed FFI calls maintain audit-level determinism
- Quarantined calls explicitly break determinism
- Clear labeling prevents accidental determinism violations

### 5.2 Safety Mechanisms
- Type safety enforced at compile time and runtime
- Memory boundary validation for all FFI calls
- Resource quota enforcement prevents resource exhaustion
- Policy-based access control for privileged operations

### 5.3 Security Considerations
- FFI functions must be explicitly declared and registered
- Dynamic loading restricted to vetted libraries
- Sandboxing for quarantined operations
- Audit trail provides forensic capability

## 6. Compatibility

This RFC extends but does not break existing T81 contracts:
- TISC ISA gains new FFI instructions
- T81VM gains FFI dispatch capability
- T81Lang gains FFI syntax
- Axion gains FFI policy governance
- Deterministic execution preserved for pure FFI calls

## 7. Open Questions

1. **Performance Overhead**: What is the governance cost for FFI calls?
2. **Library Management**: How to version and validate external libraries?
3. **Cross-Platform**: Handle platform-specific FFI differences?
4. **Error Propagation**: Best practices for FFI error handling?
5. **Resource Limits**: Appropriate default quotas for FFI operations?

## 8. Acceptance Criteria

This RFC can move from `proposed` to `accepted` when:

- FFI instruction set is defined and implemented in TISC ISA
- T81VM FFI dispatcher with governance integration exists
- T81Lang compiler supports FFI syntax and annotations
- Axion policy engine can govern FFI calls
- Test suite demonstrates deterministic and governed FFI modes
- Security audit validates FFI sandboxing and type safety

---

*FFI provides essential interoperability while maintaining T81's core values of deterministic, auditable computation through governed integration rather than uncontrolled escape hatches.*
