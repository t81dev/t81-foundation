# T81 Foundation Architecture — Comprehensive Capability Baseline

## Executive Overview

This document establishes the official capability baseline for the T81 Foundation architecture. Every capability claim in this inventory is grounded in exact source file paths, public C++ APIs, TISC opcodes, T81Lang constructs, test executables, and empirical evidence recorded directly from the codebase at commit `c930b8c2ee7478782bf3c6d231bdd59984595ef2`.

### Evidence Level Definitions
- **SPECIFIED**: A normative behavior is defined in an accepted RFC or spec document under `/spec/`.
- **IMPLEMENTED**: An identifiable code path realizes the behavior in C++, Python, or T81Lang source.
- **TESTED**: Automated unit, E2E, or spec conformance tests exercise the behavior in CTest.
- **MEASURED**: Reproducible quantitative performance or benchmark data exists in `/benchmarks/` or evidence logs.
- **COMPARATIVELY VALIDATED**: Equivalent optimized controls establish a bounded comparative result.
- **OPEN**: Specification or implementation is incomplete, deferred, or unverified on target platforms.
- **CONTRADICTED**: Empirical evidence conflicts with a previously recorded prose claim.

---

## 1. Balanced-Ternary Arithmetic & Numerical Representations

### 1.1 Native Trit Integer & SWAR Representation (`t81::T81`, `ExactTrit`)
- **Applicable RFCs**: RFC-0001, RFC-0017, RFC-0018, RFC-0040, RFC-0044, RFC-0049.
- **Source Paths**: `/include/t81/simd/simd.hpp`, `/include/t81/simd/swar.hpp`, `/include/t81/simd/packed_trit_vector.hpp`.
- **Public APIs / Opcodes**: `t81::T81` struct, `t81::simd::swar_not()`, `t81::simd::swar_and()`, `t81::simd::swar_or()`, opcodes `TNOT_SWAR` (`0xD5`), `TAND_SWAR` (`0xD6`), `TOR_SWAR` (`0xD7`).
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`, `MEASURED`, `COMPARATIVELY VALIDATED`.
- **Status & Supported Scope**: 128-trit packed register struct using 2-bits-per-trit encoding (`00`=0, `01`=+1, `10`=-1, `11`=unused). AVX2 vector addition uses carry-map lookup and prefix scan.
- **Tests & Oracles**: `t81_native_property_test`, `t81_simd_add_helpers_test`, `test_swar`.
- **Measured Results**: AVX2 128-trit SIMD addition achieves ~1.2–2.5 Gtrits/sec on x86_64; SWAR kernels show 62x–67x speedups over scalar trit interpreter loops.
- **Limitations & Open Questions**: Native hardware execution relies on host x86_64 AVX2 / ARM64 NEON emulation. Scalar fallback is required on non-AVX2 hosts.

### 1.2 Deterministic Floating-Point Subsystem (`T81Float`, `soft_math`)
- **Applicable RFCs**: RFC-0010, RFC-0030, RFC-00D5.
- **Source Paths**: `/include/t81/math/soft_math.hpp`, `/vm/value_ops.cpp`, `/experiments/checked_float_contract/`.
- **Public APIs / Opcodes**: `t81::math::soft_sin()`, `soft_cos()`, `soft_exp()`, `soft_log()`, opcodes `FAdd` (`0x24`), `FSub` (`0x25`), `FMul` (`0x26`), `FDiv` (`0x27`), `FSin` (`0x51`) .. `FPow` (`0x5D`).
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`, `MEASURED`, `COMPARATIVELY VALIDATED`.
- **Status & Supported Scope**: IEEE 754 exactly-rounded `FDiv`; integer-backed bit-exact transcendental functions (`soft_math`) eliminate host `libm` drift across x86_64/ARM64. Checked float conversion contract prototype validated in Scientific Investigation 02/04.
- **Tests & Oracles**: `t81_vm_float_fraction_ops_test`, `test_t81float_soft_math`, `test_deterministic_float_boundaries`, `python3 experiments/checked_float_contract/test_reference.py`.
- **Limitations & Open Questions**: Scientific Investigation 01 bounded float conversion discrepancies; exact rational conversion prototype (`EXACT_RATIONAL_CONVERSION_FEASIBILITY.md`) was dispositioned REVISE due to specification ambiguity.

