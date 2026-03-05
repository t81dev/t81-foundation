# T81 Model Provenance System - RFC-00A3 Task 5

This directory contains the model artifact identity, provenance, and integrity verification system for the T81 ecosystem.

## Components

### Model Manager (`t81_ai_model`)
CLI tool for managing model artifacts with cryptographic verification and CanonFS integration.

**Usage:**
```bash
# Load model with verification
./t81_ai_model load model.gguf expected_hash

# Create model manifest
./t81_ai_model manifest model.gguf

# Verify model integrity
./t81_ai_model verify model_12345

# List all registered models
./t81_ai_model list
```

## Model Formats Supported

### GGUF (GPT-Generated Unified Format)
- **Description**: Popular format for LLM models
- **Conversion**: Automatic conversion to T81 canonical format
- **Verification**: Hash-based integrity checking

### Safetensors
- **Description**: Safe tensor format for PyTorch models
- **Conversion**: Automatic conversion to T81 canonical format
- **Verification**: Cryptographic signature support

### T81 Canonical Format
- **Description**: Native T81 format with ternary optimization
- **Features**: Content-addressed storage, built-in integrity checks
- **Benefits**: Optimal for T81 VM execution

## Provenance Features

### Model Identity
- **Unique IDs**: Auto-generated model identifiers
- **Metadata Tracking**: Complete model lifecycle tracking
- **Version Management**: Semantic versioning support
- **Dependency Management**: Model dependency tracking

### Cryptographic Verification
- **SHA-256 Hashing**: Cryptographic integrity verification
- **Digital Signatures**: PKI-based model authentication
- **Key Management**: Secure key generation and storage
- **Tamper Detection**: Automatic tamper evidence

### CanonFS Integration
- **Content-Addressed Storage**: Hash-based file storage
- **Automatic Conversion**: Format conversion to canonical form
- **Audit Trail**: Complete access and modification logging
- **Space Efficiency**: Deduplication through content addressing

## Security Features

### Model Signing
- **Private Key Signing**: Secure model signing
- **Public Key Verification**: Signature validation
- **Key Rotation**: Support for key updates
- **Revocation**: Compromised key handling

### Access Control
- **Policy Integration**: Axion policy system integration
- **Role-Based Access**: User permission management
- **Audit Logging**: Complete access tracking
- **Tamper Evidence**: Automatic tamper detection

## Model Manifest Format

```json
{
  "model_id": "model_1642345678900",
  "name": "my_model",
  "version": "1.0.0",
  "format": "gguf",
  "creator": "T81 AI System",
  "created_timestamp": "2026-03-05 01:00:00",
  "description": "Example model for testing",
  "parameters": {
    "layers": 12,
    "parameters": "110M",
    "context_size": 2048
  },
  "model_hash": "sha256_hash_here",
  "signature": "digital_signature_here",
  "security_tags": ["experimental", "verified"],
  "dependencies": ["tokenizer_v1", "config_v2"]
}
```

## CanonFS Storage Structure

```
canonfs/
├── models/
│   ├── registry.json              # Model registry
│   ├── model_12345.t81          # Canonical format models
│   ├── model_67890.t81
│   └── ...
├── keys/
│   ├── private.pem                 # Model signing key
│   └── public.pem                  # Model verification key
└── audit/
    ├── access_log.json            # Access tracking
    └── modification_log.json       # Change tracking
```

## Acceptance Criteria

- [x] Model manifest format supports all required metadata
- [x] Conversion pipeline works with GGUF and Safetensors
- [x] CanonFS integration provides secure storage
- [x] Verification procedures detect tampering
- [x] Audit trail captures all model operations

## Build Instructions

```bash
# Enable AI experiments
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
make t81_ai_model

# Initialize CanonFS storage
make init_canonfs_model_storage

# Run model manager
./build/experiments/ai/bin/t81_ai_model load model.gguf
```

## Security Considerations

- **Key Management**: Private keys must be protected and backed up
- **Signature Verification**: Always verify model signatures before use
- **Audit Regularly**: Regular audit of model access and modifications
- **Revocation**: Immediate revocation of compromised models
- **Access Control**: Implement proper access controls for model operations

---

**RFC Reference**: RFC-00A3  
**Task**: 5 - Implement model provenance system  
**Status**: Completed  
**Last Updated**: 2026-03-05
