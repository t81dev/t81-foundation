# T81 Implementation Tracking – t81-spec.md Coverage

**Status legend**:
- [ ] Not started
- [x] Implemented & lightly tested
- [x+] Implemented + unit-tested / regression-covered
- [x++] Production-ready / benchmarked / fuzzed
- [?] Implemented but known issues / incomplete edge cases
- [!] Critical gap – blocks other work

## 1. Balanced Ternary Primitives & Arithmetic (section 2–3)
- [x++] Core trit encoding/decoding (packed 4-trit → base-81 byte) (see `include/t81/ternary.hpp`, `T81Int.hpp`)
- [x+] Limb54 add (Kogge-Stone style) (see `include/t81/core/T81Limb.hpp`)
- [x+] Limb54 mul Booth (full carry-save variant) (see `include/t81/core/T81Limb.hpp`, `booth_mul`)
- [x] Negation (free negation without borrow propagation) (see `operator-` in `T81Limb54`)
- [x+] Overflow / trap behavior (signed, unsigned modes) (see `axion::trap_overflow` in `T81Int.hpp`)
- [x] Division (Knuth Algorithm D) (see `T81BigInt::div_mod`)
- [ ] Modular inverse
  - [x] Basic extended Euclidean variant for T81BigInt
  - [ ] Constant-time / side-channel resistant if needed for crypto use cases

## 2. T81 Types & Cell Semantics (section 4)
- [x] T81Cell (48-trit scalar) (Implemented as `T81Limb` in `include/t81/core/T81Limb.hpp`)
- [x++] T81BigInt (arbitrary precision) (see `include/t81/core/T81BigInt.hpp`, supports Karatsuba)
- [x] T81Float / fixed-point variants (see `include/t81/core/T81Float.hpp`, `T81Fixed.hpp`)
- [x] T81Fraction (Exact rational value) (see `include/t81/core/T81Fraction.hpp`)
- [ ] Promotion / demotion rules
  - [x] T81Cell ↔ T81BigInt (scalar → multi-limb, sign-extended) (Implemented via constructor and to_int<N>)
  - [x] T81BigInt ↔ T81Float / T81Fixed (with rounding modes per spec) (Implemented via to_float/from_float)
  - [x+] Automatic promotion in expressions (T81 → T243 → T729 based on recursion depth)
  - [ ] Demotion with lossy checks / Axion traps on overflow/underflow

## 3. T81Lang Syntax & Semantics (section 5–7)
- [x] Lexer / parser basics (see `include/t81/lang/parser.hpp`)
- [x] Match expressions with canonical metadata
  - [x] Pattern matching on T81 values / structs (see `src/frontend/parser.cpp`, `MatchExpr`)
  - [x] Metadata attachment (via Policy MatchGuard, see `include/t81/axion/policy.hpp`)
- [x] Cognitive Tier Constructs (Syntax/Parsing)
  - [x] `Symbol` literals (`:foo`)
  - [x] `Infinite` literals (`∞{...}`)
  - [x] `recurse`, `distributed`, `infinite`, `reflect` blocks
- [ ] Recursion tiers & depth limits
  - [x+] Tiered promotion enforcement (T81 → T243 → T729)
  - [x] Configurable depth guards + Axion recursion traps (see `include/t81/axion/policy.hpp`, `src/vm/vm.cpp`)

## 4. HanoiVM / TISC Instruction Set (section 8–10)
- [x] Segmented memory model (see `src/vm/vm.cpp`)
- [x] Deterministic fault injection points (see `IVirtualMachine::set_fault_injections`)
  - [x] For testing Axion enforcement (overflow, illegal access, etc.)
- [x] Axion trap hooks (overflow, illegal access, recursion guard) (see `include/t81/axion/api.hpp`)
- [x] Bytecode format & decoder (see `src/vm/jit_compiler.cpp`)
- [x] JIT compilation (Research/Experimental)
- [x] Cognitive Tier Opcodes (Registered)
  - [ ] Full Logic Implementation (Symbolic, Reflective, etc.)

## 5. CanonFS & Tensor Storage (section 11)
- [x+] In-memory read/write throughput (see `src/canonfs/in_memory_driver.cpp`)
- [x] Persistent backend (see `src/canonfs/persistent_driver.cpp`)
- [x] Hash-addressed blocks & verification
  - [x] Block hashing (SHA3-256 verified in `TLoadHash`, see `src/vm/vm.cpp`)
  - [x] Integrity checks on read (implicit via content addressing and header check)
- [ ] .t81w / T3_K quantization import
  - [ ] Converter from GGUF / SafeTensors
  - [x] T3_K format parser & validation (see `promote_to_tensor` in `src/vm/vm.cpp`)

## 6. Axion Kernel & Safety Invariants (section 12)
- [x] Full enforcement in VM loop (VM hooks `axion::check`)
- [ ] GC hooks for tensor lifetimes
  - [ ] Reference counting / mark-sweep integration
  - [ ] Tensor-specific lifetime rules
- [ ] Ethics checks (Θ₁–Θ₉ implementation)
  - [ ] Θ enforcement points in VM / interpreter
  - [ ] Telemetry / logging for drift detection
  - [ ] Provable guarantees for the Nine Principles

## Cross-cutting / Later Phases
- [x] Full regression suite coverage (173/173 tests passing)
- [ ] Fuzzing / property-based testing for invariants
  - [ ] libFuzzer or similar on promotion, division, etc.
- [ ] Documentation sync (reference/benchmarks.md, notebooks)
  - [ ] Update benchmarks.md with latest numbers
  - [ ] Add spec cross-references in code comments (@spec-ref: 4.2)
- [ ] Python bindings completeness
  - [ ] Expose T81BigInt, VM, CanonFS
- [ ] GGUF / SafeTensors → ternary conversion tools
  - [ ] Basic importer script / CLI
