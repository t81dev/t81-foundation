# RFC-0040: Formalization of SWAR Operations for Deterministic Ternary Computing

- **Author(s):** T81 Foundation Architecture Team
- **Status:** Accepted
- **Created:** 2026-03-18
- **Supersedes:** None

## Summary

This RFC formalizes the SWAR (SIMD Within A Register) operations currently implemented in the experimental PackedTritVector system, promoting them from experimental to stable within the Deterministic Core Profile (DCP). It establishes SWAR as a fundamental building block for deterministic ternary operations, providing bit-exact results while maintaining cross-platform reproducibility.

## Motivation

The current SWAR implementation resides in `experimental/packed_trit_vector.hpp` and serves as a critical performance optimization for small-to-medium sized trit vectors. However, its experimental status prevents broader adoption across the T81 ecosystem. Formalizing SWAR operations will:

1. **Enable Deterministic Performance**: Provide predictable performance characteristics for trit-wise operations below SIMD thresholds
2. **Ensure Cross-Platform Consistency**: Guarantee bit-exact results across x86_64, ARM64, and future architectures
3. **Facilitate JIT Integration**: Allow the Trace-JIT to emit SWAR operations for optimized code generation
4. **Support Ecosystem Growth**: Enable external tools and language bindings to rely on stable SWAR primitives

## Proposal

### Technical Details

#### 1. SWAR Operation Specification

SWAR operations process multiple trits simultaneously using 64-bit word-level parallelism. The implementation uses 2-bit trit encoding (4 trits per byte) for optimal word alignment.

**Trit Encoding Mapping:**

| Binary | Trit | Description |
|--------|------|-------------|
| `00`   | `0`  | Zero value |
| `01`   | `1`  | Positive one |
| `11`   | `-1` | Negative one |
| `10`   | —    | **Invalid** (error detection) |

**Trit Density:**
- 4 trits per byte
- 32 trits per 64-bit word
- 256 trits in the 64-byte SWAR regime

This density sets clear expectations for when SWAR provides benefits over scalar operations.

#### 2. Core SWAR Operations

**Ternary Logic Semantics**

T81 uses the mathematically natural balanced ternary conventions:
- **TAnd** = `min(a,b)` (consensus/minimum)
- **TOr** = `max(a,b)` (any/maximum)
- **TNot** = `-a` (negation)

**Truth Tables:**

| a   | b   | TAnd (min) | TOr (max) |
|-----|-----|------------|-----------|
| -1  | -1  | -1         | -1        |
| -1  |  0  | -1         |  0        |
| -1  | +1  | -1         | +1        |
|  0  | -1  | -1         |  0        |
|  0  |  0  |  0         |  0        |
|  0  | +1  |  0         | +1        |
| +1  | -1  | -1         | +1        |
| +1  |  0  |  0         | +1        |
| +1  | +1  | +1         | +1        |

This convention preserves algebraic properties (idempotence, absorption, distributivity) and aligns with balanced ternary arithmetic used in T81's mathematical foundations.

**Encoding Duality Note:**
With this 2-bit encoding, the high bit acts as a "sign-like" discriminator (0 for non-negative 0/+1, 1 for negative -1), while the low bit distinguishes 0 from non-zero — enabling the simple negation via low-bit flip.

**TNot (Ternary Negation)**
```cpp
static void kernel_not_swar(const uint8_t* src, uint8_t* dst, size_t n);
```
Algorithm: Extract low bits of each 2-bit pair and XOR with shifted version.
- Maps `01` ↔ `11`, leaves `00` unchanged
- Fails on `10` (invalid pattern)
- Equivalent to XOR with `0x55...55` mask after validation

**TAnd (Ternary Conjunction)**
```cpp
static void kernel_and_swar(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n);
```
Algorithm: Compute high bits via OR, low bits via AND, then combine with proper masking using min(a,b) semantics.

**TOr (Ternary Disjunction)**
```cpp
static void kernel_or_swar(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n);
```
Algorithm: Compute high bits via AND, low bits via OR, then combine with proper masking using max(a,b) semantics.

#### 3. Threshold-Based Dispatch

SWAR operations are automatically selected based on data size thresholds:

```cpp
static constexpr size_t AVX2_THRESHOLD_BYTES = 64;  // ~256 trits
static constexpr size_t NEON_THRESHOLD_BYTES = 64;  // ~256 trits
```

Dispatch logic:
- `≤ 8 bytes`: Fastpath with direct 64-bit operations
- `≤ 16 bytes`: Small fastpath with two 64-bit operations  
- `1–63 bytes`: SWAR kernels (SWAR handles 1–63 bytes)
- `≥ 64 bytes`: SIMD (AVX2/NEON) with SWAR fallback for tails

#### 4. API Surface

