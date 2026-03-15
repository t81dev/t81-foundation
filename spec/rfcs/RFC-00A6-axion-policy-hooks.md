# RFC-00A6: Axion Policy Hooks for Inference and Tooling Events

Version 0.1 — Standards Track\
Status: Accepted\
Author: T81 Foundation Architecture Team\
Applies to: Axion Policy System, AI Security, Event Handling\
Updated: 2026-03-15

> **Acceptance note (2026-03-15):** The AI-specific policy hooks defined here
> are implemented as `AIHookEngine` + `PolicyEngine` per RFC-0032 Phase 3.
> Canonical hook event identifiers are registered in
> `spec/supplemental/axion-event-registry.md`.  Live APIs:
> `include/t81/axion/ai_hooks.hpp`, `include/t81/axion/policy_engine.hpp`.
> Gate tests: `tests/cpp/axion_ai_hooks_test.cpp` [C04-01..08].

______________________________________________________________________

## Summary

This RFC extends the Axion policy system with specialized hooks for AI inference and tooling events. It defines policy events, decision outcomes, and audit logging requirements specifically for AI workloads, enabling fine-grained control over model loading, inference execution, and tool-use scenarios while maintaining T81's security and determinism guarantees.

______________________________________________________________________

## Motivation

AI inference introduces unique security and policy challenges:
- Model loading and execution need security clearance
- Tool use by AI agents requires policy validation
- Inference resource consumption needs monitoring
- AI-generated content requires policy compliance checks

The existing Axion policy system needs AI-specific extensions to handle these scenarios while preserving the deterministic and auditable nature of policy enforcement.

## Proposal

### Technical Details

#### 1. AI Policy Event Classification

**Event Categories:**
- **Model Events**: Model loading, unloading, and access
- **Inference Events**: Inference execution, streaming, and completion
- **Tool Events**: Tool discovery, invocation, and result processing
- **Resource Events**: Resource allocation, usage, and cleanup
- **Content Events**: Content generation, filtering, and distribution

**Event Priority Levels:**
- **Critical**: Security violations, policy breaches
- **High**: Model loading, resource allocation
- **Medium**: Inference execution, tool invocation
- **Low**: Metrics collection, status updates

#### 2. Policy Hook Interface

**Core Hook Interface:**
```cpp
namespace t81::axion::ai {

class IAIPolicyHook {
public:
    virtual ~IAIPolicyHook() = default;
    
    // Event handling
    virtual PolicyDecision handle_event(
        const AIEvent& event,
        const PolicyContext& context
    ) = 0;
    
    // Pre-execution validation
    virtual PolicyDecision pre_execution_check(
        const AIEvent& event,
        const PolicyContext& context
    ) = 0;
    
    // Post-execution audit
    virtual void post_execution_audit(
        const AIEvent& event,
        const PolicyContext& context,
        const ExecutionResult& result
    ) = 0;
    
    // Hook metadata
    virtual std::string hook_name() const = 0;
    virtual std::vector<AIEventType> supported_events() const = 0;
    virtual HookPriority priority() const = 0;
};

}
```

**Policy Decision Structure:**
```cpp
enum class PolicyDecision {
    ALLOW,              // Permit the action
    DENY,               // Block the action
    TRANSFORM,          // Modify and allow
    LOG_ONLY,           // Allow but log for audit
    REQUIRE_APPROVAL    // Require human approval
};

struct PolicyResult {
    PolicyDecision decision;
    std::string reason;
    std::map<std::string, std::any> metadata;
    std::optional<std::string> transformed_data;
    std::optional<ApprovalRequest> approval_request;
};
```

#### 3. AI Event Specifications

**Model Loading Event:**
```cpp
struct ModelLoadEvent : public AIEvent {
    std::string model_id;
    std::string model_hash;
    std::string model_format;
    size_t model_size_bytes;
    std::string requester_id;
    std::map<std::string, std::string> model_metadata;
    
    // Security information
    std::vector<std::string> model_signatures;
    std::string trust_level;
    std::vector<std::string> security_flags;
};
```

**Inference Execution Event:**
```cpp
struct InferenceEvent : public AIEvent {
    std::string session_id;
    std::string model_id;
    std::string prompt_hash;
    uint64_t max_tokens;
    double temperature;
    uint64_t random_seed;
    
    // Resource requirements
    size_t estimated_memory_mb;
    double estimated_duration_sec;
    std::string backend_name;
    
    // Content classification
    std::vector<std::string> content_categories;
    std::string sensitivity_level;
};
```

**Tool Invocation Event:**
```cpp
struct ToolInvocationEvent : public AIEvent {
    std::string tool_name;
    std::string tool_version;
    std::string session_id;
    std::map<std::string, std::string> tool_parameters;
    std::string tool_hash;
    
    // Security classification
    std::string tool_risk_level;
    std::vector<std::string> required_permissions;
    std::vector<std::string> data_access_patterns;
};
```

