# RFC-00A8: AI-Native VM Opcode Exploration (QMATMUL/ATTN/EMBED…)

Version 0.1 — Standards Track
Status: Superseded
Superseded-By: RFC-0026
Author: T81 Foundation Architecture Team
Applies to: T81VM, TISC Instruction Set, Hardware Acceleration
Updated: 2026-03-15

> **Supersession note (2026-03-15):** The AI-native opcode surface explored
> here (QMATMUL, ATTN, EMBED, and related ops) was formally specified and
> implemented under RFC-0026 (accepted).  ATTN, QMATMUL, EMBED, and WLOAD are
> live in the TISC ISA (`include/t81/isa/ai_native_opcodes.hpp`) and dispatched
> through the T81 VM (`vm/vm.cpp`).  Phase-1 conformance evidence is in
> `docs/architecture/ai-opcode-phase1-conformance.md` (status: spec_conformant).

______________________________________________________________________

## Summary

This RFC explores design and implementation of AI-native VM opcodes for T81 Virtual Machine. It defines specialized instructions for common AI operations like quantized matrix multiplication, attention mechanisms, and embedding lookups, enabling efficient AI computation while maintaining deterministic execution guarantees.

______________________________________________________________________

## Motivation

Current AI inference relies on software libraries running on general-purpose hardware. By introducing AI-native opcodes, T81 can:
- Optimize AI operations at the VM level
- Leverage ternary computing advantages for AI workloads
- Maintain deterministic execution for AI operations
- Enable hardware acceleration through specialized instructions

This RFC represents the highest architectural impact but also the greatest potential performance benefits for AI workloads in T81.

## Proposal

### Technical Details

#### 1. AI Opcode Classification

**Opcode Categories:**
- **Linear Algebra**: Matrix operations, vector computations
- **Attention**: Scaled dot-product attention variants
- **Embedding**: Lookup and interpolation operations
- **Activation**: Non-linear functions and normalization
- **Reduction**: Pooling, aggregation, and statistical operations
- **Memory**: Tensor manipulation and layout operations

**Determinism Levels:**
- **Level 1**: Fully deterministic (required for core operations)
- **Level 2**: Deterministic with configuration (configurable precision)
- **Level 3**: Statistically deterministic (within tolerance)

#### 2. Core AI Opcode Specifications

**Quantized Matrix Multiplication (QMATMUL):**
```c
// TISC Instruction: QMATMUL
// Opcode: 0x60
// Format: QMATMUL rd, rs1, rs2, config

struct QMatmulConfig {
    uint8_t precision;      // T3_K1, T3_K2, T3_K3
    uint8_t accumulator_bits; // 16, 32, 64 bits
    uint8_t rounding_mode;   // Round to nearest, floor, ceil
    uint8_t bias_mode;       // None, per-channel, per-tensor
    bool deterministic;      // Force deterministic execution
};

// Execution semantics:
// R[rd] = quantized_matmul(R[rs1], R[rs2], config)
// Where R[rs1] and R[rs2] contain ternary-quantized tensors
```

**Scaled Dot-Product Attention (ATTN):**
```c
// TISC Instruction: ATTN
// Opcode: 0x61
// Format: ATTN rd, rs1, rs2, rs3, config

struct AttentionConfig {
    uint8_t head_dim;        // Dimension per attention head
    uint8_t num_heads;       // Number of attention heads
    uint8_t scale_type;      // Fixed, learned, adaptive
    uint8_t mask_type;       // None, causal, padding
    bool use_flash_attention; // Memory-efficient variant
    float temperature;       // Softmax temperature
};

// Execution semantics:
// R[rd] = attention(R[rs1], R[rs2], R[rs3], config)
// Where rs1=queries, rs2=keys, rs3=values
```

