# Controlled Stochastic Inference (CSI) Subsystem

## Overview

**Experimental Frontier → AI → Controlled Stochastic Inference**

The CSI subsystem provides **policy-governed stochastic execution** within T81's Experimental Frontier. It implements the missing formal layer for **accountable uncertainty** - where even stochastic behavior is governed, traceable, and reproducible.

## Architecture Position

```
Layer 0: Deterministic Substrate (DCP) - Production Truth
├── TISC ISA (v1.9.0 Frozen)
├── T81VM (deterministic interpreter)
├── Axion (policy governance)
├── CanonFS (immutable storage)
└── T81Lang (deterministic compilation)

Layer 1: Governed Stochastic Processes (CSI) - Accountable Uncertainty
├── Controlled Stochastic Inference ← THIS SUBSYSTEM
├── Policy-gated sampling with provenance
└── Seed-managed replayability

Layer 2: Unbounded External AI - Research Boundary
└── External model integration (research only)
```

## Core Components

### 1. Stochastic Decoder (`stochastic_decoder.cpp`)
**Purpose:** Core stochastic inference with policy integration

**Key Features:**
- Deterministic envelope (seed-required execution)
- Temperature-based sampling with policy bounds
- Top-k and nucleus sampling methods
- Complete provenance capture for each step

**Classes:**
- `ControlledStochasticDecoder` - Main decoder implementation
- `StochasticConfig` - Configuration with policy constraints
- `StochasticResult` - Result with provenance data
- `StochasticPolicyContext` - Policy evaluation context

### 2. Policy-Gated Sampling (`policy_gated_sampling.cpp`)
**Purpose:** Advanced sampling methods with real-time policy enforcement

**Key Features:**
- Multiple sampling methods (greedy, top-k, nucleus, temperature, beam search)
- Real-time policy constraint application
- Entropy escalation detection
- Forbidden token filtering
- Confidence threshold enforcement

**Classes:**
- `PolicyGatedSampler` - Main sampling engine
- `StochasticPolicyEvaluator` - Advanced policy evaluation
- `SamplingConfig` - Configuration with safety constraints
- `SamplingPolicyResult` - Policy evaluation results

### 3. Stochastic Provenance (`stochastic_provenance.cpp`)
**Purpose:** Immutable provenance chains for stochastic operations

**Key Features:**
- Complete stochastic step recording
- CanonFS integration for immutable storage
- Chain integrity verification
- Cross-system portability of stochastic decisions
- JSON and binary serialization

**Classes:**
- `StochasticProvenanceChain` - Complete provenance chain
- `StochasticStep` - Individual step recording
- `StochasticChainManager` - Multi-chain management

## Policy Integration

### Axion Policy Extensions

CSI extends Axion with new policy types:

```axl
# Stochastic Inference Policy
policy "stochastic.inference.sampling" {
    description: "Control stochastic sampling behavior",
    version: "1.0",
    
    # Core constraints
    max_entropy_per_token: 2.5,
    max_candidate_set_size: 10,
    require_explicit_consent: true,
    
    # Sampling parameters
    default_temperature: 1.0,
    default_top_k: 5,
    default_top_p: 0.9,
    
    # Provenance requirements
    capture_logits: true,
    capture_candidates: true,
    capture_selection_trace: true,
    
    # Safety constraints
    forbid_repetition: false,
    diversity_penalty: 0.0,
    forbidden_tokens: []
}
```

### Real-time Policy Enforcement

Every stochastic operation passes through Axion:

1. **Pre-sampling validation** - Check entropy bounds, candidate set size
2. **Dynamic constraint application** - Adjust parameters based on policy
3. **Post-selection verification** - Validate selected token against policy
4. **Provenance capture** - Record policy decision and reasoning

## Usage Examples

### Basic Stochastic Inference

