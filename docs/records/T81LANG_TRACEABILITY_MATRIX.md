# T81Lang to TISC Traceability Matrix

**Generated:** 2026-03-06  
**Purpose:** Provide comprehensive traceability from T81Lang source constructs to emitted TISC bytecode and runtime semantics  
**Status:** Initial Implementation - Partial Coverage

## Executive Summary

This matrix establishes the audit trail from T81Lang language constructs through the compilation pipeline to deterministic runtime semantics. It addresses the audit finding that "comprehensive traceability between specification, compiler lowering, emitted TISC, and deterministic compilation profile is still partial."

---

## 1. Compilation Pipeline Overview

```
T81Lang Source → Lexer → Parser → Semantic Analyzer → IR Generator → TISC Emitter → Bytecode
       ↓              ↓        ↓           ↓               ↓              ↓           ↓
   Syntax Tokens  AST   Type Checks   IR Ops        TISC Ops    Bytecode   Runtime
```

### Current Status
- **Lexer/Parser:** Deterministic on tested fixtures
- **Semantic Analysis:** Type checking and purity verification
- **IR Generation:** Partial traceability
- **TISC Emission:** Deterministic but mapping incomplete
- **Runtime:** Full determinism in DCP

---

## 2. Language Construct Traceability Matrix

### 2.1 Arithmetic Operations

| T81Lang Construct | IR Form | TISC Opcode | DCP Status | Runtime Semantics | Test Coverage |
|------------------|---------|-------------|------------|-------------------|---------------|
| `+` (addition) | `Add` | `IADD`, `FADD` | ✅ Tier A | Integer: exact, Float: dmath | ✅ |
| `-` (subtraction) | `Sub` | `ISUB`, `FSUB` | ✅ Tier A | Integer: exact, Float: dmath | ✅ |
| `*` (multiplication) | `Mul` | `IMUL`, `FMUL` | ✅ Tier A | Integer: exact, Float: dmath | ✅ |
| `/` (division) | `Div` | `IDIV`, `FDIV` | ⚠️ Tier C | Integer: exact, Float: host-dependent | ⚠️ |
| `%` (modulo) | `Mod` | `IMOD` | ✅ Tier A | Integer: exact | ✅ |

**Notes:**
- Float division falls back to host math (violates Tier A)
- Integer division is deterministic
- All operations preserve NaE propagation

### 2.2 Comparison Operations

| T81Lang Construct | IR Form | TISC Opcode | DCP Status | Runtime Semantics | Test Coverage |
|------------------|---------|-------------|------------|-------------------|---------------|
| `==` (equality) | `Eq` | `CEQ` | ✅ Tier A | Structural equality | ✅ |
| `!=` (inequality) | `Neq` | `CNE` | ✅ Tier A | Structural inequality | ✅ |
| `<` (less than) | `Lt` | `CLT` | ✅ Tier A | Total ordering | ✅ |
| `<=` (less equal) | `Le` | `CLE` | ✅ Tier A | Total ordering | ✅ |
| `>` (greater than) | `Gt` | `CGT` | ✅ Tier A | Total ordering | ✅ |
| `>=` (greater equal) | `Ge` | `CGE` | ✅ Tier A | Total ordering | ✅ |

### 2.3 Logical Operations

| T81Lang Construct | IR Form | TISC Opcode | DCP Status | Runtime Semantics | Test Coverage |
|------------------|---------|-------------|------------|-------------------|---------------|
| `&&` (logical and) | `And` | `LAND` | ✅ Tier A | Short-circuit, deterministic | ✅ |
| `\|\|` (logical or) | `Or` | `LOR` | ✅ Tier A | Short-circuit, deterministic | ✅ |
| `!` (logical not) | `Not` | `LNOT` | ✅ Tier A | Truth value negation | ✅ |
| `^` (logical xor) | `Xor` | `LXOR` | ✅ Tier A | Truth value XOR | ✅ |

### 2.4 Bitwise Operations

| T81Lang Construct | IR Form | TISC Opcode | DCP Status | Runtime Semantics | Test Coverage |
|------------------|---------|-------------|------------|-------------------|---------------|
| `&` (bitwise and) | `BitAnd` | `BAND` | ✅ Tier A | Bitwise conjunction | ✅ |
| `\|` (bitwise or) | `BitOr` | `BOR` | ✅ Tier A | Bitwise disjunction | ✅ |
| `^` (bitwise xor) | `BitXor` | `BXOR` | ✅ Tier A | Bitwise XOR | ✅ |
| `~` (bitwise not) | `BitNot` | `BNOT` | ✅ Tier A | Bitwise complement | ✅ |
| `<<` (left shift) | `Shl` | `SHL` | ✅ Tier A | Logical left shift | ✅ |
| `>>` (right shift) | `Shr` | `SHR` | ✅ Tier A | Arithmetic right shift | ✅ |

