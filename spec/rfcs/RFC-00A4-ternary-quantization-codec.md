# RFC-00A4: Ternary Quantization Codec Contract (T3_K and Friends)

Version 0.1 — Standards Track\
Status: Draft\
Author: T81 Foundation Architecture Team\
Applies to: Quantization Codecs, Ternary Operations, Model Compression

______________________________________________________________________

## Summary

This RFC defines a standardized contract for ternary quantization codecs in the T81 ecosystem. It establishes clean APIs, deterministic guarantees, and performance metrics for quantization schemes including T3_K (ternary 3-bit), enabling efficient neural network compression while maintaining reproducibility.

______________________________________________________________________

## Motivation

Ternary quantization offers significant advantages for AI workloads:
- 3x memory compression compared to 8-bit quantization
- Deterministic arithmetic operations
- Hardware-friendly computation patterns

However, without a standardized codec contract, different implementations produce incompatible results, making reproducibility and collaboration impossible. This RFC provides the foundation for consistent ternary quantization across the T81 ecosystem.

## Proposal

### Technical Details

#### 1. Codec Classification System

**Quantization Families:**
- **T3_K**: Balanced ternary quantization (-1, 0, +1)
- **T3_A**: Asymmetric ternary quantization
- **T3_M**: Mixed-precision ternary (variable bits per weight)
- **T3_S**: Sparse ternary (with zero-skipping)

**Precision Levels:**
```yaml
precision_levels:
  T3_K1:  # 1 trit per weight
    bits_per_weight: 1.585
    compression_ratio: 5.06x
    accuracy_impact: high
    
  T3_K2:  # 2 trits per weight
    bits_per_weight: 3.17
    compression_ratio: 2.53x
    accuracy_impact: medium
    
  T3_K3:  # 3 trits per weight
    bits_per_weight: 4.75
    compression_ratio: 1.69x
    accuracy_impact: low
```

#### 2. Codec Interface Specification

**Core Codec API:**
```cpp
namespace t81::ai::quantization {

class TernaryCodec {
public:
    // Codec identification
    virtual std::string codec_name() const = 0;
    virtual std::string version() const = 0;
    virtual uint32_t bits_per_weight() const = 0;
    
    // Encoding operations
    virtual CanonHash encode(
        const Tensor<float>& weights,
        CanonFS::Writer& output
    ) const = 0;
    
    // Decoding operations
    virtual Tensor<float> decode(
        const CanonHash& encoded_hash,
        CanonFS::Reader& input
    ) const = 0;
    
    // Quality metrics
    virtual QuantizationMetrics measure_quality(
        const Tensor<float>& original,
        const Tensor<float>& reconstructed
    ) const = 0;
    
    // Determinism validation
    virtual bool verify_determinism(
        const Tensor<float>& weights,
        uint32_t test_runs = 100
    ) const = 0;
};

}
```

**Metrics Structure:**
```cpp
struct QuantizationMetrics {
    double mse;                    // Mean squared error
    double snr;                    // Signal-to-noise ratio
    double cosine_similarity;      // Angular similarity
    double sparsity;               // Percentage of zeros
    uint64_t compression_ratio;    // Size reduction factor
    double encoding_time_ms;       // Encoding performance
    double decoding_time_ms;       // Decoding performance
    bool deterministic;            // Determinism verification
};
```

#### 3. T3_K Implementation Specification

**Balanced Ternary Algorithm:**
```cpp
class T3_KCodec : public TernaryCodec {
public:
    CanonHash encode(
        const Tensor<float>& weights,
        CanonFS::Writer& output
    ) const override {
        // Step 1: Analyze weight distribution
        auto stats = analyze_distribution(weights);
        
        // Step 2: Compute quantization thresholds
        float threshold = compute_optimal_threshold(stats);
        
        // Step 3: Quantize to balanced ternary
        Tensor<Trit> quantized(weights.shape());
        for (size_t i = 0; i < weights.size(); ++i) {
            if (weights[i] > threshold) {
                quantized[i] = Trit::POSITIVE;
            } else if (weights[i] < -threshold) {
                quantized[i] = Trit::NEGATIVE;
            } else {
                quantized[i] = Trit::ZERO;
            }
        }
        
        // Step 4: Pack trits into base-81 groups
        auto packed = pack_trits_base81(quantized);
        
        // Step 5: Write to CanonFS with metadata
        return write_canonical_format(packed, stats, output);
    }
    
private:
    struct DistributionStats {
        float mean;
        float std_dev;
        float min_val;
        float max_val;
        float percentile_99;
    };
    
    float compute_optimal_threshold(
        const DistributionStats& stats
    ) const {
        // Use percentile-based threshold for robustness
        return std::max(
            std::abs(stats.percentile_99),
            std::abs(stats.mean + 2 * stats.std_dev)
        ) * 0.1f;  // Conservative scaling factor
    }
};
```

#### 4. Canonical Storage Format

**Ternary Tensor Structure:**
```c
// Canonical Ternary Tensor Format
struct TernaryTensorHeader {
    uint8_t magic[4];          // "T3TR"
    uint8_t version;           // Format version
    uint8_t codec_id;          // T3_K, T3_A, etc.
    uint8_t precision;         // K1, K2, K3, etc.
    uint32_t rank;             // Tensor rank
    uint64_t shape[8];         // Tensor dimensions
    uint64_t trit_count;       // Total number of trits
    uint64_t packed_size;      // Size of packed data
    float threshold;           // Quantization threshold
    uint8_t reserved[32];      // Future use
};

// Followed by packed trit data in base-81 encoding
```

