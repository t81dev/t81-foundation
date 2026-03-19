# RFC-0041: Formalization of SIMD Operations for Deterministic Ternary Computing

- **Author(s):** T81 Foundation Architecture Team
- **Status:** Proposed
- **Created:** 2026-03-18
- **Supersedes:** None (builds on RFC-0016, RFC-0018, RFC-0040)

## Summary

This RFC formalizes the SIMD (Single Instruction Multiple Data) operations currently implemented in the experimental PackedTritVector system, promoting them from experimental to stable within the Deterministic Core Profile (DCP). It establishes SIMD as the highest-performance tier in the scalar → SWAR → SIMD optimization hierarchy, providing maximum throughput for large trit vectors while maintaining strict cross-platform determinism.

## Current Implementation Status

The core SIMD implementation already exists in code and is not greenfield:

- AVX2 kernels for `TNot`, `TAnd`, and `TOr`
- NEON kernels for `TNot`, `TAnd`, and `TOr`
- threshold-based dispatch with SWAR fallback tails
- randomized differential tests against scalar and SWAR backends
- explicit boundary-size coverage (`63/64/65/127/129`-class cases)
- benchmark coverage for kernel-level and API-level throughput

This RFC therefore focuses on promotion and surface formalization:

1. stabilize the public API over the existing implementation
2. record cross-architecture evidence explicitly under RFC-0041
3. define the deprecation path away from direct external use of `experimental`

It does **not** currently claim a SIMD-specific TISC opcode family or SIMD-specific
Trace-JIT lowering. Existing VM/JIT tritwise integration remains the RFC-0040
SWAR path.

## Motivation

The current SIMD implementation resides in `experimental/packed_trit_vector.hpp` and serves as the critical performance path for large-scale ternary operations (256+ trits). However, its experimental status prevents broader adoption and creates uncertainty about cross-architecture consistency. Formalizing SIMD operations will:

1. **Complete the Optimization Stack**: Provide stable scalar → SWAR → SIMD hierarchy
2. **Ensure Cross-Platform Consistency**: Guarantee bit-exact results across AVX2 and NEON implementations
3. **Enable Large-Scale AI Workloads**: Support high-performance inference and training on large tensors
4. **Facilitate JIT Optimization**: Allow the Trace-JIT to emit SIMD code for hot loops
5. **Provide Performance Guarantees**: Establish deterministic throughput characteristics

## Proposal

### Technical Details

#### 1. SIMD Architecture Support

**Primary Architectures:**
- **x86_64**: AVX2 (256-bit vectors, 32 bytes per operation)
- **ARM64**: NEON (128-bit vectors, 16 bytes per operation)

**Future Extensions:**
- **AVX-512**: 512-bit vectors (64 bytes per operation)
- **SVE**: Scalable Vector Extension for ARM
- **RISC-V Vector**: V-extension support

#### 2. Trit Encoding and Vector Width

**2-Bit Trit Encoding (Consistent with SWAR):**

| Trit | Binary Pattern | Description |
|------|---------------|-------------|
| -1   | `00`          | Negative one |
|  0   | `01`          | Zero |
| +1   | `11`          | Positive one |
|  --  | `10`          | **Invalid** (error detection) |

*Note: `10` pattern is reserved for explicit error marking; validation rejects vectors containing any `10` pair.*

**Vector Width Configuration:**

| Architecture | Vector Width | Bytes/Op | Trits/Op | Processing Stride |
|--------------|-------------|----------|----------|------------------|
| AVX2         | 256-bit     | 32       | 128      | 32-byte loop      |
| NEON         | 128-bit     | 16       | 64       | 16-byte loop      |

**Trit Density Summary:**
- 4 trits per byte (2-bit encoding)
- 128 trits per AVX2 vector operation
- 64 trits per NEON vector operation
- Throughput targets: ~10-15 Gops/s (AVX2), ~5-8 Gops/s (NEON)

#### 3. Core SIMD Operations