**Embedding Lookup (EMBED):**
```c
// TISC Instruction: EMBED
// Opcode: 0x62
// Format: EMBED rd, rs1, rs2, config

struct EmbeddingConfig {
    uint8_t embedding_dim;   // Dimension of embedding vectors
    uint8_t lookup_mode;     // Exact, interpolated, hashed
    uint8_t padding_mode;    // Zero, random, learned
    bool use_cache;          // Cache frequent lookups
    bool deterministic;       // Force deterministic lookup
};

// Execution semantics:
// R[rd] = embedding_lookup(R[rs1], R[rs2], config)
// Where rs1=indices, rs2=embedding_table
```

#### 3. Ternary-Specific Optimizations

**Ternary Matrix Multiplication:**
```cpp
// Optimized ternary matrix multiplication
class TernaryMatmul {
public:
    // Core operation using base-81 packing
    static void execute(
        const TernaryTensor& A,
        const TernaryTensor& B,
        Tensor<int32_t>& C,
        const QMatmulConfig& config
    ) {
        // Convert ternary to balanced trits
        auto trits_A = convert_to_trits(A);
        auto trits_B = convert_to_trits(B);
        
        // Use base-81 multiplication table
        for (size_t i = 0; i < A.rows(); ++i) {
            for (size_t j = 0; j < B.cols(); ++j) {
                int32_t sum = 0;
                for (size_t k = 0; k < A.cols(); k += 4) {
                    // Pack 4 trits from each matrix
                    uint8_t pack_A = pack_trits(&trits_A[i*A.cols() + k]);
                    uint8_t pack_B = pack_trits(&trits_B[k*B.cols() + j]);
                    
                    // Lookup multiplication result
                    sum += lookup_ternary_mult(pack_A, pack_B);
                }
                C(i, j) = sum;
            }
        }
    }
    
private:
    // Precomputed multiplication table for base-81 digits
    static int32_t lookup_ternary_mult(uint8_t a, uint8_t b);
    static uint8_t pack_trits(const Trit* trits);
};
```

**Deterministic Attention Implementation:**
```cpp
class DeterministicAttention {
public:
    static void execute(
        const Tensor<float>& queries,
        const Tensor<float>& keys,
        const Tensor<float>& values,
        Tensor<float>& output,
        const AttentionConfig& config
    ) {
        // Compute scaled dot-product attention deterministically
        
        // 1. Compute attention scores
        Tensor<float> scores(queries.shape[0], keys.shape[0]);
        for (size_t i = 0; i < queries.shape[0]; ++i) {
            for (size_t j = 0; j < keys.shape[0]; ++j) {
                float score = 0.0f;
                for (size_t k = 0; k < queries.shape[1]; ++k) {
                    score += queries(i, k) * keys(j, k);
                }
                scores(i, j) = score * config.temperature;
            }
        }
        
        // 2. Apply softmax with deterministic implementation
        Tensor<float> attention_weights(scores.shape);
        deterministic_softmax(scores, attention_weights);
        
        // 3. Apply attention to values
        for (size_t i = 0; i < queries.shape[0]; ++i) {
            for (size_t k = 0; k < values.shape[1]; ++k) {
                float sum = 0.0f;
                for (size_t j = 0; j < keys.shape[0]; ++j) {
                    sum += attention_weights(i, j) * values(j, k);
                }
                output(i, k) = sum;
            }
        }
    }
    
private:
    static void deterministic_softmax(
        const Tensor<float>& input,
        Tensor<float>& output
    );
};
```

#### 4. Memory Layout and Optimization

**Ternary Tensor Memory Layout:**
```c
// Optimized memory layout for ternary tensors
struct TernaryTensorLayout {
    // Base-81 packed trits (4 trits per byte)
    uint8_t* packed_data;
    
    // Metadata for efficient access
    uint64_t num_elements;
    uint8_t bits_per_element;
    uint8_t padding_bits;
    
    // Stride information for multi-dimensional access
    uint64_t strides[MAX_TENSOR_RANK];
    
    // Cache alignment for SIMD operations
    uint8_t alignment;
};

// Memory access patterns optimized for ternary operations
class TernaryMemoryAccessor {
public:
    // Sequential access for matrix multiplication
    static void iterate_rows_sequential(
        const TernaryTensorLayout& layout,
        size_t row,
        std::function<void(uint8_t)> callback
    );
    
    // Random access for embedding lookups
    static uint8_t get_element(
        const TernaryTensorLayout& layout,
        const uint64_t* indices
    );
    
    // Batch access for attention operations
    static void get_batch(
        const TernaryTensorLayout& layout,
        const uint64_t* indices,
        size_t batch_size,
        uint8_t* output
    );
};
```