### 1.3 Exact Fraction & BigInt Arithmetic (`T81Fraction`, `T81BigInt`)
- **Applicable RFCs**: RFC-0001, RFC-0007, RFC-0010.
- **Source Paths**: `/include/t81/math/fraction.hpp`, `/include/t81/math/bigint.hpp`, `/vm/value_ops.cpp`.
- **Public APIs / Opcodes**: `t81::math::Fraction`, `t81::math::BigInt`, opcodes `FracAdd` (`0x28`) .. `FracDiv` (`0x2B`), `I2Frac` (`0x1F`), `Frac2I` (`0x20`), `Int2BigInt` (`0xC1`).
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: Arbitrary-precision integer (`T81BigInt`) and exact rational fraction (`T81Fraction`) arithmetic with GCD reduction.
- **Tests & Oracles**: `t81_determinism_math_test`, `e2e_arithmetic_test`.

---

## 2. TISC Architecture, Opcodes & Execution Model

### 2.1 Instruction Encoding & Machine State
- **Applicable RFCs**: RFC-0000, RFC-0002, RFC-0005, RFC-0026, RFC-0034, RFC-0049.
- **Source Paths**: `/include/t81/isa/opcodes.hpp`, `/isa/encoding.cpp`, `/isa/binary_emitter.cpp`, `/vm/decoder.cpp`.
- **Public APIs / Opcodes**: `t81::tisc::Opcode`, `t81::tisc::Insn`, `t81::tisc::opcode_name()`, `t81::vm::derive_initial_transition()`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: Fixed-width 13-byte instruction encoding (`uint8_t opcode`, `int32_t A, B, C`). Machine state model `STATE = (R, PC, SP, FLAGS, MEM, META)` with 243 flat registers. ISA frozen at v1.1.0 (209 active opcodes).
- **Tests & Oracles**: `python3 scripts/ci/check_tisc_freeze_integrity.py`, `t81_vm_decoder_test`, `tisc_opcode_family_semantics_test`.