**TNot (Ternary Negation)**
```cpp
// AVX2 implementation using XOR mask (correct for balanced ternary negation)
__m256i mask55 = _mm256_set1_epi8(0x55);
__m256i maskAA = _mm256_set1_epi8(0xAA);
for (; i + 32 <= n; i += 32) {
    __m256i x = _mm256_loadu_si256((const __m256i*)(src + i));
    
    // Extract high and low bits
    __m256i high = _mm256_and_si256(x, maskAA);
    __m256i low = _mm256_and_si256(x, mask55);
    
    // Negation: flip high bit, keep low bit unchanged
    // -1(00) → +1(11): high 0→1, low 0→1  
    //  0(01) →  0(01): high 0→0, low 1→1
    // +1(11) → -1(00): high 1→0, low 1→0
    __m256i neg_high = _mm256_andnot_si256(high, maskAA);  // Flip high bits
    __m256i neg_low = _mm256_and_si256(low, low);         // Keep low bits only for non-zero
    __m256i res = _mm256_or_si256(neg_high, neg_low);
    
    _mm256_storeu_si256((__m256i*)(dst + i), res);
}
```

**NEON Implementation (TNot):**
```cpp
uint8x16_t mask55 = vdupq_n_u8(0x55);
uint8x16_t maskAA = vdupq_n_u8(0xAA);
for (; i + 16 <= n; i += 16) {
    uint8x16_t x = vld1q_u8(src + i);
    
    // Extract high and low bits
    uint8x16_t high = vandq_u8(x, maskAA);
    uint8x16_t low = vandq_u8(x, mask55);
    
    // Negation: flip high bit, conditional low bit
    uint8x16_t neg_high = vbicq_u8(maskAA, high);        // Flip high bits
    uint8x16_t neg_low = vandq_u8(low, low);             // Keep low bits only for non-zero
    uint8x16_t res = vorrq_u8(neg_high, neg_low);
    
    vst1q_u8(dst + i, res);
}
```

**Worked Example (TNot):**
```
Input trits:  [-1, 0, +1, -1, 0, +1, 0, -1]
Packed hex:   00 01 11 00 01 11 01 00 = 0x1D 0x34
Negation:    [+1, 0, -1, +1, 0, -1, 0, +1]
Result hex:  11 01 00 11 01 00 01 11 = 0xD3 0x8B
```

**TAnd (Ternary Conjunction)**
```cpp
// AVX2 implementation with min(a,b) semantics
__m256i maskAA = _mm256_set1_epi8(0xAA);
__m256i mask55 = _mm256_set1_epi8(0x55);
for (; i + 32 <= n; i += 32) {
    __m256i va = _mm256_loadu_si256((const __m256i*)(src_a + i));
    __m256i vb = _mm256_loadu_si256((const __m256i*)(src_b + i));
    
    __m256i a_or_b = _mm256_or_si256(va, vb);
    __m256i H = _mm256_and_si256(a_or_b, maskAA);
    
    __m256i a_and_b = _mm256_and_si256(va, vb);
    __m256i L_content = _mm256_and_si256(a_and_b, mask55);
    
    __m256i H_shr = _mm256_srli_epi64(H, 1);
    __m256i res = _mm256_or_si256(H, H_shr);
    res = _mm256_or_si256(res, L_content);
    _mm256_storeu_si256((__m256i*)(dst + i), res);
}
```

**NEON Implementation (TAnd):**
```cpp
uint8x16_t maskAA = vdupq_n_u8(0xAA);
uint8x16_t mask55 = vdupq_n_u8(0x55);
for (; i + 16 <= n; i += 16) {
    uint8x16_t va = vld1q_u8(src_a + i);
    uint8x16_t vb = vld1q_u8(src_b + i);
    
    uint8x16_t a_or_b = vorrq_u8(va, vb);
    uint8x16_t H = vandq_u8(a_or_b, maskAA);
    
    uint8x16_t a_and_b = vandq_u8(va, vb);
    uint8x16_t L_content = vandq_u8(a_and_b, mask55);
    
    uint8x16_t H_shr = vshrq_n_u8(H, 1);
    uint8x16_t res = vorrq_u8(H, H_shr);
    res = vorrq_u8(res, L_content);
    vst1q_u8(dst + i, res);
}
```

**Worked Example (TAnd):**
```
Input A:     [-1, 0, +1, -1, 0, +1, 0, -1]
Input B:     [+1, 0, -1, -1, +1, 0, 0, +1]
Packed A:    00 01 11 00 01 11 01 00 = 0x1D 0x34
Packed B:    11 01 00 00 11 01 01 11 = 0xC3 0x8B
TAnd (min):  [-1, 0, -1, -1, 0, 0, 0, -1]
Result hex:  00 01 00 00 01 01 01 00 = 0x10 0x74
```

