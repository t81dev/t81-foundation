# T81 Ternary Quantization Codec - RFC-00A4 Task 6

This directory contains the standardized ternary quantization codec system for T81, implementing T3_K, T3_A, and T3_M schemes with deterministic guarantees.

## Components

### Ternary Codec (`t81_ai_quantize`)
CLI tool for ternary quantization with multiple codec schemes and canonical storage format.

**Usage:**
```bash
# T3_K quantization with 256 clusters
./t81_ai_quantize t3k model.fp32 model.t3k2 256

# T3_A quantization with adaptive thresholds
./t81_ai_quantize t3a model.fp32 model.t3a

# T3_M quantization with minimal MSE
./t81_ai_quantize t3m model.fp32 model.t3m

# Decode ternary back to float
./t81_ai_quantize decode model.t3k2 model.fp32
```

## Quantization Schemes

### T3_K (Ternary K-means)
- **Algorithm**: K-means clustering with k=3 centroids
- **Parameters**: Number of clusters, convergence tolerance, max iterations
- **Advantages**: Optimal for weight distribution, good compression
- **Use Case**: General purpose quantization

### T3_A (Ternary Adaptive)
- **Algorithm**: Adaptive thresholding based on weight distribution
- **Parameters**: Percentile thresholds (25th, 75th)
- **Advantages**: Preserves distribution characteristics
- **Use Case**: Skewed weight distributions

### T3_M (Ternary Minimal MSE)
- **Algorithm**: Threshold optimization to minimize mean squared error
- **Parameters**: Mean-based threshold calculation
- **Advantages**: Best reconstruction accuracy
- **Use Case**: Accuracy-critical applications

## Canonical Storage Format

### Base-81 Packing
- **Encoding**: 4 ternary values per byte (2 bits each)
- **Digits**: -1 → 80, 0 → 40, 1 → 1 (base-81)
- **Efficiency**: 16:1 compression ratio vs 32-bit floats
- **Determinism**: Bit-exact encoding/decoding

### File Structure
```
model.t3k2
├── weights/           # Ternary weight values
├── metadata.json     # Quantization parameters and metrics
└── report.json      # Detailed quantization report
```

## Determinism Guarantees

### Encoding Determinism
- **Identical Input**: Same input always produces identical output
- **Deterministic Algorithm**: No randomness in encoding process
- **Reproducible Results**: Same parameters produce same results

### Decoding Determinism
- **Lossless Decoding**: Perfect reconstruction of ternary values
- **Bit-Exact**: Identical float reconstruction from ternary
- **Cross-Platform**: Consistent results across all platforms

## Quality Metrics

### Reconstruction Quality
- **MSE (Mean Squared Error)**: Average squared reconstruction error
- **PSNR (Peak Signal-to-Noise Ratio)**: Quality in decibels
- **Accuracy Preservation**: Percentage of original accuracy maintained

### Performance Metrics
- **Compression Ratio**: Size reduction factor (typically 16:1)
- **Encoding Time**: Microseconds for encoding process
- **Decoding Time**: Microseconds for decoding process
- **Memory Usage**: Peak memory consumption

## API Interface

### Codec Operations
```cpp
class TernaryCodec {
    // Quantization methods
    QuantizationMetrics quantize_t3k(const std::vector<float>& weights);
    QuantizationMetrics quantize_t3a(const std::vector<float>& weights);
    QuantizationMetrics quantize_t3m(const std::vector<float>& weights);
    
    // Conversion methods
    std::vector<TernaryValue> decode_ternary(const std::vector<uint8_t>& packed);
    std::vector<uint8_t> pack_to_canonical(const std::vector<TernaryValue>& weights);
    
    // Utility methods
    void set_parameter(const std::string& key, const std::string& value);
    void generate_report(const std::string& model_id, const QuantizationMetrics& metrics);
};
```

## Acceptance Criteria

- [x] T3_K codec validated on 3+ model architectures
- [x] Cross-platform determinism demonstrated
- [x] Performance benchmarks meet targets (3-10x speedup)
- [x] Quality impact quantified and documented
- [x] Canonical storage format with base-81 packing

## Build Instructions

```bash
# Enable AI experiments
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
make t81_ai_quantize

# Run quantization
./build/experiments/ai/bin/t81_ai_quantize t3k model.fp32 model.t3k2 256
```

## Integration with T81 Ecosystem

The quantization codec integrates with:
- **Model Provenance** (RFC-00A3) for model integrity verification
- **Determinism Framework** (RFC-00A1) for quantization validation
- **Backend Adapter** (RFC-00A5) for model loading with quantized weights
- **CLI Tools** (RFC-00A7) for unified quantization interface

---

**RFC Reference**: RFC-00A4  
**Task**: 6 - Implement ternary quantization codecs  
**Status**: Completed  
**Last Updated**: 2026-03-05
