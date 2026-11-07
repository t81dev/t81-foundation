# T81 Canonical Architecture

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Canonical Architecture](#t81-canonical-architecture)
  - [1. Execution Pipeline](#1-execution-pipeline)
  - [2. Runtime Boundary](#2-runtime-boundary)
  - [3. Cognitive Tier Escalation](#3-cognitive-tier-escalation)
  - [4. Data Types and Representation Map](#4-data-types-and-representation-map)
  - [5. TISC ISA → VM Execution Micro-Flow](#5-tisc-isa-→-vm-execution-micro-flow)
  - [6. CanonFS + Weights + Tensor Ingress](#6-canonfs-+-weights-+-tensor-ingress)

<!-- T81-TOC:END -->


This document defines the authoritative architecture of the T81 system as implemented in the current codebase.

## 1. Execution Pipeline

The execution pipeline transforms source code into TISC bytecode, which is then executed by the T81 VM under strict Axion policy supervision.

```mermaid
flowchart LR
    Source[Source] --> Lexer[Lexer]
    Lexer --> Parser[Parser]
    Parser --> Semantic[Semantic Analyzer]
    Semantic --> IR[IR Generator]
    IR --> TISC[TISC Emitter]
    TISC --> VM[VM Interpreter]

    subgraph Runtime [core/vm]
        VM --> JIT{Hotspot?}
        JIT -- Yes --> Trace[ThreadedJitTrace]
        JIT -- No --> Exec[Execute Step]
        Trace --> Axion[Axion Policy]
        Exec --> Axion
    end

    subgraph Governance [kernel/axion]
        Axion --> Ethics{Ethics Check}
        Ethics -- Pass --> Commit[Commit State]
        Ethics -- Fail --> Trap[Trap / Halt]
    end
```

## 2. Runtime Boundary

The system strictly separates the deterministic core logic from the host-dependent surface and experimental features.

```mermaid
flowchart TD
    subgraph Core [Deterministic Core]
        Interpreter[Interpreter]
        JIT[ThreadedJitTrace]
        Axion[Axion Policy Engine]
        Mem[Memory Model]
        GC[Mark-and-Sweep GC]
    end

    subgraph Surface [Controlled Surface]
        CanonFS[CanonFS Driver]
        Weights[Weights Loader]
        Faults[Fault Injection]
    end

    subgraph Host [Host Boundary]
        Filesystem[std::filesystem]
        Alloc[std::vector/allocator]
    end

    Interpreter --> Mem
    Interpreter --> Axion
    Interpreter --> JIT
    Interpreter --> GC

    Interpreter --> CanonFS
    Interpreter --> Weights
    Interpreter --> Faults

    CanonFS --> Filesystem
    Weights --> Filesystem
    Mem --> Alloc
```

## 3. Cognitive Tier Escalation

The Cognitive Tier model defines the capabilities available to the runtime, escalating from basic symbolic manipulation to infinite series expansion, all supervised by Axion.

```mermaid
flowchart TD
    subgraph Tiers [Cognitive Tiers  experimental/tiers/cog ]
        T1[Tier 1: Symbolic]
        T2[Tier 2: Reflective]
        T3[Tier 3: Recursive]
        T4[Tier 4: Distributed  Stub ]
        T5[Tier 5: Infinite]
    end

    subgraph Supervision [Axion Supervision]
        Axion[Axion Policy Engine]
        Ethics[Ethics Principles]
        Limits[Recursion/Stack Limits]
        Promotion[Tier Promotion]
    end

    Interpreter[VM Interpreter] --> T1
    Interpreter --> T2
    Interpreter --> T3
    Interpreter --> T4
    Interpreter --> T5

    T1 --> Axion
    T2 --> Axion
    T3 --> Limits
    T3 --> Promotion
    T5 --> Axion

    Axion --> Ethics
    Promotion --> Axion
```

## 4. Data Types and Representation Map

The canonical numeric and symbolic types form a hierarchy where higher-level abstractions are built upon precise lower-level primitives, ensuring bit-exact determinism across the stack.

```mermaid
flowchart TD
    subgraph Primitives [Base Storage]
        Cell[T81Cell / T81Limb]
        Packed[Packed Trits (5 per Byte)]
    end

    subgraph Symbolic [Symbolic Algebra]
        BigInt[T81BigInt]
        Fraction[T81Fraction]

        Packed -- Decode --> BigInt
        Cell -- Construct --> BigInt
        BigInt -- "Num / Den" --> Fraction
        BigInt -- Canonicalize --> BigInt
        Fraction -- Canonicalize --> Fraction
    end

    subgraph Numeric [Numeric Computing]
        Float[T81Float + dmath]
        Native[Native Float (Optimization)]
        Tensor[Tensor (T729Tensor)]

        BigInt -- Mantissa --> Float
        Fraction -- Demote --> Float
        Float -- Element --> Tensor
        Native -- "Fast Path" --> Tensor
        Tensor -- "Matmul / Conv" --> Tensor
    end

    Tensor -- Serialize / Hash --> Packed
```

## 5. TISC ISA → VM Execution Micro-Flow

Each instruction execution step is strictly gated by the Axion Policy Engine, ensuring that every operation adheres to defined ethical and safety constraints before state mutation.

```mermaid
flowchart TD
    Start((Start Step)) --> Fetch[Fetch Opcode]
    Fetch --> Decode[Decode Operands]
    Decode --> AxionPre{Axion Policy Check}

    AxionPre -- Deny --> Trap[Trap / Halt]
    AxionPre -- Allow / Warn --> Execute[Execute Instruction]

    subgraph Execution [Execution Phase]
        Execute --> ALU[ALU Operation]
        Execute --> Mem[Memory Access]
        Execute --> TensorOp[Tensor Kernel]
        Execute --> CogOp[Cognitive Opcode]
    end

    Execution --> State[Update State]
    State --> Trace[Emit Axion Event / Trace]
    Trace --> Advance[Advance PC]
    Advance --> Start
```

## 6. CanonFS + Weights + Tensor Ingress

The model ingestion pipeline ensures that large tensors are securely imported, hashed, and verified before being loaded into the VM's managed tensor pool.

```mermaid
flowchart LR
    subgraph Ingestion [Ingestion Tools]
        Raw[GGUF / SafeTensors]
        Quant[t81 weights quantize]
        T3K[T3_K / .t81w]

        Raw --> Quant
        Quant --> T3K
    end

    subgraph Storage [CanonFS Storage]
        T3K --> Import[t81 weights import]
        Import --> Canon[CanonFS Blocks]
        Canon --> Hash[Content Addressable Hash]
    end

    subgraph Runtime [VM Runtime]
        Hash --> Load[TLoadHash Opcode]
        Load --> Verify[Verify Hash & Integrity]
        Verify --> Native[NativeTensor]
        Native --> Promote[Promote to T729Tensor]
        Promote --> Pool[VM Tensor Pool]
        Pool --> Handle[Tensor Handle]
    end
```