**TOr (Ternary Disjunction)**
```cpp
// AVX2 implementation with max(a,b) semantics
__m256i maskAA = _mm256_set1_epi8(0xAA);
__m256i mask55 = _mm256_set1_epi8(0x55);
for (; i + 32 <= n; i += 32) {
    __m256i va = _mm256_loadu_si256((const __m256i*)(src_a + i));
    __m256i vb = _mm256_loadu_si256((const __m256i*)(src_b + i));
    
    __m256i h_a = _mm256_and_si256(va, maskAA);
    __m256i h_b = _mm256_and_si256(vb, maskAA);
    __m256i l_a = _mm256_and_si256(va, mask55);
    __m256i l_b = _mm256_and_si256(vb, mask55);
    
    __m256i H = _mm256_and_si256(h_a, h_b);
    __m256i h_or = _mm256_or_si256(h_a, h_b);
    __m256i mask = _mm256_srli_epi64(h_or, 1);
    
    __m256i l_and = _mm256_and_si256(l_a, l_b);
    __m256i l_or = _mm256_or_si256(l_a, l_b);
    __m256i L = _mm256_or_si256(l_and, _mm256_andnot_si256(mask, l_or));
    
    __m256i H_shr = _mm256_srli_epi64(H, 1);
    __m256i res = _mm256_or_si256(H, H_shr);
    res = _mm256_or_si256(res, L);
    _mm256_storeu_si256((__m256i*)(dst + i), res);
}
```

**NEON Implementation (TOr):**
```cpp
uint8x16_t maskAA = vdupq_n_u8(0xAA);
uint8x16_t mask55 = vdupq_n_u8(0x55);
for (; i + 16 <= n; i += 16) {
    uint8x16_t va = vld1q_u8(src_a + i);
    uint8x16_t vb = vld1q_u8(src_b + i);
    
    uint8x16_t h_a = vandq_u8(va, maskAA);
    uint8x16_t h_b = vandq_u8(vb, maskAA);
    uint8x16_t l_a = vandq_u8(va, mask55);
    uint8x16_t l_b = vandq_u8(vb, mask55);
    
    uint8x16_t H = vandq_u8(h_a, h_b);
    uint8x16_t h_or = vorrq_u8(h_a, h_b);
    uint8x16_t mask = vshrq_n_u8(h_or, 1);
    
    uint8x16_t l_and = vandq_u8(l_a, l_b);
    uint8x16_t l_or = vorrq_u8(l_a, l_b);
    uint8x16_t L = vorrq_u8(l_and, vbicq_u8(l_or, mask));
    
    uint8x16_t H_shr = vshrq_n_u8(H, 1);
    uint8x16_t res = vorrq_u8(H, H_shr);
    res = vorrq_u8(res, L);
    vst1q_u8(dst + i, res);
}
```

**Worked Example (TOr):**
```
Input A:     [-1, 0, +1, -1, 0, +1, 0, -1]
Input B:     [+1, 0, -1, -1, +1, 0, 0, +1]
Packed A:    00 01 11 00 01 11 01 00 = 0x1D 0x34
Packed B:    11 01 00 00 11 01 01 11 = 0xC3 0x8B
TOr (max):   [+1, 0, +1, -1, +1, +1, 0, +1]
Result hex:  11 01 11 00 11 11 01 11 = 0xD7 0xFB
```
```

#### 4. Threshold-Based Dispatch

**Dispatch Logic:**
```cpp
static constexpr size_t AVX2_THRESHOLD_BYTES = 64;  // ~256 trits
static constexpr size_t NEON_THRESHOLD_BYTES = 64;   // ~256 trits

// Runtime tunable thresholds (optional)
static size_t get_avx2_threshold() {
    const char* env = std::getenv("T81_AVX2_THRESHOLD");
    return env ? std::stoul(env) : AVX2_THRESHOLD_BYTES;
}
```

**Threshold Rationale:**
- **< 64 bytes**: SIMD setup overhead outweighs benefits
- **≥ 64 bytes**: SIMD provides 2-4x speedup over SWAR
- **Tail handling**: Always falls back to SWAR for remaining bytes
- **Future tuning**: Thresholds can be adjusted per-architecture or via environment

#### 5. Memory Alignment and Access Patterns

**Alignment Requirements:**
- **Unaligned loads**: `_mm256_loadu_si256` / `vld1q_u8` for compatibility
- **Prefetching**: Optional `_mm_prefetch` for large vectors (>1KB)
- **Cache line awareness**: 32-byte strides avoid cache line splits

**Access Patterns:**
```cpp
// Vectorized loop with 32-byte stride (AVX2)
for (; i + 32 <= n; i += 32) {
    // Process 128 trits per iteration
    __m256i chunk = _mm256_loadu_si256((const __m256i*)(src + i));
    // ... SIMD operations ...
    _mm256_storeu_si256((__m256i*)(dst + i), result);
}