#### 4. Policy Rule Language Extensions

**AI-Specific Policy Directives:**
```ebnf
ai_policy_directive ::= model_load_directive
                      | inference_directive
                      | tool_use_directive
                      | resource_directive

model_load_directive ::= '(' 'model-load-policy' model_load_rule ')'
inference_directive ::= '(' 'inference-policy' inference_rule ')'
tool_use_directive ::= '(' 'tool-use-policy' tool_use_rule ')'
resource_directive ::= '(' 'resource-policy' resource_rule ')'
```

**Example Policy Rules:**
```lisp
;; Model loading policy
(model-load-policy
  (allow-models ["sha3-256:trusted_model_hash"])
  (deny-models ["sha3-256:compromised_model_hash"])
  (require-signatures ["t81-foundation-2024"])
  (max-model-size "10GB")
  (allowed-formats ["t81_canonical" "gguf"]))

;; Inference policy
(inference-policy
  (max-tokens-per-request 4096)
  (allowed-temperature-range [0.0 2.0])
  (require-determinism-for-models ["llama-7b"])
  (log-sensitive-prompts true)
  (content-filtering enabled))

;; Tool use policy
(tool-use-policy
  (allowed-tools ["file_read" "web_search" "calculator"])
  (dangerous-tools ["file_write" "system_command"])
  (require-approval-for ["file_write" "system_command"])
  (log-all-tool-use true))
```

#### 5. Audit Logging Framework

**Audit Log Structure:**
```cpp
struct AIAuditLog {
    // Event identification
    std::string event_id;
    std::chrono::system_clock::time_point timestamp;
    AIEventType event_type;
    std::string session_id;
    
    // Policy decision
    PolicyDecision decision;
    std::string policy_rule_applied;
    std::string decision_reason;
    
    // Event details
    AIEvent event_details;
    PolicyContext context;
    
    // Execution outcome
    std::optional<ExecutionResult> result;
    std::optional<std::string> error_message;
    
    // Security metadata
    std::string user_id;
    std::vector<std::string> security_tags;
    std::map<std::string, std::string> compliance_flags;
};
```

**Audit Log Format:**
```json
{
  "audit_log": {
    "event_id": "ai-event-abc123",
    "timestamp": "2026-03-05T12:00:00Z",
    "event_type": "model_load",
    "session_id": "session-def456",
    "decision": "ALLOW",
    "policy_rule": "model-load-policy",
    "reason": "Model hash matches trusted list",
    "event_details": {
      "model_id": "llama-7b-chat",
      "model_hash": "sha3-256:abc123...",
      "requester_id": "user-789",
      "model_size_bytes": 4194304000
    },
    "context": {
      "user_permissions": ["model_load"],
      "resource_usage": {"memory_mb": 4096},
      "security_context": {"trust_level": "medium"}
    },
    "result": {
      "execution_time_ms": 1250,
      "memory_allocated_mb": 4096,
      "success": true
    },
    "security_metadata": {
      "user_id": "alice@t81.dev",
      "security_tags": ["ai_inference", "model_load"],
      "compliance_flags": {
        "gdpr_compliant": true,
        "data_residency": "eu"
      }
    }
  }
}
```

#### 6. Reference Hook Implementations

**Model Loading Security Hook:**
```cpp
class ModelLoadSecurityHook : public IAIPolicyHook {
public:
    PolicyDecision handle_event(
        const AIEvent& event,
        const PolicyContext& context
    ) override {
        const auto& model_event = 
            static_cast<const ModelLoadEvent&>(event);
        
        // Check model hash against allowlist
        if (!is_model_allowed(model_event.model_hash)) {
            return PolicyDecision{
                PolicyDecision::DENY,
                "Model hash not in trusted list",
                {},
                std::nullopt,
                std::nullopt
            };
        }
        
        // Verify model signatures
        if (!verify_model_signatures(model_event)) {
            return PolicyDecision{
                PolicyDecision::DENY,
                "Model signature verification failed",
                {},
                std::nullopt,
                std::nullopt
            };
        }
        
        // Check model size limits
        if (model_event.model_size_bytes > max_model_size_) {
            return PolicyDecision{
                PolicyDecision::REQUIRE_APPROVAL,
                "Model exceeds size limit",
                {},
                std::nullopt,
                create_approval_request(model_event)
            };
        }
        
        return PolicyDecision{
            PolicyDecision::ALLOW,
            "Model passes all security checks",
            {},
            std::nullopt,
            std::nullopt
        };
    }
    
private:
    bool is_model_allowed(const std::string& hash);
    bool verify_model_signatures(const ModelLoadEvent& event);
    ApprovalRequest create_approval_request(const ModelLoadEvent& event);
    
    size_t max_model_size_ = 10ULL * 1024 * 1024 * 1024; // 10GB
};
```