**Public API (Stable)**
```cpp
namespace t81::swar {
    // Primary operations
    Result<ComputeTritVector> t_not(const ComputeTritVector& input);
    Result<ComputeTritVector> t_and(const ComputeTritVector& a, const ComputeTritVector& b);
    Result<ComputeTritVector> t_or(const ComputeTritVector& a, const ComputeTritVector& b);
    
    // In-place variants for zero-allocation scenarios
    Result<bool> t_not_inplace(ComputeTritVector& input);
    Result<bool> t_and_inplace(ComputeTritVector& a, const ComputeTritVector& b);
    Result<bool> t_or_inplace(ComputeTritVector& a, const ComputeTritVector& b);
    
    // Explicit SWAR selection (for testing/benchmarking)
    Result<ComputeTritVector> t_not_swar(const ComputeTritVector& input);
    Result<ComputeTritVector> t_and_swar(const ComputeTritVector& a, const ComputeTritVector& b);
    Result<ComputeTritVector> t_or_swar(const ComputeTritVector& a, const ComputeTritVector& b);
}
```

**Internal Kernel API**
```cpp
namespace t81::swar::kernel {
    void t_not(const uint8_t* src, uint8_t* dst, size_t len);
    void t_and(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t len);
    void t_or(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t len);
}
```

#### 5. Integration Points

**VM Integration**
- Expose SWAR operations through TISC opcodes `TNOT_SWAR`, `TAND_SWAR`, `TOR_SWAR`
- JIT compiler can emit SWAR kernels for hot loops below SIMD thresholds

**CanonFS Integration**
- SWAR operation traces stored with canonical hash for reproducible builds
- Deterministic cache keys include SWAR operation signatures

### Corner Cases

**Invalid Trit Patterns**
- SWAR kernels validate input data and return `Result<T>::failure` for invalid 2-bit patterns
- Invalid patterns (binary `10`) trigger deterministic error handling
- Validation: Compute running OR of `(byte & 0xAA)` across all bytes; fail if result ≠ 0 (detects any high-bit-set without low-bit-set patterns)
- Note: 0xAA isolates the high bit of each 2-bit trit pair, making this a branchless check for invalid `10` patterns
- Validation overhead: <3% on typical workloads, <8% on smallest vectors (≤8 bytes)

**Unaligned Data**
- Tail handling with `mask_trailing()` ensures partial bytes are properly masked
- Zero-extension rules applied to incomplete final bytes

**Size Mismatches**
- Binary operations validate vector length matches before processing
- Length mismatch returns deterministic error with clear diagnostics

## Impact

### Backward Compatibility

**Breaking Changes:**
- `experimental/packed_trit_vector.hpp` SWAR methods will be deprecated
- Migration path provided through compatibility shim for 2 release cycles

**Non-Breaking Changes:**
- Existing scalar and SIMD operations remain unchanged
- Current dispatch logic continues to work with new stable API

### Performance

**Expected Improvements:**
- 3-5x speedup over scalar operations for 16-256 trit vectors
- Deterministic performance characteristics across platforms
- Zero-allocation in-place variants eliminate memory overhead

**Benchmark Results (Current):**
- TNot: 4.2ns/trit (238 MB/s) vs 18.7ns scalar (53 MB/s)
- TAnd: 5.1ns/trit (196 MB/s) vs 22.3ns scalar (45 MB/s)
- TOr: 5.3ns/trit (189 MB/s) vs 23.1ns scalar (43 MB/s)

*Throughput measured on x86_64 @ 3.2GHz; ARM64 shows similar relative performance*
*Assumes packed 2-bit encoding (4 trits/byte); effective trit bandwidth is 4× byte throughput*

### Security

**Determinism Guarantees:**
- Bit-exact results across all supported architectures
- No floating-point or nondeterministic operations
- Full integration with Axion policy boundaries

**Memory Safety:**
- Bounds checking on all array accesses
- Explicit validation of input data patterns
- Safe handling of unaligned and partial data

## Alternatives Considered

**Pure Scalar Implementation**
- Rejected: Unacceptable performance degradation
- Would violate T81 performance requirements for AI workloads

**SIMD-Only Approach**
- Rejected: SIMD overhead dominates for small vectors
- Would leave performance gap for common tensor sizes

**Hardware-Specific Optimizations**
- Rejected: Violates cross-platform determinism requirements
- Would compromise bit-exact reproducibility

**Lookup Table (LUT) Approach**
- Rejected: Memory bandwidth becomes bottleneck
- Cache misses introduce nondeterministic latency

**4-Trit Shuffle LUT Approach**
- Rejected: 16→8-bit shuffle operations lose to mask-based SWAR on modern out-of-order CPUs
- Similar approaches tried in ternary hobby projects show poor performance on contemporary silicon

## Implementation Roadmap

