# T81 LLM Backend Adapter - RFC-00A5 Task 7

This directory contains the engine-agnostic adapter interface for LLM inference backends with deterministic execution guarantees.

## Components

### Backend Adapter (`t81_ai_backend`)
CLI tool for managing multiple LLM inference backends with unified interface and deterministic enforcement.

**Usage:**
```bash
# Register backends
./t81_ai_backend register llama_cpp llama_cpp
./t81_ai_backend register onnx onnx_runtime

# List available backends
./t81_ai_backend list

# Activate specific backend
./t81_ai_backend activate llama_cpp

# Show backend capabilities
./t81_ai_backend capabilities llama_cpp

# Run inference
./t81_ai_backend infer model.gguf "Hello, world!" --max_tokens 100

# Set inference mode
./t81_ai_backend mode strict_deterministic
```

## Supported Backends

### Llama.cpp
- **Description**: Popular C++ implementation of LLaMA models
- **Formats**: GGUF, T81 canonical
- **Features**: Deterministic inference, quantization support, streaming
- **Performance**: CPU-optimized, good for edge deployment

### ONNX Runtime
- **Description**: Cross-platform inference engine for neural networks
- **Formats**: ONNX, T81 canonical
- **Features**: Hardware acceleration, batch inference, dynamic shapes
- **Performance**: GPU-optimized, good for data center

### TensorRT
- **Description**: NVIDIA's high-performance inference optimizer
- **Formats**: TensorRT, ONNX, T81 canonical
- **Features**: Maximum performance, precision calibration
- **Performance**: GPU-optimized, best for production

### Custom Backend
- **Description**: User-defined inference implementations
- **Formats**: Any custom format
- **Features**: Full flexibility, specialized optimizations
- **Performance**: Variable, depends on implementation

## Inference Modes

### Strict Deterministic
- **Requirement**: Bit-exact reproducibility across all executions
- **Use Case**: Critical applications requiring perfect reproducibility
- **Validation**: Hash-based verification of input/output consistency

### Statistical Deterministic
- **Requirement**: Results within statistical tolerance bounds
- **Use Case**: Applications tolerant of minor variations
- **Validation**: Variance analysis with configurable tolerance

### Reproducible Non-Deterministic
- **Requirement**: Documented randomness with proper seeding
- **Use Case**: AI applications requiring controlled randomness
- **Validation**: Seed verification and reproducibility checks

## API Interface

### IInferenceBackend Interface
```cpp
class IInferenceBackend {
public:
    virtual bool initialize(const ModelInfo& model_info) = 0;
    virtual bool load_model(const std::string& model_path) = 0;
    virtual InferenceResult inference(const InferenceRequest& request) = 0;
    virtual BackendCapabilities get_capabilities() const = 0;
    virtual void cleanup() = 0;
    virtual std::string get_backend_name() const = 0;
};
```

### BackendManager
```cpp
class BackendManager {
public:
    void register_backend(const std::string& name, std::unique_ptr<IInferenceBackend> backend);
    bool set_active_backend(const std::string& name);
    InferenceResult inference(const ModelInfo& model_info, const InferenceRequest& request);
    std::vector<std::string> list_backends() const;
    BackendCapabilities get_backend_capabilities(const std::string& name) const;
    void set_inference_mode(InferenceMode mode);
    void cleanup_all();
};
```

## Determinism Enforcement

### Strict Protocol
- **Fixed Seeds**: Random number generators use fixed seeds
- **Deterministic Operations**: All operations produce identical results
- **Hash Validation**: Input/output hash consistency verification
- **Cross-Platform**: Identical results across all supported platforms

### Resource Management
- **Memory Limits**: Configurable memory usage limits per backend
- **CPU Throttling**: Prevent system overload
- **Concurrent Requests**: Limited concurrent inference sessions
- **Cleanup**: Automatic resource cleanup on completion

## Performance Optimization

### Backend Selection
- **Automatic Selection**: Choose optimal backend based on model and hardware
- **Capability Matching**: Match model requirements with backend features
- **Fallback Support**: Graceful fallback to alternative backends
- **Load Balancing**: Distribute inference across multiple backends

### Metrics Collection
- **Inference Time**: Millisecond-precision timing
- **Throughput**: Tokens per second measurement
- **Memory Usage**: Peak and average memory consumption
- **Model Loading**: Model loading and initialization time
- **Error Rates**: Failure and timeout statistics

## Acceptance Criteria

- [x] llama.cpp backend fully implemented
- [x] At least one additional backend supported
- [x] Cross-backend determinism validated
- [x] Performance benchmarks meet targets (<2% overhead)
- [x] Resource management prevents system overload

## Build Instructions

```bash
# Enable AI experiments
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
make t81_ai_backend

# Run backend adapter
./build/experiments/ai/bin/t81_ai_backend register llama_cpp llama_cpp
./build/experiments/ai/bin/t81_ai_backend activate llama_cpp
./build/experiments/ai/bin/t81_ai_backend infer model.gguf "Hello, world!"
```

## Integration with T81 Ecosystem

The backend adapter integrates with:
- **Model Provenance** (RFC-00A3) for secure model loading
- **Quantization Codecs** (RFC-00A4) for compressed model support
- **Determinism Framework** (RFC-00A1) for reproducible inference
- **Policy Hooks** (RFC-00A6) for security enforcement
- **CLI Tools** (RFC-00A7) for unified command interface

---

**RFC Reference**: RFC-00A5  
**Task**: 7 - Build engine-agnostic LLM backend system  
**Status**: Completed  
**Last Updated**: 2026-03-05
