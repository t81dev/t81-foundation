# RFC-00A5: LLM Backend Adapter Interface (Engine-Agnostic)

Version 0.1 — Standards Track\
Status: Superseded\
Superseded-By: RFC-0032 §4 (Phase 4)\
Author: T81 Foundation Architecture Team\
Applies to: Inference Backends, LLM Integration, Hardware Abstraction\
Updated: 2026-03-15

> **Supersession note (2026-03-15):** The engine-agnostic adapter vision
> described here (llama.cpp, ONNX Runtime, TensorRT backends) was rejected
> during RFC-0032 audit as fundamentally non-deterministic.  The concrete
> realization is `T81VmBackend` — a deterministic-only backend that dispatches
> RFC-0026 AI opcodes (ATTN, QMATMUL, EMBED, WLOAD) through the T81 VM
> interpreter via the Axion `AIHookEngine`.  See
> `include/t81/vm/ai_backend/backend_adapter.hpp` and RFC-0032 §4.

______________________________________________________________________

## Summary

This RFC defines a clean, engine-agnostic adapter interface for LLM inference backends in the T81 ecosystem. It establishes standardized APIs for model loading, inference execution, and resource management while maintaining deterministic execution guarantees and supporting multiple inference engines (llama.cpp, ONNX Runtime, TensorRT, custom implementations).

______________________________________________________________________

## Motivation

The LLM inference landscape is fragmented with multiple competing backends (llama.cpp, ONNX Runtime, TensorRT, custom implementations). Without a standardized interface, T81 would become tightly coupled to specific implementations, limiting flexibility and making it difficult to support new hardware or optimization techniques.

This RFC provides the abstraction layer needed to support multiple inference engines while preserving T81's deterministic principles.

## Proposal

### Technical Details

#### 1. Backend Architecture

**Adapter Layer Design:**
```
T81 AI Interface
    ↓
LLM Backend Adapter (Standardized API)
    ↓
┌─────────────┬─────────────┬─────────────┬─────────────┐
│ llama.cpp   │ ONNX Runtime│ Custom      │ Hardware    │
│ Backend     │ Backend     │ Backend     │ Accelerator │
└─────────────┴─────────────┴─────────────┴─────────────┘
```

**Core Components:**
- **ILlmBackend**: Main backend interface
- **BackendRegistry**: Backend discovery and management
- **ModelLoader**: Unified model loading interface
- **InferenceEngine**: Deterministic inference execution
- **ResourceManager**: Memory and compute resource management

#### 2. Backend Interface Specification

**Primary Backend Interface:**
```cpp
namespace t81::ai::llm {

class ILlmBackend {
public:
    // Backend identification
    virtual std::string backend_name() const = 0;
    virtual std::string version() const = 0;
    virtual std::vector<std::string> supported_formats() const = 0;
    virtual BackendCapabilities capabilities() const = 0;
    
    // Lifecycle management
    virtual bool initialize(const BackendConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual bool is_healthy() const = 0;
    
    // Model operations
    virtual std::unique_ptr<IModel> load_model(
        const ModelLoadRequest& request
    ) = 0;
    
    virtual void unload_model(const std::string& model_id) = 0;
    
    // Inference operations
    virtual std::unique_ptr<IInferenceSession> create_session(
        const std::string& model_id,
        const SessionConfig& config
    ) = 0;
    
    // Resource management
    virtual ResourceInfo get_resource_info() const = 0;
    virtual bool can_allocate(const ResourceRequirements& req) const = 0;
};

}
```

**Backend Capabilities:**
```cpp
struct BackendCapabilities {
    bool supports_deterministic_inference;
    bool supports_streaming;
    bool supports_batch_inference;
    bool supports_gpu_acceleration;
    bool supports_ternary_models;
    
    std::vector<ModelFormat> supported_formats;
    std::vector<DataType> supported_dtypes;
    std::vector<QuantizationScheme> supported_quantization;
    
    PerformanceLimits limits;
};
```

#### 3. Model Interface Specification

**Model Operations Interface:**
```cpp
class IModel {
public:
    virtual ~IModel() = default;
    
    // Model metadata
    virtual std::string model_id() const = 0;
    virtual ModelInfo get_info() const = 0;
    virtual ModelMetrics get_metrics() const = 0;
    
    // Model operations
    virtual bool validate() const = 0;
    virtual size_t memory_footprint() const = 0;
    virtual std::vector<TensorSpec> get_input_specs() const = 0;
    virtual std::vector<TensorSpec> get_output_specs() const = 0;
    
    // Determinism verification
    virtual bool verify_determinism(
        const DeterminismConfig& config
    ) const = 0;
};
```