#### 5. Hardware Acceleration Interface

**Accelerator Abstraction:**
```cpp
class AIAccelerator {
public:
    virtual ~AIAccelerator() = default;
    
    // Capability detection
    virtual bool supports_opcode(TISCOpcode opcode) const = 0;
    virtual bool supports_ternary_ops() const = 0;
    virtual size_t max_tensor_size() const = 0;
    
    // Opcode execution
    virtual bool execute_qmatmul(
        const TernaryTensor& A,
        const TernaryTensor& B,
        Tensor<int32_t>& C,
        const QMatmulConfig& config
    ) = 0;
    
    virtual bool execute_attention(
        const Tensor<float>& queries,
        const Tensor<float>& keys,
        const Tensor<float>& values,
        Tensor<float>& output,
        const AttentionConfig& config
    ) = 0;
    
    // Memory management
    virtual bool allocate_tensor(
        const TensorSpec& spec,
        TensorHandle& handle
    ) = 0;
    
    virtual void deallocate_tensor(TensorHandle handle) = 0;
};
```

#### 6. Determinism Guarantees

**Deterministic Execution Protocol:**
```cpp
class DeterministicAIExecution {
public:
    // Validate opcode determinism
    static bool validate_opcode_determinism(
        TISCOpcode opcode,
        const std::vector<Tensor>& inputs,
        uint32_t test_runs = 100
    ) {
        std::vector<Tensor> results(test_runs);
        
        for (uint32_t i = 0; i < test_runs; ++i) {
            // Execute with identical inputs
            execute_opcode(opcode, inputs, results[i]);
        }
        
        // Verify all results are identical
        for (uint32_t i = 1; i < test_runs; ++i) {
            if (!tensors_equal(results[0], results[i])) {
                return false;
            }
        }
        
        return true;
    }
    
    // Enforce deterministic execution
    static void enforce_determinism(
        TISCOpcode opcode,
        const std::vector<Tensor>& inputs,
        Tensor& output
    ) {
        // Set deterministic flags
        set_deterministic_mode(true);
        set_random_seed(42);  // Fixed seed for reproducibility
        
        // Execute with deterministic settings
        execute_opcode(opcode, inputs, output);
        
        // Verify result consistency
        if (!validate_result_consistency(output)) {
            throw DeterminismViolation("Non-deterministic result detected");
        }
    }
};
```

#### 7. Integration with T81VM

**VM Extension Points:**
```cpp
// Extend T81VM with AI opcodes
class T81VM_AI : public T81VM {
public:
    // Register AI opcodes
    void register_ai_opcodes() {
        register_opcode(0x60, std::bind(&T81VM_AI::execute_qmatmul, this));
        register_opcode(0x61, std::bind(&T81VM_AI::execute_attention, this));
        register_opcode(0x62, std::bind(&T81VM_AI::execute_embedding, this));
        register_opcode(0x63, std::bind(&T81VM_AI::execute_activation, this));
        // ... more opcodes
    }
    
    // AI opcode implementations
    void execute_qmatmul() {
        // Decode operands
        uint8_t rd = decode_register();
        uint8_t rs1 = decode_register();
        uint8_t rs2 = decode_register();
        QMatmulConfig config = decode_config();
        
        // Execute with determinism enforcement
        DeterministicAIExecution::enforce_determinism(
            TISCOpcode::QMATMUL,
            {get_register(rs1), get_register(rs2)},
            get_register(rd)
        );
    }
    
private:
    std::unique_ptr<AIAccelerator> accelerator_;
    bool deterministic_mode_ = true;
};
```