```cpp
#include "experimental/ai/csi/stochastic_decoder.hpp"

// Create policy gate and decoder
AxionPolicyGate policy_gate;
auto decoder = t81::experimental::csi::create_stochastic_decoder(
    policy_gate, "inference.context");

// Configure stochastic sampling
t81::experimental::csi::StochasticConfig config;
config.seed = 12345;
config.temperature = 1.0;
config.top_k = 5;
config.max_entropy_per_token = 2.5;

// Run stochastic inference
auto result = decoder->decode_with_policy(tokens, logits, config);

if (result.policy_verdict == AxionVerdict::ALLOW) {
    std::cout << "Selected: " << result.selected_token << std::endl;
    std::cout << "Entropy: " << result.entropy << std::endl;
    std::cout << "Provenance: " << result.provenance_hash.to_string() << std::endl;
}
```

### Advanced Policy-Gated Sampling

```cpp
#include "experimental/ai/csi/policy_gated_sampling.hpp"

// Create policy-gated sampler
auto sampler = t81::experimental::csi::create_policy_gated_sampler(12345);

// Configure sampling with constraints
t81::experimental::csi::SamplingConfig config;
config.method = t81::experimental::csi::STOCHASTIC_TOP_K;
config.temperature = 0.8;
config.top_k = 5;
config.max_entropy_per_token = 2.0;
config.forbidden_tokens = {"[UNK]", "[PAD]"};

// Sample with policy enforcement
auto result = sampler->sample(tokens, logits, config, "generation.context");

if (result.success) {
    std::cout << "Sampled: " << result.selected_token << std::endl;
    std::cout << "Method: " << result.method << std::endl;
    std::cout << "Policy: " << t81::experimental::csi::axion_verdict_to_string(result.policy_verdict) << std::endl;
}
```

### Provenance Chain Management

```cpp
#include "experimental/ai/csi/stochastic_provenance.hpp"

// Create provenance chain
t81::experimental::csi::StochasticConfig config;
config.seed = 12345;

auto chain = std::make_unique<t81::experimental::csi::StochasticProvenanceChain>(
    "tinyllama", "model_hash_abc123", config);

// Add stochastic steps
for (uint32_t step = 0; step < sequence_length; ++step) {
    auto result = decoder->decode_with_policy(tokens, logits, config);
    chain->append_step(result, step, input_hash);
}

// Finalize and store chain
CanonFSHash chain_hash = chain->finalize_chain();
CanonFSHash stored_hash = chain->store_to_canonfs(storage);

// Verify chain integrity later
bool is_valid = chain->verify_chain_integrity(stored_hash);
```

## CLI Integration

### Stochastic Inference Command

```bash
# Run with controlled stochastic inference
./build/t81 ai inference run \
  --model tinyllama \
  --model-file /tmp/tinyllama.gguf \
  --mode controlled_stochastic \
  --seed 12345 \
  --stochastic-config top_k=5,temperature=1.0,max_entropy=2.5 \
  --prompt "What is the capital of France?" \
  --policy experimental/stochastic_sampling.apl \
  --out /tmp/stochastic_result.json
```

### Provenance Verification

```bash
# Verify stochastic provenance chain
./build/t81 canonfs verify-stochastic-chain \
  --chain-hash <hash_from_result> \
  --canonfs-root /tmp/canonfs \
  --policy verification_policy.apl
```

### Chain Analysis

```bash
# Analyze stochastic chain properties
./build/t81 csi analyze-chain \
  --chain-hash <hash> \
  --metrics entropy,violations,consistency \
  --out /tmp/chain_analysis.json
```

## Determinism Guarantees

### Replayable Stochasticity

```bash
# Same seed = identical stochastic path
./build/t81 ai inference run --seed 12345 ...  # Run 1
./build/t81 ai inference run --seed 12345 ...  # Run 2 (identical)

# Different seed = different path, same structure
./build/t81 ai inference run --seed 67890 ...  # Run 3 (different output)
```

### Policy Consistency

- Same policy + same config = identical constraints
- Policy changes create new provenance branch
- All policy decisions recorded and verifiable
- Cross-system policy portability

## Security and Governance

### Threat Model Mitigations

1. **Seed Injection Attacks**
   - Seeds validated and logged
   - Seed provenance captured in chains
   - Policy gates seed changes

2. **Entropy Escalation**
   - Real-time entropy monitoring
   - Automatic constraint tightening
   - Policy violation detection

3. **Provenance Tampering**
   - CanonFS immutability prevents modification
   - Chain integrity verification
   - Cryptographic hash verification

