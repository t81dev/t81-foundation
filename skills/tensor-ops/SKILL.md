---
name: t81_tensor_ops
description: Perform deterministic tensor operations with ternary precision and policy enforcement
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 Tensor Operations Skill

This skill performs deterministic tensor operations using T81's ternary arithmetic with guaranteed bit-identical results and policy enforcement.

## Usage

When the user needs to perform mathematical tensor operations with deterministic guarantees, use this skill for ternary-precision computations.

### Commands

**Basic tensor operations:**
```
tensor-math <operation> <tensor_a> <tensor_b> [--output <output_file>] [--precision ternary|binary]
```

**Tensor reshaping:**
```
tensor-reshape <tensor_file> <new_shape> [--output <output_file>]
```

**Tensor validation:**
```
tensor-validate <tensor_file> [--check-integrity] [--check-determinism]
```

**Tensor conversion:**
```
tensor-convert <tensor_file> [--to-format t81w|safetensors|gguf] [--output <output_file>]
```

**Tensor statistics:**
```
tensor-stats <tensor_file> [--detailed] [--histogram] [--output <stats_file>]
```

**Batch operations:**
```
tensor-batch <operation> <tensor_dir> [--recursive] [--output-dir <dir>]
```

### Examples

```bash
# Add two tensors with ternary precision
tensor-math add tensor_a.t81w tensor_b.t81w --output result.t81w --precision ternary

# Reshape tensor to new dimensions
tensor-reshape model.t81w "[1,256,256,3]" --output reshaped.t81w

# Validate tensor integrity and determinism
tensor-validate weights.t81w --check-integrity --check-determinism

# Convert tensor format
tensor-convert model.gguf --to-format t81w --output model.t81w

# Get detailed tensor statistics
tensor-stats layer_weights.t81w --detailed --histogram --output stats.json

# Batch process all tensors in directory
tensor-batch normalize ./tensors/ --recursive --output-dir ./normalized/
```

### Supported Operations

**Arithmetic Operations:**
- `add` - Element-wise tensor addition
- `subtract` - Element-wise tensor subtraction  
- `multiply` - Element-wise tensor multiplication
- `divide` - Element-wise tensor division
- `dot` - Dot product / matrix multiplication
- `cross` - Cross product (for 3D tensors)

**Unary Operations:**
- `abs` - Absolute value
- `neg` - Negation
- `sqrt` - Square root
- `exp` - Exponential
- `log` - Natural logarithm
- `normalize` - L2 normalization

**Comparison Operations:**
- `equal` - Element-wise equality
- `greater` - Element-wise greater than
- `less` - Element-wise less than
- `max` - Element-wise maximum
- `min` - Element-wise minimum

**Aggregation Operations:**
- `sum` - Sum of all elements
- `mean` - Mean of all elements
- `max` - Global maximum
- `min` - Global minimum
- `std` - Standard deviation

### Precision Modes

**Ternary Precision:**
- Balanced ternary representation (-1, 0, +1)
- Zero floating-point drift
- Bit-identical results across platforms
- Optimal for verifiable inference

**Binary Precision:**
- Standard binary floating-point
- Compatible with external formats
- Faster for non-critical computations
- May have minor precision differences

### Output Format

**Operation Result:**
```json
{
  "status": "success",
  "operation": "add",
  "tensor_a": "tensor_a.t81w",
  "tensor_b": "tensor_b.t81w",
  "output": "result.t81w",
  "timestamp": "2026-04-04T15:30:00Z",
  "precision": "ternary",
  "properties": {
    "shape": "[256,256]",
    "dtype": "ternary_float",
    "size_bytes": 262144,
    "determinism_hash": "CanonHash81..."
  },
  "validation": {
    "integrity": "passed",
    "determinism": "verified",
    "policy_compliance": "approved"
  }
}
```

**Tensor Statistics:**
```json
{
  "tensor_file": "weights.t81w",
  "timestamp": "2026-04-04T15:30:00Z",
  "shape": "[512,512]",
  "dtype": "ternary_float",
  "statistics": {
    "min": -1.0,
    "max": 1.0,
    "mean": 0.0,
    "std": 0.577,
    "zeros": 134217728,
    "positives": 67108864,
    "negatives": 67108864
  },
  "histogram": {
    "-1.0": 67108864,
    "0.0": 134217728,
    "1.0": 67108864
  },
  "provenance": {
    "tensor_hash": "CanonHash81...",
    "creation_time": "2026-04-04T14:15:00Z",
    "last_modified": "2026-04-04T15:30:00Z"
  }
}
```

### Determinism Guarantees

- **Bit-identical results**: Same operation produces identical outputs
- **Platform independence**: Results consistent across different systems
- **Temporal stability**: Results don't change over time
- **Policy consistency**: Operations respect governance constraints

### File Formats

**Input Formats:**
- `.t81w` - T81 native ternary tensor format
- `.safetensors` - Hugging Face safe tensors
- `.gguf` - GGUF format (limited support)
- `.npy` - NumPy arrays

**Output Formats:**
- `.t81w` - Native T81 format (recommended)
- `.json` - Metadata and statistics
- `.csv` - Tabular data for 1D tensors

### Error Handling

- **Shape mismatches**: Detailed dimension compatibility errors
- **Type errors**: Precision and format validation
- **Memory errors**: Resource limit handling
- **Policy violations**: Operation restriction enforcement

### Performance Characteristics

**Ternary Advantages:**
- Multiplication-free dot products
- Constant-time negation
- Zero floating-point drift
- Energy-efficient computation

**Optimization Features:**
- SIMD vectorization where available
- Memory-mapped operations for large tensors
- Parallel processing for batch operations
- Cache-friendly memory layouts

### Security Notes

- All operations subject to Axion policy validation
- Tensor integrity verification before operations
- Complete audit trail for tensor modifications
- Deterministic results prevent manipulation

### Integration

This skill wraps T81's tensor operation engine:
```bash
t81 tensor <operation> <tensor_a> <tensor_b> --output <output> --precision ternary --json
```