### 2.2 Interpreter Dispatch & Decoder
- **Applicable RFCs**: RFC-0000, RFC-0002, RFC-0031.
- **Source Paths**: `/vm/vm.cpp`, `/vm/decoder.cpp`.
- **Public APIs / Opcodes**: `t81::vm::Interpreter`, `IVirtualMachine::run_to_halt()`, `IVirtualMachine::step()`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`, `MEASURED`.
- **Status & Supported Scope**: Full 209-opcode interpreter dispatch loop with instruction-level Axion policy hooks, trace logging, step budgeting, and trap handling.
- **Tests & Oracles**: `spec_conformance_tisc_*` suite (10 tests), `t81_vm_stubbed_opcode_fail_closed_test`.

---

## 3. T81Lang Compiler & Runtime Infrastructure

### 3.1 Lexer, Parser & AST
- **Applicable RFCs**: RFC-0007, RFC-0011, RFC-0015, RFC-0029, RFC-0036.
- **Source Paths**: `/lang/frontend/lexer.cpp`, `/lang/frontend/parser.cpp`, `/include/t81/frontend/ast.hpp`.
- **Public APIs / Constructs**: `fn`, `let`, `if/else`, `while`, `match`, `struct`, `enum`, `agent`, `foreign fn`, `@pure`, `@module`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: Hand-written recursive-descent lexer and parser. Full support for algebraic data types, generics monomorphization, pattern matching, and agent declarations.
- **Tests & Oracles**: `e2e_advanced_features_test`, `e2e_match_expression_test`, `e2e_option_result_test`.

### 3.2 Semantic Analysis & IR Generator
- **Applicable RFCs**: RFC-0007, RFC-0023, RFC-0029.
- **Source Paths**: `/lang/frontend/semantic_analyzer.cpp`, `/lang/frontend/ir_generator.cpp`.
- **Public APIs**: `t81::frontend::SemanticAnalyzer`, `t81::frontend::IRGenerator`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: Type checking, purity checking (`@pure`), tier-gate verification, and lowering AST to `IntermediateProgram` IR.
- **Tests & Oracles**: `test_t81lang_traceability_enforcement`, `e2e_compile_determinism_test`.

---

## 4. T81VM Execution, Memory Model, Traces & JIT

### 4.1 Memory Architecture & Stack/Heap
- **Applicable RFCs**: RFC-0006, RFC-0045, RFC-0047.
- **Source Paths**: `/vm/internal/memory_segments.cpp`, `/vm/gc_helpers.cpp`, `/vm/vm.cpp`.
- **Public APIs / Opcodes**: `HeapAlloc` (`0x46`), `HeapFree` (`0x47`), `GcSafepoint` (`0x80`), `t81::vm::gc_mark_and_sweep()`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: Segmented memory space (Stack, Heap, Tensor Pool, Meta Space). Mark-and-sweep GC with compaction and safepoints triggered at byte threshold `3^12` (531,441 bytes).
- **Tests & Oracles**: `t81_vm_system_registers_deterministic_test`, `vm_gc_safepoint_test`, `axion_heap_compaction_trace_test`.

### 4.2 Deterministic Trace JIT Engine
- **Applicable RFCs**: RFC-0028, RFC-0047.
- **Source Paths**: `/vm/jit/jit_engine.cpp`, `/vm/jit/jit_cache.cpp`.
- **Public APIs**: `t81::vm::jit::JitEngine`, `JitTraceCache`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`, `MEASURED`.
- **Status & Supported Scope**: Trace-based JIT compiler with On-Stack Replacement (OSR) and CanonFS-backed JIT artifact caching (`JitTraceCache`).
- **Tests & Oracles**: `t81_jit_test`, `jit_trace_equivalence_test`, `jit_canonfs_cache_test`, `jit_repro_oracle_test`.
- **Measured Results**: JIT compilation speeds up loop-heavy scalar execution by 5x–20x over interpreter loops while preserving exact trace equivalence.

---

## 5. SWAR, SIMD & Vector Acceleration