4. **Policy Bypass**
   - Every stochastic operation gated by Axion
   - Policy decision logging
   - Constraint enforcement at multiple levels

### Compliance Features

1. **Complete Audit Trail**
   - Every stochastic decision recorded
   - Policy decisions with reasoning
   - Complete parameter provenance

2. **Replay Capability**
   - Auditors can replay exact stochastic paths
   - Seed-based reproducibility
   - Chain verification tools

3. **Policy Transparency**
   - All stochastic policies explicit and versioned
   - Policy change tracking
   - Constraint documentation

## Integration Points

### VM Integration

New opcode family for stochastic operations:

```cpp
// In T81VM instruction decoder
case OpcodeFamily::STOCHASTIC_DECODE: {
    auto result = csi_decoder->decode_with_policy(
        get_tensor_operand(insn.op1),
        get_stochastic_config(insn.op2),
        current_policy_context
    );
    set_register(insn.dest, result.selected_token);
    record_stochastic_provenance(result);
    break;
}
```

### CanonFS Integration

New object type for stochastic provenance:

```cpp
enum class CanonFSObjectType {
    BUNDLE_DECISION = 0,
    MODEL_WEIGHTS = 1,
    STOCHASTIC_PROVENANCE = 2,  // New type
    POLICY_TRACE = 3
};
```

### Axion Integration

Extended policy evaluation for stochastic operations:

```cpp
class StochasticPolicyEvaluator : public PolicyEvaluator {
public:
    AxionVerdict evaluate_stochastic_operation(
        const StochasticOperation& op,
        const PolicyContext& ctx
    ) override;
    
    bool validate_entropy_bounds(double entropy, double max_allowed);
    bool validate_candidate_set_size(size_t k, size_t max_k);
};
```

## Performance Characteristics

### Overhead Analysis

- **Policy Evaluation**: ~100μs per stochastic step
- **Provenance Capture**: ~50μs per step
- **Chain Storage**: ~1KB per step in CanonFS
- **Verification**: ~200μs per chain verification

### Optimization Strategies

1. **Policy Caching** - Cache frequent policy evaluations
2. **Batch Provenance** - Batch multiple steps before storage
3. **Compression** - Compress provenance data for storage
4. **Parallel Sampling** - Parallel candidate evaluation

## Future Evolution

### Phase 1: Foundation (Current)
- Basic stochastic decoding
- Policy gating framework
- CanonFS provenance capture

### Phase 2: Advanced Control
- Multi-seed ensemble sampling
- Adaptive entropy management
- Hierarchical stochastic policies
- Real-time policy learning

### Phase 3: Cognitive Integration
- Stochastic reasoning chains
- Uncertainty quantification
- Probabilistic decision bundles
- Cross-chain consistency

## Relationship to Existing Systems

### Complement to DCP
- DCP provides deterministic foundation
- CSI adds accountable uncertainty
- No pollution of deterministic guarantees
- Clear boundary between layers

### Enhancement to AI Tools
- Extends existing AI CLI with stochastic modes
- Integrates with current model loading
- Complements experimental AI OS research
- Provides formal foundation for cognitive architectures

### Bridge to External AI
- Controlled interface to unbounded models
- Policy-gated external model calls
- Stochastic provenance for external decisions
- Clear trust boundaries

## Testing and Verification

### Unit Tests
- Policy evaluation correctness
- Stochastic algorithm validation
- Provenance chain integrity
- Determinism guarantees

### Integration Tests
- VM integration with stochastic ops
- CanonFS storage and retrieval
- Axion policy enforcement
- Cross-system portability

### Property Tests
- Same seed = same output
- Policy consistency across runs
- Chain integrity verification
- Performance bounds validation

## Conclusion

The Controlled Stochastic Inference subsystem completes T81's architectural vision by providing **accountable uncertainty** with the same rigor as the deterministic core. It enables real-world AI applications while maintaining the system's core principles of governance, provenance, and verifiability.

This is not "non-deterministic AI" - this is **governed stochasticity** where even uncertainty has rules.

---

**Status:** Experimental Subsystem Implementation  
**Location:** `experimental/ai/csi/`  
**Dependencies:** Axion, CanonFS, T81VM  
**Impact:** Completes T81's three-layer intelligence model