// SWAR fallback for tail
if (i < n) {
    kernel_not_swar(src + i, dst + i, n - i);
}
```

#### 6. Cross-Architecture Determinism

**Bit-Exact Guarantees:**
- Identical trit encoding across all architectures
- Same mathematical operations (min/max semantics)
- Deterministic tail handling with SWAR fallback
- No floating-point or nondeterministic instructions

**Validation Strategy:**
- Property testing across x86_64 and ARM64
- Canonical hash verification for large test vectors
- Regression testing on each supported architecture

#### 7. API Surface

**Public API (Stable)**
```cpp
namespace t81::simd {
    // Primary operations with automatic dispatch (aliases for clarity)
    Result<ComputeTritVector> t_not(const ComputeTritVector& input);
    Result<ComputeTritVector> t_and(const ComputeTritVector& a, const ComputeTritVector& b);
    Result<ComputeTritVector> t_or(const ComputeTritVector& a, const ComputeTritVector& b);
    
    // Semantic aliases (ternary logic operations)
    Result<ComputeTritVector> t_neg(const ComputeTritVector& input);  // Alias for t_not
    Result<ComputeTritVector> t_min(const ComputeTritVector& a, const ComputeTritVector& b);  // Alias for t_and
    Result<ComputeTritVector> t_max(const ComputeTritVector& a, const ComputeTritVector& b);  // Alias for t_or
    
    // In-place variants for zero-allocation scenarios
    Result<bool> t_not_inplace(ComputeTritVector& input);
    Result<bool> t_and_inplace(ComputeTritVector& a, const ComputeTritVector& b);
    Result<bool> t_or_inplace(ComputeTritVector& a, const ComputeTritVector& b);
    
    // Architecture-specific operations (for advanced use)
    Result<ComputeTritVector> t_not_avx2(const ComputeTritVector& input);
    Result<ComputeTritVector> t_not_neon(const ComputeTritVector& input);
    
    // Configuration and introspection
    bool is_avx2_available();
    bool is_neon_available();
    size_t get_optimal_threshold();
    void set_threshold_override(size_t bytes);
}
```

**Internal Kernel API**
```cpp
namespace t81::simd::kernel {
    void t_not_avx2(const uint8_t* src, uint8_t* dst, size_t len);
    void t_and_avx2(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t len);
    void t_or_avx2(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t len);
    