### Phase 1: API Stabilization (Week 1-2)
- [x] Create `include/t81/swar/swar.hpp` with stable API
- [x] Implement compatibility shim in experimental namespace
- [x] Add comprehensive unit tests for all SWAR operations

### Phase 2: VM Integration (Week 3-4)  
- [x] Add TISC opcodes for SWAR operations
- [x] Update VM dispatch layer with SWAR support
- [ ] Integrate with Axion policy boundaries

### Phase 3: JIT Integration (Week 5-6)
- [x] Extend Trace-JIT to emit SWAR kernels
- [x] Add SWAR operation caching to CanonFS
- [x] Implement deterministic trace hashing for SWAR

### Phase 4: Documentation & Migration (Week 7-8)
- [x] Update T81VM specification with SWAR operations
- [x] Create migration guide for existing code
- [x] Add performance tuning guidelines

## Current Implementation Status

Implemented as of 2026-03-18:

- Stable SWAR API in `include/t81/swar/swar.hpp`
- Compatibility shim retained in `include/t81/experimental/packed_trit_vector.hpp`
- Explicit VM opcodes `TNOT_SWAR`, `TAND_SWAR`, `TOR_SWAR`
- Interpreter dispatch over `ExactTrit` tensor handles with shape/type faulting
- Setun bridge mnemonics for text assembly authoring
- Trace-JIT execution, trace hashing, and CanonFS cache roundtrip coverage
- Axion log emission in the interpreter and SWAR policy enforcement across the JIT path
- Normative opcode/spec documentation in `spec/tisc-spec.md` and `spec/tisc/opcode-*.md`
- Migration guidance in `docs/process/migration/RFC_0040_SWAR_MIGRATION.md`
- Performance guidance in `docs/explanation/performance-strategy.md`
- Benchmark snapshot in `docs/records/status-history/RFC_0040_SWAR_EVIDENCE_2026-03-18.md`

Still open:

- Cross-architecture bit-exact evidence refresh on x86_64 (pending CI x86_64 runner)

Status 2026-03-22: accepted in-repo. ARM64 evidence is current (see
`RFC_0041_SIMD_EVIDENCE_2026-03-22.md` §Benchmark Results for refreshed numbers).
Deprecation wording for `t81/experimental/packed_trit_vector.hpp` is now in place
(`#pragma message` guard, 2026-03-22). The sole remaining `stable`-promotion item is
the x86_64 evidence refresh.

### Future Operations Roadmap
After core SWAR stabilization, priority extensions for ternary ML/AI workloads:
1. **Ternary ADD** (with carry trit — critical for MAC operations)
2. **Ternary MUL** (implemented via lookup or shift-add patterns)
3. **Compare/Clip/Abs** operations for neural network activation functions
4. **Population count** and **bitmask extraction** for sparse tensor operations
5. **Reduction operations** (sum, product, consensus) for vector operations

## Acceptance Criteria

| ID | Criterion | Status |
| :--- | :--- | :--- |
| [A-0040-01] | All SWAR operations produce bit-exact results across x86_64 and ARM64 | Accepted in-repo; refreshed cross-architecture evidence is still pending for the next status transition, while local ARM64 backend/JIT/VM coverage is already in place |
| [A-0040-02] | Performance benchmarks meet or exceed current implementation | Met for promoted reference baseline: `docs/records/status-history/RFC_0040_SWAR_EVIDENCE_2026-03-18.md` shows SWAR materially ahead of the Phase 2A reference path on ARM64; LUT remains competitive on small local cases but is not the promoted VM/JIT path |
| [A-0040-03] | VM integration passes full conformance test suite | Met: `t81_vm_rfc0040_swar_test`, `tisc_opcode_matrix_test`, `t81_vm_tisc_v04_extensions_test` |
| [A-0040-04] | JIT integration maintains determinism invariants | Met: `jit_trace_equivalence_test`, `jit_repro_oracle_test`, `jit_canonfs_cache_test`, including SWAR policy enforcement coverage |
| [A-0040-05] | Backward compatibility maintained through deprecation cycle | Met: stable API plus deprecated compatibility shim in `experimental/packed_trit_vector.hpp` |
| [A-0040-06] | Documentation and migration guide complete | Met: spec updates plus `docs/process/migration/RFC_0040_SWAR_MIGRATION.md`, `docs/explanation/performance-strategy.md`, and the evidence snapshot |

## References

- RFC-0001: Architecture Principles
- RFC-0002: Deterministic Execution Contract  
- RFC-0028: Deterministic Trace-JIT
- `include/t81/experimental/packed_trit_vector.hpp` (current implementation)
- `benchmarks/BM_PackedTritVector.cpp` (performance validation)
- `tests/cpp/test_packed_trit_vector.cpp` (existing test coverage)