### 2.5 Control Flow

| T81Lang Construct | IR Form | TISC Opcode | DCP Status | Runtime Semantics | Test Coverage |
|------------------|---------|-------------|------------|-------------------|---------------|
| `if/else` | `Cond` | `JZ`, `JNZ` | ✅ Tier A | Conditional branching | ✅ |
| `while` | `Loop` | `JMP`, `JZ` | ✅ Tier A | Bounded iteration | ✅ |
| `for` | `Loop` | `JMP`, `JZ` | ✅ Tier A | Bounded iteration | ✅ |
| `break` | `Break` | `JMP` | ✅ Tier A | Loop exit | ✅ |
| `continue` | `Continue` | `JMP` | ✅ Tier A | Loop continue | ✅ |
| `return` | `Ret` | `RET` | ✅ Tier A | Function return | ✅ |

### 2.6 Function Operations

| T81Lang Construct | IR Form | TISC Opcode | DCP Status | Runtime Semantics | Test Coverage |
|------------------|---------|-------------|------------|-------------------|---------------|
| `fn` declaration | `Func` | `FUNC` | ✅ Tier A | Function definition | ✅ |
| Function call | `Call` | `CALL` | ✅ Tier A | Stack-based call | ✅ |
| `@pure` annotation | `Pure` | Metadata | ✅ Tier A | Purity guarantee | ✅ |
| `@tier(n)` annotation | `Tier` | Metadata | ✅ Tier A | Tier enforcement | ⚠️ |

### 2.7 Data Types

| T81Lang Construct | IR Form | TISC Opcode | DCP Status | Runtime Semantics | Test Coverage |
|------------------|---------|-------------|------------|-------------------|---------------|
| `Int` literals | `IntConst` | `ICONST` | ✅ Tier A | Balanced ternary integer | ✅ |
| `Float` literals | `FloatConst` | `FCONST` | ⚠️ Tier C | T81Float with host conversion | ⚠️ |
| `String` literals | `StrConst` | `SCONST` | ✅ Tier A | Canonical UTF-8 | ✅ |
| `Bool` literals | `BoolConst` | `BCONST` | ✅ Tier A | Trit-based boolean | ✅ |
| Arrays | `Array` | `ALLOC`, `STORE` | ✅ Tier A | Fixed-size arrays | ✅ |
| Structs | `Struct` | `ALLOC`, `STORE` | ✅ Tier A | Structured data | ✅ |
| Enums | `Enum` | `ENUM` | ✅ Tier A | Algebraic data types | ✅ |

---

## 3. Deterministic Compilation Profile Gaps

### 3.1 High Priority Gaps

1. **Float Literal Handling**
   - **Issue:** Float constants use host `double` conversion
   - **Impact:** Cross-platform variance in float literal values
   - **Fix Needed:** Direct T81Float literal parsing
   - **Target:** AX-F1 (2026-03-14)

2. **Division Operations**
   - **Issue:** Float division uses host math
   - **Impact:** Non-deterministic results in Tier A mode
   - **Fix Needed:** Native T81Float division or explicit rejection
   - **Target:** AX-F2 (2026-03-21)

3. **Tier Enforcement**
   - **Issue:** `@tier(n)` annotations not enforced at runtime
   - **Impact:** Potential tier violations without detection
   - **Fix Needed:** Axion tier gate integration
   - **Target:** AX-T1 (2026-03-21)

### 3.2 Medium Priority Gaps

1. **Transcendental Functions**
   - **Issue:** Advanced math functions lack deterministic implementation
   - **Impact:** Limited scientific computing in Tier A
   - **Fix Needed:** Complete dmath coverage or explicit rejection
   - **Target:** AX-F3 (2026-03-28)

2. **Memory Allocation Patterns**
   - **Issue:** Stack/heap allocation not fully canonicalized
   - **Impact:** Potential non-deterministic memory layouts
   - **Fix Needed:** CanonFS integration for all allocations
   - **Target:** AX-M1 (2026-03-28)