**Model Loading Request:**
```cpp
struct ModelLoadRequest {
    std::string model_path;
    std::string model_id;  // Optional, auto-generated if empty
    ModelFormat format;
    
    // Loading options
    bool use_mmap = true;
    bool validate_checksums = true;
    bool preload_weights = false;
    
    // Determinism options
    DeterminismConfig determinism;
    
    // Resource constraints
    ResourceConstraints constraints;
    
    // Backend-specific options
    std::map<std::string, std::any> backend_options;
};
```

#### 4. Inference Session Interface

**Session Management:**
```cpp
class IInferenceSession {
public:
    virtual ~IInferenceSession() = default;
    
    // Session lifecycle
    virtual bool initialize(const SessionConfig& config) = 0;
    virtual void cleanup() = 0;
    
    // Inference operations
    virtual InferenceResult inference(
        const InferenceRequest& request
    ) = 0;
    
    virtual void inference_async(
        const InferenceRequest& request,
        std::function<void(const InferenceResult&)> callback
    ) = 0;
    
    // Streaming inference
    virtual void inference_stream(
        const InferenceRequest& request,
        std::function<void(const StreamChunk&)> chunk_callback,
        std::function<void(const InferenceResult&)> completion_callback
    ) = 0;
    
    // Session state
    virtual SessionState get_state() const = 0;
    virtual SessionMetrics get_metrics() const = 0;
};
```

**Deterministic Inference Request:**
```cpp
struct InferenceRequest {
    // Input data
    std::map<std::string, Tensor> inputs;
    
    // Generation parameters
    GenerationParams generation;
    
    // Determinism controls
    uint64_t random_seed;  // Fixed seed for reproducibility
    bool deterministic_mode = true;
    bool validate_determinism = false;
    
    // Performance options
    bool enable_profiling = false;
    bool enable_tracing = false;
    
    // Backend hints
    std::map<std::string, std::any> backend_hints;
};
```

#### 5. Backend Registry and Discovery

**Registry Interface:**
```cpp
class BackendRegistry {
public:
    // Backend registration
    void register_backend(
        std::unique_ptr<ILlmBackend> backend
    );
    
    void unregister_backend(const std::string& name);
    
    // Backend discovery
    std::vector<std::string> list_backends() const;
    ILlmBackend* get_backend(const std::string& name) const;
    ILlmBackend* get_best_backend(
        const ModelLoadRequest& request
    ) const;
    
    // Backend selection
    ILlmBackend* select_backend(
        const BackendSelectionCriteria& criteria
    ) const;
};
```

**Automatic Backend Selection:**
```cpp
struct BackendSelectionCriteria {
    // Required capabilities
    std::vector<ModelFormat> required_formats;
    bool require_deterministic = true;
    bool require_gpu = false;
    
    // Performance requirements
    std::optional<double> max_latency_ms;
    std::optional<double> min_throughput;
    
    // Resource constraints
    std::optional<size_t> max_memory_mb;
    
    // Quality requirements
    std::optional<double> min_accuracy;
    std::vector<QuantizationScheme> allowed_quantization;
};
```

#### 6. Determinism Guarantees

**Deterministic Execution Protocol:**
```cpp
class DeterministicInference {
public:
    // Determinism validation
    static bool validate_determinism(
        IInferenceSession* session,
        const InferenceRequest& request,
        uint32_t num_runs = 100
    );
    
    // Determinism enforcement
    static InferenceResult enforce_determinism(
        IInferenceSession* session,
        const InferenceRequest& request
    );
    
    // Determinism monitoring
    static DeterminismReport monitor_determinism(
        IInferenceSession* session,
        const InferenceRequest& request,
        std::function<void(const DeterminismViolation&)> violation_handler
    );
};
```

**Determinism Violation Handling:**
```cpp
enum class DeterminismViolation {
    NONE,
    OUTPUT_MISMATCH,
    TIMING_VARIATION,
    MEMORY_DIFFERENCE,
    BACKEND_NON_DETERMINISM
};

struct DeterminismReport {
    bool is_deterministic;
    uint32_t consistent_runs;
    uint32_t total_runs;
    std::vector<DeterminismViolation> violations;
    double variance_score;
    std::string backend_name;
};
```

#### 7. Reference Implementations

