# Controlled Stochastic Inference (CSI)

## Purpose

Formal subsystem for **policy-governed stochastic execution** within T81's Experimental Frontier.

## Architectural Position

```
Layer 0: Deterministic Substrate (DCP)
├── TISC ISA (v1.9.0 Frozen)
├── T81VM (deterministic interpreter)
├── Axion (policy governance)
├── CanonFS (immutable storage)
└── T81Lang (deterministic compilation)

Layer 1: Governed Stochastic Processes (CSI) ← THIS SUBSYSTEM
├── Controlled Stochastic Inference
├── Policy-gated sampling
├── Stochastic provenance chains
└── Seed-managed replayability

Layer 2: Unbounded External AI (non-trusted)
└── External model integration (research only)
```

## Core Invariants

### 1. Deterministic Envelope
- **Seed Required**: Every stochastic operation requires explicit seed
- **Traceable Execution**: Complete execution path recorded
- **Replayable**: Same seed + same input = identical stochastic path

### 2. Policy Gate (Axion Integration)
Before any stochastic operation:
```axl
# Policy evaluation for stochastic sampling
policy "stochastic.inference.sampling" {
    description: "Control stochastic sampling behavior",
    version: "1.0",
    
    # Pre-sampling validation
    require_explicit_consent: true,
    evidence_collection: true,
    
    # Sampling constraints
    allowed_contexts: ["ai.inference", "ai.exploration"],
    max_entropy_per_token: 2.5,  # bits of uncertainty
    
    # Candidate set control
    default_top_k: 5,
    default_temperature: 1.0,
    
    # Provenance requirements
    capture_logits: true,
    capture_candidates: true,
    capture_selection_trace: true
}
```

### 3. CanonFS Capture
Every stochastic step produces immutable evidence:
```json
{
  "stochastic_step": {
    "seed": "0x7f3a2b1c",
    "timestep": 42,
    "input_hash": "sha3-256:...",
    "candidates": [
      {"token": "Paris", "logit": 2.34, "rank": 1},
      {"token": "Lyon", "logit": 1.87, "rank": 2},
      {"token": "Marseille", "logit": 1.23, "rank": 3}
    ],
    "selected_token": "Paris",
    "selection_method": "top_k_sampling",
    "entropy": 1.82,
    "policy_verdict": "allow",
    "canonfs_hash": "sha3-256:..."
  }
}
```

## Implementation Components

### 1. Stochastic Decoder
```cpp
class ControlledStochasticDecoder {
public:
    struct StochasticConfig {
        uint64_t seed;
        double temperature;
        size_t top_k;
        bool deterministic_envelope;
    };
    
    struct StochasticResult {
        std::string selected_token;
        std::vector<Candidate> candidates;
        double entropy;
        uint64_t seed_used;
        CanonFSHash provenance_hash;
    };
    
    StochasticResult decode_with_policy(
        const Tensor& logits,
        const StochasticConfig& config,
        AxionPolicyContext& policy_ctx
    );
};
```

### 2. Policy-Gated Sampling
```cpp
class PolicyGatedSampler {
public:
    enum class SamplingMethod {
        DETERMINISTIC_GREEDY,
        STOCHASTIC_TOP_K,
        STOCHASTIC_NUCLEUS,
        STOCHASTIC_TEMPERATURE
    };
    
    SamplingResult sample(
        const Tensor& logits,
        SamplingMethod method,
        const SamplingConfig& config,
        AxionPolicyGate& policy_gate
    );
};
```

### 3. Stochastic Provenance Chain
```cpp
class StochasticProvenanceChain {
public:
    struct StochasticStep {
        uint64_t seed;
        uint32_t timestep;
        std::vector<Candidate> candidates;
        std::string selected_token;
        double entropy;
        AxionVerdict policy_verdict;
        CanonFSHash step_hash;
    };
    
    void append_step(const StochasticStep& step);
    CanonFSHash finalize_chain();
    bool verify_chain_integrity(const CanonFSHash& expected_hash);
};
```