### 5.1 SWAR & SIMD Kernel Integration
- **Applicable RFCs**: RFC-0017, RFC-0018, RFC-0040, RFC-0041, RFC-0042, RFC-0044, RFC-0050.
- **Source Paths**: `/include/t81/simd/simd.hpp`, `/include/t81/simd/swar.hpp`, `/include/t81/simd/packed_trit_vector.hpp`.
- **Public APIs / Opcodes**: `t81::simd::add()`, `t81::simd::swar_not()`, `TNOT_SWAR` (`0xD5`), `TAND_SWAR` (`0xD6`), `TOR_SWAR` (`0xD7`).
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`, `MEASURED`, `COMPARATIVELY VALIDATED`.
- **Status & Supported Scope**: AVX2 and ARM64 NEON vector kernels under `simd.hpp` facade. SWAR formalization permits 64-trit / 128-trit parallel operations per register.
- **Tests & Oracles**: `test_swar`, `t81_simd_add_helpers_test`, `tisc_semantic_equivalence_test`.
- **Measured Results**: Scientific Investigation 05 recorded packed trit matvec and SWAR throughput metrics (`PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md`).

---

## 6. CanonFS Identity, Serialization & Persistence

### 6.1 Content-Addressable Storage & CanonHash81
- **Applicable RFCs**: RFC-0025, RFC-0054, RFC-00C0, RFC-00C1, RFC-00D1.
- **Source Paths**: `/fs/canon_driver.cpp`, `/include/t81/canonfs/canon_driver.hpp`, `/fs/interchange_ops.cpp`.
- **Public APIs**: `t81::canonfs::make_persistent_driver()`, `t81::canonfs::write_object()`, `t81::canonfs::read_object_bytes()`, CLI `t81 canonfs put-file/import/export/snapshot/fsck/repair`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: Content-addressable object store keyed by SHA3-256 (`sha3-256:<hash>`). Manages raw blocks, snapshots, interchange manifests, and provenance records.
- **Tests & Oracles**: `t81_cli_contract_test_runner`, `canonfs_trace_demo`, `t81_canonfs_rs_repair_test`.

---

## 7. Axion Policy, Capabilities & Governance

### 7.1 Axion Policy Engine & APL
- **Applicable RFCs**: RFC-0003, RFC-0019, RFC-0020, RFC-0022, RFC-00A6.
- **Source Paths**: `/src/policy/axion_policy.cpp`, `/include/t81/axion/policy_validator.hpp`, `/vm/policy_trace_bridge.cpp`.
- **Public APIs / Opcodes**: `t81::axion::PolicyEngine`, `t81::axion::validate_policy()`, opcodes `AxRead` (`0x16`), `AxSet` (`0x17`), `AxVerify` (`0x18`), `AxCheck` (`0x9E`), `AxReport` (`0xA2`).
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: S-expression policy language parser and evaluation engine. Enforces allowed tensor hashes (`allowed-tensor-hashes`), tier supervision, and post-execute activation ceilings (`TACT`). Fail-closed design.
- **Tests & Oracles**: `axion_policy_invariants_test`, `axion_policy_conformance_matrix_test`, `axion_policy_allow_deny_determinism_test`, `spec_conformance_axion-kernel_*` suite.

---

## 8. Deterministic Parallel Execution (DPE)

### 8.1 DPE Task Graph & Epoch Commitment
- **Applicable RFCs**: RFC-DPE-0001 through RFC-DPE-0009, RFC-0046, RFC-0052, RFC-0053.
- **Source Paths**: `/vm/dpe/dpe_task_runner.cpp`, `/vm/dpe/dpe_epoch_commit.cpp`, `/vm/dpe/dpe_epoch_dag.cpp`, `/vm/dpe/dpe_epoch_parallel.cpp`, `/vm/dpe/dpe_thread_pool.cpp`.
- **Public APIs**: `t81::dpe::DpeTaskRunner`, `DpeEpochCommit`, `DpeThreadPool`, `axion_kernel_submit_epoch()`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`, `MEASURED`.
- **Status & Supported Scope**: Task-graph primitives with topological DAG level sorting, parallel epoch execution, word-tag delta records, float reduction interning, 8-entry epoch history ring, and steady_clock timeout enforcement.
- **Tests & Oracles**: `t81_dpe_task_graph_test`, `t81_dpe_epoch_commit_test`, `t81_dpe_epoch_dag_test`, `t81_dpe_epoch_parallel_test`, `t81_dpe_thread_pool_test`, `t81_ternaryos_epoch_timeout_test`, `t81_ternaryos_epoch_audit_test`, `t81_ternaryos_epoch_history_test`.

---

## 9. AI Inference, Quantization & Tensor Infrastructure