**llama.cpp Backend Adapter:**
```cpp
class LlamaCppBackend : public ILlmBackend {
public:
    std::string backend_name() const override {
        return "llama.cpp";
    }
    
    std::unique_ptr<IModel> load_model(
        const ModelLoadRequest& request
    ) override {
        // Convert T81 request to llama.cpp parameters
        llama_model_params params = convert_to_llama_params(request);
        
        // Load model with deterministic settings
        llama_model* model = llama_load_model_from_file(
            request.model_path.c_str(),
            params
        );
        
        if (!model) {
            throw std::runtime_error("Failed to load llama.cpp model");
        }
        
        return std::make_unique<LlamaCppModel>(model, request);
    }
    
    BackendCapabilities capabilities() const override {
        BackendCapabilities caps;
        caps.supports_deterministic_inference = true;
        caps.supports_ternary_models = false;  // Requires conversion
        caps.supported_formats = {ModelFormat::GGUF};
        caps.supported_dtypes = {DataType::F32, DataType::F16, DataType::Q8_0};
        return caps;
    }
};
```

### Corner Cases

#### Backend Failures
- Graceful fallback to alternative backends
- Automatic backend health monitoring
- Session recovery and restart capabilities

#### Resource Exhaustion
- Predictive resource allocation
- Dynamic session prioritization
- Memory pressure handling

#### Format Incompatibility
- Automatic format conversion when possible
- Clear error messages for unsupported formats
- Format capability negotiation

## Impact

### Backward Compatibility

No impact on existing code. New backend system is opt-in and provides migration path.

### Performance

Adapter layer adds minimal overhead:
- Function call overhead: ~1-5μs per call
- Memory overhead: ~10-50MB per backend
- Latency impact: <2% for typical inference workloads

### Security

Enhanced security through:
- Sandboxed backend execution
- Resource usage limits
- Determinism verification prevents manipulation
- Backend capability validation

## Alternatives Considered

1. **Direct llama.cpp integration**: Rejected due to vendor lock-in
2. **Multiple separate APIs**: Rejected due to complexity
3. **Runtime backend switching**: Rejected due to determinism concerns

## UX / Developer Experience Impact

### CLI Interface

```bash
# List available backends
t81 ai backend list

# Test backend capabilities
t81 ai backend test llama.cpp --model model.gguf

# Run inference with specific backend
t81 ai inference run \
  --backend llama.cpp \
  --model model.gguf \
  --prompt "Hello, world!" \
  --deterministic

# Compare backend performance
t81 ai backend benchmark \
  --backends llama.cpp,onnx \
  --model model.gguf \
  --metrics latency,throughput,memory

# Validate backend determinism
t81 ai backend verify-determinism \
  --backend llama.cpp \
  --model model.gguf \
  --runs 100
```

### IDE Integration

- Backend capability visualization
- Automatic backend selection based on constraints
- Performance comparison tools
- Determinism validation dashboard

### Development Workflow

- Backend development kit and templates
- Automated backend testing framework
- Performance profiling and optimization
- Determinism verification tools

## Acceptance Criteria

1. Backend interface supports llama.cpp and at least one other backend
2. Determinism verification works across all supported backends
3. Performance overhead within specified limits
4. Resource management prevents system overload
5. Backend registry provides automatic selection

## Promotion Gates

### Experimental → Extension
- [ ] llama.cpp backend fully implemented
- [ ] At least one additional backend supported
- [ ] Cross-backend determinism validated
- [ ] Performance benchmarks meet targets

### Extension → Core
- [ ] Adopted as standard inference interface
- [ ] All official AI features use adapter interface
- [ ] Community contribution of additional backends
- [ ] Hardware accelerator support implemented

## Impact

### Backward Compatibility

No impact on existing code. New backend system is opt-in and provides migration path.

### Performance

Adapter layer adds minimal overhead:
- Function call overhead: ~1-5μs per call
- Memory overhead: ~10-50MB per backend
- Latency impact: <2% for typical inference workloads

### Security

Enhanced security through:
- Sandboxed backend execution
- Resource usage limits
- Determinism verification prevents manipulation
- Backend capability validation

______________________________________________________________________

## Alternatives Considered

1. **Direct llama.cpp integration**: Rejected due to vendor lock-in
2. **Multiple separate APIs**: Rejected due to complexity
3. **Runtime backend switching**: Rejected due to determinism concerns

______________________________________________________________________

## References

- [Model Artifact Provenance](RFC-00A3-model-artifact-provenance.md)
- [Deterministic Evidence Protocol](RFC-00A1-deterministic-evidence-protocol.md)
- [Ternary Quantization Codec](RFC-00A4-ternary-quantization-codec.md)
- [llama.cpp Documentation](https://github.com/ggerganov/llama.cpp)