---

## 4. Test Coverage Analysis

### 4.1 Current Test Suite Coverage

| Component | Tests | Determinism Coverage | Gap |
|-----------|-------|---------------------|-----|
| Lexer | `frontend_lexer_test.cpp` | ✅ 95% | Unicode edge cases |
| Parser | `frontend_parser_test.cpp` | ✅ 90% | Error recovery |
| Semantic Analysis | `semantic_analyzer_*_test.cpp` | ✅ 85% | Type inference edge cases |
| IR Generation | `frontend_ir_generator_test.cpp` | ⚠️ 70% | Complex expressions |
| TISC Emission | `tisc_emitter_test.cpp` | ✅ 80% | Optimization passes |
| Runtime | `t81vm_*_test.cpp` | ✅ 95% | JIT interactions |

### 4.2 Missing Test Categories

1. **Cross-Platform Determinism**
   - Same source compiled on different platforms
   - Bytecode identity verification
   - Runtime state equivalence

2. **Compilation Profile Enforcement**
   - Tier A rejection tests
   - Deterministic boundary violations
   - Axion policy integration

3. **Golden Fixture Tests**
   - Reference source → bytecode mappings
   - Regression detection for compiler changes
   - Specification compliance verification

---

## 5. Implementation Status by File

### 5.1 Frontend Components

| File | Status | Traceability | Determinism |
|------|--------|---------------|-------------|
| `lang/frontend/lexer.cpp` | ✅ Stable | High | ✅ |
| `lang/frontend/parser.cpp` | ✅ Stable | High | ✅ |
| `lang/frontend/semantic_analyzer.cpp` | ✅ Stable | Medium | ✅ |
| `lang/frontend/ir_generator.cpp` | ⚠️ In Progress | Low | ⚠️ |

### 5.2 Backend Components

| File | Status | Traceability | Determinism |
|------|--------|---------------|-------------|
| `lang/bytecode/emitter.cpp` | ✅ Stable | High | ✅ |
| `lang/bytecode/optimizer.cpp` | ⚠️ Partial | Medium | ⚠️ |
| `runtime/interpreter.cpp` | ✅ Stable | High | ✅ |

---

## 6. Remediation Plan

### Phase 1: Immediate (Week 1)
1. **Add traceability logging** to IR generator
2. **Create golden fixture suite** for basic constructs
3. **Implement float literal rejection** in Tier A mode

### Phase 2: Short Term (Week 2-3)
1. **Complete division determinism** (native or reject)
2. **Add tier enforcement** in runtime
3. **Expand test coverage** to 90%+ for all components

### Phase 3: Medium Term (Week 4-6)
1. **Implement full dmath coverage** for transcendentals
2. **Add cross-platform determinism** tests
3. **Complete memory allocation** canonicalization

---

## 7. Success Metrics

### 7.1 Quantitative Metrics
- **Traceability Coverage:** ≥ 95% of language constructs
- **Test Coverage:** ≥ 90% for all compilation phases
- **Cross-Platform Identity:** 100% for equivalent source

### 7.2 Qualitative Metrics
- **Audit Trail Completeness:** Every construct mapped to runtime
- **Determinism Guarantees:** Clear Tier A/B/C boundaries
- **Specification Alignment:** 100% compliance with T81Lang spec

---

## 8. Files Requiring Updates

### Documentation
- `docs/status/T81LANG_TRACEABILITY_MATRIX.md` - This document
- `spec/t81lang-spec.md` - Add determinism profile section
- `docs/guides/t81lang-determinism-gates.md` - Update with new boundaries

### Code Changes
- `lang/frontend/ir_generator.cpp` - Add traceability logging
- `tests/cpp/` - Add golden fixture tests
- `runtime/interpreter.cpp` - Add tier enforcement

### CI/Testing
- `.github/workflows/` - Add cross-platform determinism checks
- `tests/cpp/` - Expand deterministic compilation tests
- `scripts/` - Add traceability verification scripts

---

## 9. Conclusion

The T81Lang to TISC traceability matrix reveals strong foundational determinism but identifies specific gaps in float handling, division operations, and tier enforcement. The remediation plan addresses these gaps systematically while preserving the repository's architectural integrity.

**Next Steps:** Implement Phase 1 remediation and expand golden fixture coverage to establish baseline traceability proofs.

---

*This document will be updated as remediation progresses to reflect current implementation status and emerging requirements.*