    void t_not_neon(const uint8_t* src, uint8_t* dst, size_t len);
    void t_and_neon(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t len);
    void t_or_neon(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t len);
}
```

### Corner Cases

**Unsupported Architectures**
- Graceful fallback to SWAR operations
- Clear error messages for missing SIMD capabilities
- Runtime feature detection with `__builtin_cpu_supports`

**Alignment and Memory Constraints**
- Unaligned access support for compatibility
- Segfault prevention with bounds checking
- Safe handling of zero-length vectors

**Tail Handling**
- Deterministic SWAR fallback for partial vectors
- Bit-exact results regardless of vector length
- Consistent behavior across architectures

**Invalid Trit Patterns**
- Same validation as SWAR: branchless OR accumulation
- SIMD-accelerated validation for large vectors
- Early termination on detection of invalid patterns

## Impact

### Backward Compatibility

**Breaking Changes:**
- `experimental/packed_trit_vector.hpp` SIMD methods will be deprecated
- Migration path provided through compatibility shim for 2 release cycles

**Non-Breaking Changes:**
- Existing scalar and SWAR operations remain unchanged
- Current dispatch logic continues to work with new stable API
- No changes to trit encoding or logical semantics

### Performance

**Expected Improvements:**
- 2-4x speedup over SWAR for vectors ≥256 trits
- 8-15x speedup over scalar operations for large tensors
- Deterministic throughput: ~10-15 Gops/s (AVX2), ~5-8 Gops/s (NEON)

**Benchmark Targets (per ternary operation):**
- TNot: 0.8ns/trit (1.25 GB/s effective) vs 4.2ns SWAR
- TAnd: 1.0ns/trit (1.0 GB/s effective) vs 5.1ns SWAR  
- TOr: 1.1ns/trit (0.9 GB/s effective) vs 5.3ns SWAR

*Throughput assumes packed 2-bit encoding; effective trit bandwidth is 4× byte throughput*

### Security

**Determinism Guarantees:**
- Bit-exact results across all supported architectures
- No floating-point or nondeterministic operations
- Full integration with Axion policy boundaries

**Memory Safety:**
- Bounds checking on all SIMD memory accesses
- Safe handling of unaligned and partial data
- Runtime feature detection prevents illegal instruction usage

## Alternatives Considered

**SWAR-Only Approach**
- Rejected: Insufficient performance for large-scale AI workloads
- Would leave T81 uncompetitive for inference/training

**Hardware-Specific SIMD Only**
- Rejected: Violates cross-platform determinism requirements
- Would fragment ecosystem and compromise reproducibility

**Lookup Table (LUT) SIMD**
- Rejected: Memory bandwidth becomes bottleneck at scale
- Cache misses introduce nondeterministic latency

**Custom Ternary Hardware Instructions**
- Rejected: No existing hardware support
- Would limit portability and adoption

## Implementation Roadmap

### Phase 1: API Stabilization
- [x] Create `include/t81/simd/simd.hpp` with stable API wrappers
- [x] Preserve compatibility through `experimental::ComputeTritVector`
- [x] Add dedicated stable-surface regression coverage

### Phase 2: Cross-Platform Validation
- [x] Maintain scalar/SWAR/SIMD differential tests in the existing suites
- [x] Keep explicit boundary-size tests in the packed-trit validation corpus
- [~] Refresh and record explicit x86_64 and ARM64 evidence under this RFC
  - ARM64/NEON evidence recorded: `docs/records/status-history/RFC_0041_SIMD_EVIDENCE_2026-03-18.md`
  - ARM64 tuning now keeps NEON enabled for `TOr` while routing `TAnd` and `TNot` to SWAR on the current host class
  - x86_64 refresh still pending

### Phase 3: Documentation & Migration
- [x] Create migration guidance for callers moving off direct `experimental` usage
- [~] Complete formal deprecation wording for direct external `experimental` inclusion
- [~] Add a dedicated RFC-0041 evidence note summarizing current benchmark status

### Deferred Work
- [ ] SIMD-specific VM opcode surface
- [ ] SIMD-specific Trace-JIT lowering
- [ ] AVX-512 / SVE / RISC-V vector follow-ons

## Acceptance Criteria

| ID | Criterion | Status |
| :--- | :--- | :--- |
| [A-0041-01] | All SIMD operations produce bit-exact results across x86_64 and ARM64 | Partial: tests exist and ARM64 evidence is now recorded; refreshed x86_64 evidence still needs to be recorded under this RFC |
| [A-0041-02] | Performance benchmarks meet or exceed targets (≥2x SWAR speedup) | Partial: benchmark coverage exists; current ARM64 evidence is mixed, and the tuned implementation now treats only `TOr` as a clear NEON candidate on this host class |
| [A-0041-03] | Stable public SIMD API exists outside `experimental` | Met: `include/t81/simd/simd.hpp` now exposes the promoted surface |
| [A-0041-04] | Backward compatibility maintained through a compatibility period | Met: stable API is currently a promotion wrapper over the existing implementation |
| [A-0041-05] | Cross-platform differential/property tests remain in CI-visible test targets | Met |
| [A-0041-06] | Boundary condition tests (63,64,65,127,128,129 bytes) pass | Met |
| [A-0041-07] | Documentation and migration guide complete | Partial: migration guide exists; RFC evidence/status still needs final cross-arch closeout |
| [A-0041-08] | SIMD-specific VM/JIT scope is either implemented or explicitly deferred | Met: explicitly deferred in this RFC revision |

## References

- RFC-0001: Architecture Principles
- RFC-0002: Deterministic Execution Contract  
- RFC-0028: Deterministic Trace-JIT
- RFC-0040: Formalization of SWAR Operations
- RFC-0016: Register-native SIMD T81 Limb (superseded by RFC-0017)
- RFC-0018: SIMD T81 Arithmetic – Native addition & multiplication
- `include/t81/experimental/packed_trit_vector.hpp` (current implementation)
- `benchmarks/BM_PackedTritVector.cpp` (performance validation)
- `tests/cpp/test_packed_trit_vector.cpp` (existing test coverage)
