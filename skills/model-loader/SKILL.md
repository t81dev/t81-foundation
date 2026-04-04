---
name: t81_model_loader
description: Load external AI models into T81 with secure conversion and immutable storage
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 Model Loader Skill

This skill loads external AI models into T81's ecosystem with secure conversion, deterministic validation, and immutable CanonFS storage.

## Usage

When the user needs to import external AI models (Hugging Face, GGUF, etc.) into T81, use this skill for secure model conversion and storage.

### Commands

**Load Hugging Face model:**
```
load-hf <model_name_or_path> [--output <output_file>] [--canonfs-root <path>] [--policy <policy_file>]
```

**Load GGUF model:**
```
load-gguf <gguf_file> [--output <output_file>] [--canonfs-root <path>] [--validate]
```

**Load SafeTensors model:**
```
load-safetensors <safetensors_file> [--output <output_file>] [--canonfs-root <path>]
```

**Probe model structure:**
```
probe-model <model_file> [--detailed] [--layer-info] [--output <probe_file>]
```

**Validate model integrity:**
```
validate-model <model_file> [--check-determinism] [--security-scan] [--output <report_file>]
```

**Convert model format:**
```
convert-model <input_file> [--to-format t81w|safetensors|gguf] [--output <output_file>]
```

**Batch model processing:**
```
batch-load <model_directory> [--recursive] [--output-dir <dir>] [--canonfs-root <path>]
```

### Examples

```bash
# Load Hugging Face model with policy
load-hf "bert-base-uncased" --output bert.t81w --policy model_import.apl

# Load GGUF model with validation
load-gguf llama-2-7b.gguf --canonfs-root ~/.t81_canonfs --validate

# Probe model structure
probe-model model.t81w --detailed --layer-info --output model_structure.json

# Validate model integrity and determinism
validate-model weights.t81w --check-determinism --security-scan

# Convert between formats
convert-model model.safetensors --to-format t81w --output model.t81w

# Batch process model directory
batch-load ./models/ --recursive --output-dir ./converted/ --canonfs-root ./canonfs/
```

### Supported Model Formats

**Input Formats:**
- **Hugging Face**: Hub models and local checkpoints
- **SafeTensors**: Native safe tensor format
- **GGUF**: GGUF format (quantized models)
- **PyTorch**: .pth and .pt files
- **TensorFlow**: SavedModel and .pb files
- **ONNX**: .onnx model files

**Output Formats:**
- **T81W**: T81 native ternary weights format
- **SafeTensors**: Universal safe format
- **GGUF**: Compressed quantized format

### Model Conversion Pipeline

**1. Model Analysis:**
- Architecture detection
- Layer structure mapping
- Parameter counting
- Memory requirements

**2. Security Validation:**
- Malware scanning
- Code injection detection
- Signature verification
- Policy compliance checking

**3. Deterministic Conversion:**
- Bit-identical parameter conversion
- Ternary precision optimization
- Deterministic layout generation
- Hash verification

**4. CanonFS Storage:**
- Immutable storage
- Content-addressed hashing
- Complete provenance
- Policy-gated access

### Output Format

**Model Loading Result:**
```json
{
  "status": "success",
  "model_name": "bert-base-uncased",
  "input_format": "huggingface",
  "output_file": "bert.t81w",
  "canonfs_hash": "CanonHash81_ABC123...",
  "timestamp": "2026-04-04T15:30:00Z",
  "model_info": {
    "architecture": "BERT",
    "parameters": 110000000,
    "layers": 12,
    "hidden_size": 768,
    "vocab_size": 30522
  },
  "conversion": {
    "precision_preserved": true,
    "determinism_verified": true,
    "ternary_optimized": true,
    "compression_ratio": 0.85
  },
  "security": {
    "scanned": true,
    "threats_detected": 0,
    "policy_compliant": true,
    "signature_verified": true
  },
  "provenance": {
    "source": "huggingface_hub",
    "import_version": "1.9.0",
    "conversion_hash": "CanonHash81_CONV...",
    "policy_hash": "CanonHash81_POLICY..."
  }
}
```

**Model Probe Result:**
```json
{
  "model_file": "bert.t81w",
  "timestamp": "2026-04-04T15:30:00Z",
  "architecture": "BERT",
  "layers": [
    {
      "name": "embeddings.word_embeddings",
      "type": "embedding",
      "shape": "[30522, 768]",
      "dtype": "ternary_float",
      "parameters": 23440896
    },
    {
      "name": "encoder.layer.0.attention.self.query",
      "type": "linear",
      "shape": "[768, 768]",
      "dtype": "ternary_float",
      "parameters": 589824
    }
  ],
  "total_parameters": 110000000,
  "model_size_mb": 420,
  "ternary_optimized": true,
  "determinism_guaranteed": true
}
```

### Security Features

**Model Scanning:**
- Static code analysis
- Malware detection
- Vulnerability scanning
- Signature verification

**Policy Enforcement:**
- Import permission checking
- Model type validation
- Size and complexity limits
- Source authorization

**Access Control:**
- User authentication
- Role-based permissions
- Audit trail maintenance
- Secure storage

### Determinism Guarantees

**Conversion Determinism:**
- Bit-identical parameter conversion
- Reproducible model loading
- Consistent hash generation
- Deterministic layout

**Execution Determinism:**
- Same model produces same outputs
- Cross-platform consistency
- Temporal stability
- Verifiable inference

### Performance Considerations

**Large Model Handling:**
- Streaming conversion for large models
- Memory-efficient processing
- Progress reporting
- Resume capability

**Optimization Features:**
- Ternary precision optimization
- Memory layout optimization
- Compression opportunities
- Cache-friendly structures

### Error Handling

- **Format Errors**: Unsupported format detection and suggestions
- **Corruption Errors**: Model integrity validation and recovery
- **Security Errors**: Threat detection and quarantine procedures
- **Policy Errors**: Compliance violation explanations

### Integration

This skill wraps T81's model loading and conversion pipeline:
```bash
t81 model load <source> --format <format> --output <file> --canonfs-root <path> --policy <policy>
```