**Inference Resource Monitoring Hook:**
```cpp
class InferenceResourceHook : public IAIPolicyHook {
public:
    PolicyDecision handle_event(
        const AIEvent& event,
        const PolicyContext& context
    ) override {
        const auto& inference_event = 
            static_cast<const InferenceEvent&>(event);
        
        // Check resource availability
        if (!check_resource_availability(inference_event)) {
            return PolicyDecision{
                PolicyDecision::DENY,
                "Insufficient resources for inference",
                {},
                std::nullopt,
                std::nullopt
            };
        }
        
        // Log resource allocation
        log_resource_allocation(inference_event);
        
        return PolicyDecision{
            PolicyDecision::ALLOW,
            "Resources allocated successfully",
            {},
            std::nullopt,
            std::nullopt
        };
    }
    
    void post_execution_audit(
        const AIEvent& event,
        const PolicyContext& context,
        const ExecutionResult& result
    ) override {
        // Track actual resource usage
        update_resource_metrics(event, result);
        
        // Check for resource leaks
        detect_resource_anomalies(event, result);
    }
    
private:
    bool check_resource_availability(const InferenceEvent& event);
    void log_resource_allocation(const InferenceEvent& event);
    void update_resource_metrics(const AIEvent& event, const ExecutionResult& result);
    void detect_resource_anomalies(const AIEvent& event, const ExecutionResult& result);
};
```

#### 7. Integration with Axion Core

**Hook Registration:**
```cpp
// Register AI-specific hooks during Axion initialization
void register_ai_hooks(AxionKernel& kernel) {
    auto& policy_engine = kernel.get_policy_engine();
    
    // Register model loading hooks
    policy_engine.register_hook(
        std::make_unique<ModelLoadSecurityHook>()
    );
    policy_engine.register_hook(
        std::make_unique<ModelProvenanceHook>()
    );
    
    // Register inference hooks
    policy_engine.register_hook(
        std::make_unique<InferenceResourceHook>()
    );
    policy_engine.register_hook(
        std::make_unique<ContentFilteringHook>()
    );
    
    // Register tool use hooks
    policy_engine.register_hook(
        std::make_unique<ToolSecurityHook>()
    );
    policy_engine.register_hook(
        std::make_unique<ToolAuditHook>()
    );
}
```

### Corner Cases

#### Policy Conflicts
- Priority-based conflict resolution
- Policy override mechanisms for emergencies
- Conflict logging and notification

#### Hook Failures
- Graceful degradation when hooks fail
- Fail-safe default behaviors
- Hook health monitoring

#### Performance Impact
- Asynchronous hook execution where possible
- Hook result caching
- Performance monitoring and optimization

## Impact

### Backward Compatibility

No impact on existing Axion policies. AI hooks are additive and opt-in.

### Performance

Policy enforcement adds overhead:
- Hook execution: ~1-10ms per event
- Audit logging: ~1-5ms per event
- Memory overhead: ~50-200MB for policy state

Overhead is minimal compared to inference latency.

### Security

Significantly enhanced security through:
- Fine-grained policy control over AI operations
- Comprehensive audit trails
- Real-time policy enforcement
- Automated threat detection

## Alternatives Considered

1. **Generic policy extensions**: Rejected due to lack of AI-specific context
2. **Post-execution only policies**: Rejected due to security risks
3. **Hardcoded security rules**: Rejected due to lack of flexibility

## UX / Developer Experience Impact

### CLI Interface

```bash
# Test AI policy rules
t81 axion test-policy --event model_load --model model.gguf

# View audit logs
t81 axion audit-logs --filter ai_events --since "2026-03-01"

# Add new policy rule
t81 axion add-policy --file ai_policy.lisp --type ai

# Validate policy configuration
t81 axion validate-policy --include-ai-hooks

# Monitor policy violations
t81 axion monitor-violations --alert-email security@t81.dev
```

### IDE Integration

- Policy rule editor with syntax highlighting
- Real-time policy validation
- Audit log viewer and analysis
- Policy impact simulation tools

### Development Workflow

- Policy testing framework
- Automated policy validation in CI/CD
- Security audit integration
- Compliance reporting tools

## Acceptance Criteria

1. AI policy hooks handle all specified event types
2. Policy decisions are deterministic and auditable
3. Audit logging captures all required information
4. Performance overhead within acceptable limits
5. Integration with existing Axion system seamless

## Promotion Gates

### Experimental → Extension
- [ ] All AI event types supported
- [ ] Reference implementations working
- [ ] Policy language extensions implemented
- [ ] Audit logging framework complete

### Extension → Core
- [ ] Adopted as standard AI policy framework
- [ ] All AI features use policy hooks
- [ ] Community policy rule library established
- [ ] Security audit passed by third parties

## References

- [Axion Policy Language](RFC-0009-axion-policy-language.md)
- [Axion Safety Model](RFC-0003-axion-safety-model.md)
- [Model Artifact Provenance](RFC-00A3-model-artifact-provenance.md)
- [Deterministic Evidence Protocol](RFC-00A1-deterministic-evidence-protocol.md)