## Integration Points

### 1. VM Integration
Add stochastic decode mode to T81VM:
```cpp
// In vm.cpp - new opcode family
case OpcodeFamily::STOCHASTIC_DECODE: {
    auto result = csi_decoder.decode_with_policy(
        get_tensor_operand(insn.op1),
        get_stochastic_config(insn.op2),
        current_policy_context
    );
    set_register(insn.dest, result.selected_token);
    record_stochastic_provenance(result);
    break;
}
```

### 2. Axion Policy Integration
Extend Axion for stochastic operations:
```cpp
// In axion policy evaluation
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

### 3. CanonFS Storage
New stochastic provenance object type:
```cpp
// In canonfs object types
enum class ObjectType {
    BUNDLE_DECISION = 0,
    MODEL_WEIGHTS = 1,
    STOCHASTIC_PROVENANCE = 2,  // New type
    POLICY_TRACE = 3
};
```

## Usage Examples

### Basic Stochastic Inference
```bash
# Run with controlled stochastic inference
./build/t81 ai inference run \
  --model tinyllama \
  --model-file /tmp/tinyllama.gguf \
  --mode controlled_stochastic \
  --seed 12345 \
  --stochastic-config top_k=5,temperature=1.0 \
  --prompt "What is the capital of France?" \
  --out /tmp/stochastic_result.json
```

### Policy-Gated Exploration
```bash
# Run with explicit policy for stochastic exploration
./build/t81 ai inference run \
  --model tinyllama \
  --model-file /tmp/tinyllama.gguf \
  --mode controlled_stochastic \
  --seed 12345 \
  --policy experimental/stochastic_exploration.apl \
  --prompt "Creative writing task" \
  --out /tmp/creative_result.json
```

### Provenance Verification
```bash
# Verify stochastic provenance chain
./build/t81 canonfs verify-chain \
  --stochastic-hash <hash_from_result> \
  --canonfs-root /tmp/canonfs \
  --policy verification_policy.apl
```

## Determinism Guarantees

### Replayable Stochasticity
```bash
# Same seed = identical stochastic path
./build/t81 ai inference run --seed 12345 ...  # Run 1
./build/t81 ai inference run --seed 12345 ...  # Run 2 (identical)

# Different seed = different path, but same provenance structure
./build/t81 ai inference run --seed 67890 ...  # Run 3 (different output)
```

### Policy Consistency
- Same policy + same config = identical constraints
- Policy changes create new provenance branch
- All policy decisions recorded and verifiable

## Security and Governance

### Threat Model Mitigations
1. **Seed Injection Attacks** - Seeds validated and logged
2. **Entropy Escalation** - Policy enforces entropy bounds
3. **Provenance Tampering** - CanonFS immutability prevents modification
4. **Policy Bypass** - Axion gates every stochastic operation

### Compliance Features
1. **Complete Audit Trail** - Every stochastic decision recorded
2. **Replay Capability** - Auditors can replay exact stochastic paths
3. **Policy Transparency** - All stochastic policies explicit and versioned
4. **Cross-System Portability** - Stochastic chains consumable without original environment

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

## Future Evolution Path

### Phase 1: Foundation (Current)
- Basic stochastic decoding
- Policy gating framework
- CanonFS provenance capture

### Phase 2: Advanced Control
- Multi-seed ensemble sampling
- Adaptive entropy management
- Hierarchical stochastic policies

### Phase 3: Cognitive Integration
- Stochastic reasoning chains
- Uncertainty quantification
- Probabilistic decision bundles

## Conclusion

Controlled Stochastic Inference provides the **missing formal subsystem** for accountable uncertainty in T81. It maintains the architecture's core principle of **governed execution** while extending it to handle **real-world intelligence requirements**.

This is not "non-deterministic AI" - this is **accountable uncertainty** with the same rigor as the deterministic core.

---

**Status:** Proposed Subsystem Definition  
**Location:** Experimental Frontier → AI → CSI  
**Impact:** Completes T81's architectural vision for real-world intelligence