### Corner Cases

#### Precision Limitations
- Overflow handling in accumulator operations
- Underflow detection in attention computations
- Rounding mode consistency across platforms

#### Memory Constraints
- Out-of-memory handling for large tensors
- Memory fragmentation in tensor allocation
- Cache coherence issues

#### Hardware Variations
- Fallback to software implementation
- Performance degradation on unsupported hardware
- Feature detection and capability negotiation

## Impact

### Backward Compatibility

No impact on existing T81 programs. AI opcodes are additive and optional.

### Performance

Potential performance improvements:
- QMATMUL: 5-10x speedup for ternary matrices
- ATTN: 3-5x speedup for attention mechanisms
- EMBED: 10-20x speedup for embedding lookups

### Security

Enhanced security through:
- Deterministic AI operations prevent manipulation
- Hardware isolation for sensitive computations
- Auditable AI execution traces

## Alternatives Considered

1. **Library-based approach**: Rejected due to performance overhead
2. **JIT compilation**: Rejected due to determinism concerns
3. **External accelerator**: Rejected due to integration complexity

## UX / Developer Experience Impact

### Development Tools

```bash
# Test AI opcode performance
t81 ai opcode benchmark --opcode QMATMUL --size 1024x1024

# Validate opcode determinism
t81 ai opcode verify-determinism --opcode ATTN --runs 1000

# Profile AI operations
t81 ai opcode profile --program model.tisc --output profile.json

# Optimize for specific hardware
t81 ai opcode optimize --target hardware --program model.tisc
```

### Compiler Integration

```t81
// T81Lang with AI opcode intrinsics
fn ternary_matmul(a: TernaryTensor, b: TernaryTensor) -> Tensor {
    // Compiler generates QMATMUL opcode
    return __builtin_qmatmul(a, b, QMatmulConfig::T3_K2);
}

fn attention(q: Tensor, k: Tensor, v: Tensor) -> Tensor {
    // Compiler generates ATTN opcode
    return __builtin_attention(q, k, v, AttentionConfig::default());
}
```

## Acceptance Criteria

1. Core AI opcodes (QMATMUL, ATTN, EMBED) implemented
2. Determinism validation passes across platforms
3. Performance improvements demonstrated vs software implementation
4. Hardware acceleration interface functional
5. Integration with T81VM complete

## Promotion Gates

### Experimental → Extension
- [ ] Basic AI opcodes implemented in VM
- [ ] Determinism validation working
- [ ] Performance benchmarks show improvement
- [ ] Hardware abstraction layer functional

### Extension → Core
- [ ] Comprehensive AI opcode set implemented
- [ ] Hardware acceleration support mature
- [ ] Community adoption and optimization
- [ ] Formal verification of opcode semantics

## Impact

### Backward Compatibility

No impact on existing T81 programs. AI opcodes are additive and optional.

### Performance

Potential performance improvements:
- QMATMUL: 5-10x speedup for ternary matrices
- ATTN: 3-5x speedup for attention mechanisms
- EMBED: 10-20x speedup for embedding lookups

### Security

Enhanced security through:
- Deterministic AI operations prevent manipulation
- Hardware isolation for sensitive computations
- Auditable AI execution traces

______________________________________________________________________

## Alternatives Considered

1. **Library-based approach**: Rejected due to performance overhead
2. **JIT compilation**: Rejected due to determinism concerns
3. **External accelerator**: Rejected due to integration complexity

______________________________________________________________________

## References

- [TISC Specification](../tisc-spec.md)
- [T81VM Specification](../t81vm-spec.md)
- [Ternary Quantization Codec](RFC-00A4-ternary-quantization-codec.md)
- [Deterministic Evidence Protocol](RFC-00A1-deterministic-evidence-protocol.md)