**Packing Algorithm:**
```cpp
// Pack 4 trits into one base-81 digit (0-80)
uint8_t pack_trits_quad(const Trit trits[4]) {
    uint8_t result = 0;
    for (int i = 0; i < 4; ++i) {
        uint8_t trit_val;
        switch (trits[i]) {
            case Trit::NEGATIVE: trit_val = 0; break;
            case Trit::ZERO:     trit_val = 1; break;
            case Trit::POSITIVE: trit_val = 2; break;
        }
        result = result * 3 + trit_val;
    }
    return result;  // 0-80 range
}
```

#### 5. Determinism Guarantees

**Deterministic Encoding:**
- Identical input weights produce identical encoded output
- Threshold computation uses deterministic algorithms
- Bit-exact reproducibility across platforms
- All random operations use fixed seeds

**Deterministic Decoding:**
- Perfect reconstruction of quantized values
- No platform-dependent arithmetic
- Consistent rounding behavior
- Verified against reference implementation

**Validation Protocol:**
```bash
# Test codec determinism
t81 ai quantization test-determinism \
  --codec T3_K2 \
  --input model_weights.npy \
  --runs 1000 \
  --tolerance 0

# Cross-platform validation
t81 ai quantization cross-platform-test \
  --codec T3_K2 \
  --platforms "darwin-arm64,linux-x86_64" \
  --input model_weights.npy
```

#### 6. Performance Benchmarks

**Target Performance Metrics:**
```yaml
performance_targets:
  encoding:
    throughput: "1GB/s"  # Weights encoded per second
    memory_overhead: "2x"  # Additional memory during encoding
    accuracy_preservation: ">95%"  # Model accuracy retained
    
  decoding:
    throughput: "2GB/s"  # Weights decoded per second
    memory_overhead: "1.5x"  # Additional memory during decoding
    latency: "<10ms"  # Time to decode 1M parameters
    
  compression:
    ratio: "3-5x"  # Size reduction vs FP32
    quality: "SNR > 20dB"  # Signal quality metric
```

### Corner Cases

#### Edge Case Weights
- Zero weights handled efficiently
- Outlier values clipped appropriately
- NaN/infinite values rejected with clear errors

#### Memory Constraints
- Streaming encoding for large tensors
- Chunked processing for memory-limited environments
- Progressive quality levels

#### Platform Differences
- Endianness handling in canonical format
- Floating-point format consistency
- Alignment and padding requirements

## Impact

### Backward Compatibility

No impact on existing code. New quantization features are opt-in.

### Performance

Quantization operations add computational overhead:
- Encoding: ~10-50ms per million parameters
- Decoding: ~5-20ms per million parameters
- Memory usage: 60-70% reduction for T3_K2

### Security

Enhanced security through:
- Deterministic quantization prevents manipulation
- Cryptographic hashing of quantized artifacts
- Tamper-evident encoding process

## Alternatives Considered

1. **Binary quantization**: Rejected due to excessive accuracy loss
2. **Standard 8-bit quantization**: Rejected due to lack of ternary advantages
3. **Custom per-model quantization**: Rejected due to reproducibility issues

## UX / Developer Experience Impact

### CLI Interface

```bash
# Quantize model with T3_K2
t81 ai quantize \
  --input model.fp32 \
  --output model.t3k2 \
  --codec T3_K2 \
  --threshold auto

# Compare quantization quality
t81 ai quantization compare \
  --original model.fp32 \
  --quantized model.t3k2 \
  --metrics mse,snr,accuracy

# Test codec performance
t81 ai quantization benchmark \
  --codec T3_K2 \
  --model llama-7b \
  --iterations 100

# Validate determinism
t81 ai quantization verify-determinism \
  --codec T3_K2 \
  --input model.fp32
```

### IDE Integration

- Quantization quality visualization
- Real-time accuracy impact assessment
- Automatic codec selection based on constraints
- Performance profiling and optimization suggestions

### Model Development

- Automatic quantization during training
- Progressive quantization for large models
- Quality-aware fine-tuning
- Compression-aware architecture design

## Acceptance Criteria

1. T3_K codec implements full API specification
2. Determinism validation passes across platforms
3. Performance targets met for encoding/decoding
4. Quality metrics meet specified thresholds
5. Integration with CanonFS and model provenance

## Promotion Gates

### Experimental → Extension
- [ ] T3_K codec validated on 3+ model architectures
- [ ] Cross-platform determinism demonstrated
- [ ] Performance benchmarks meet targets
- [ ] Quality impact quantified and documented

### Extension → Core
- [ ] Adopted as standard quantization in T81
- [ ] All official models support ternary quantization
- [ ] Hardware acceleration support implemented
- [ ] Community adoption and feedback incorporated

## Impact

### Backward Compatibility

No impact on existing code. New quantization features are opt-in.

### Performance

Quantization operations add computational overhead:
- Encoding: ~10-50ms per million parameters
- Decoding: ~5-20ms per million parameters
- Memory usage: 60-70% reduction for T3_K2

### Security

Enhanced security through:
- Deterministic quantization prevents manipulation
- Cryptographic hashing of quantized artifacts
- Tamper-evident encoding process

______________________________________________________________________

## Alternatives Considered

1. **Binary quantization**: Rejected due to excessive accuracy loss
2. **Standard 8-bit quantization**: Rejected due to lack of ternary advantages
3. **Custom per-model quantization**: Rejected due to reproducibility issues

______________________________________________________________________

## References

- [Ternary Tensor Type](/spec/rfc/RFC-0012-ternary-tensor-quantization.md)
- [T81 Data Types Specification](/spec/t81-data-types.md)
- [CanonFS Specification](/spec/supplemental/canonfs-spec.md)
- [Deterministic Evidence Protocol](/spec/rfc/RFC-00A1-deterministic-evidence-protocol.md)