### 9.1 Native Ternary & Quantized Inference Opcodes
- **Applicable RFCs**: RFC-0026, RFC-0031, RFC-0032, RFC-0034, RFC-00A4, RFC-00BA, RFC-00BB, RFC-00D3.
- **Source Paths**: `/include/t81/isa/opcodes.hpp`, `/vm/decoder.cpp`, `/codec/gguf_parser.cpp`, `/tools/model/weights.cpp`, `/tools/model/gguf_import_bridge.cpp`.
- **Public APIs / Opcodes**: `TWMATMUL` (`0xC2`), `TQUANT` (`0xC3`), `TATTN` (`0xC4`), `TWEMBED` (`0xC5`), `TERNACCUM` (`0xC6`), `TACT` (`0xC7`), `ATTN` (`0xBB`), `QMATMUL` (`0xBC`), `EMBED` (`0xBD`), `WLOAD` (`0xBE`), CLI `t81 weights import/export/quantize`, `t81 model import/diff`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`, `MEASURED`, `COMPARATIVELY VALIDATED`.
- **Status & Supported Scope**: Full GGUF/SafeTensors model import bridge supporting 7 architecture compatibility profiles (`llama-dense-v1`, `bitnet-b1.58-v1`, `gemma`, `mistral`, `phi3`, `qwen2`, `native-dense-v1`). Native VM fast-path execution of ternary weights.
- **Tests & Oracles**: `spec_conformance_ai_*` suite (9 tests), `safetensors_import_test`, `gguf_import_bridge_test`, `t81_weights_quantize_test`, `t81_cli_contract_test_runner`.
- **Measured Results**: Native VM vector acceleration achieves 62x–67x speedups for unary ternary ops, 6259x for `TQUANT`, 6834x for `TACT`, 80.5x for `TERNACCUM`, 11.8x–570x for `TWMATMUL`, and 110,970x for `TATTN` over unoptimized interpreter loops.

---

## 10. Symbolic, Recursive & Cognitive Tier Mechanisms

### 10.1 Cognition Tiers & Self-Model Ring Buffer
- **Applicable RFCs**: RFC-0015, RFC-0021.
- **Source Paths**: `/include/t81/cognition/tier4.hpp`, `/src/cognition/tier4.cpp`, `/vm/tier_limits.cpp`.
- **Public APIs / Opcodes**: `t81::cognition::SelfModel`, `TierAwarePlanner`, opcodes `SymLoad` (`0x85`), `SymRewrite` (`0x86`), `SymCanon` (`0x88`), `ReflCap` (`0x8A`), `Recurse` (`0x8F`), `AgentInvoke` (`0xC9`).
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: Cognitive Tiers 1–4 execution limits. 81-entry self-model ring buffer (`SelfModel`), recursive improvement bounds, and agent behavior invocation (`AGENT_INVOKE`).
- **Tests & Oracles**: `t81_tier4_vm_test`, `t81_tier4_distributed_test`, `agent_constructs_test`, `spec_conformance_cognitive-tiers_tier-annotation-enforcement`.

---

## 11. TernaryOS, HAL, MMU & Kernel

### 11.1 Axion Kernel, HAL & EL0 Userland
- **Applicable RFCs**: RFC-00B0 through RFC-00B9, RFC-00BC through RFC-00CE.
- **Source Paths**: `/kernel/axion/kernel.cpp`, `/ternaryos/kernel/axion/mmu.cpp`, `/ternaryos/kernel/axion/scheduler.cpp`, `/ternaryos/kernel/axion/syscall.cpp`, `/ternaryos/userenv/userenv.cpp`.
- **Public APIs**: `KernelCallKind`, `mmu_map()`, `KernelCallResult`, `t81-init`, `t81sh`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: Bare-metal governance kernel, radix page table MMU, cooperative EL0 scheduler, KernelCall ordinal map (frozen `00BD`), fault containment (`00C7-00CE`), and user environment (`t81sh`, default-ON).
- **Tests & Oracles**: `t81_ternaryos_hal_boot_test`, `t81_ternaryos_mmu_test`, `t81_ternaryos_scheduler_test`, `t81_ternaryos_context_switch_test`, `t81_ternaryos_user_env_test`, `t81_ternaryos_shell_session_test`.
- **Limitations & Open Questions**: QEMU/EFI guest target builds require Homebrew LLVM clang (disabled by default in standard Linux build environments).

---

## 12. Governance, Specification Authority & Reproducibility

### 12.1 Conformance Suites & CI Governance
- **Applicable RFCs**: RFC-0027, RFC-0029, RFC-0043, RFC-0048.
- **Source Paths**: `/tests/spec_conformance/`, `/scripts/governance/check_determinism_claims.py`, `/scripts/governance/check_public_api_semver.py`, `/scripts/ci/check_tisc_freeze_integrity.py`.
- **Public APIs / CLI**: `t81 determinism verify/certify/explain/hash`, `python3 scripts/ci/t81lang_repro_gate.py`.
- **Evidence Level**: `SPECIFIED`, `IMPLEMENTED`, `TESTED`.
- **Status & Supported Scope**: Spec-as-executable suite (33 tests). Governance scripts strictly enforce API lockfile digests, TISC ISA opcode freeze (209 opcodes), root directory structure whitelist, and determinism surface registry claim bounds.
- **Tests & Oracles**: `python3 scripts/ci/check_tisc_freeze_integrity.py`, `python3 scripts/governance/check_root_structure.py`, `python3 scripts/governance/check_public_api_semver.py`.
