# Axion API Documentation

## Overview

The Axion API provides a comprehensive interface for governing deterministic ternary computing workloads. This documentation covers the complete API surface for policy enforcement, ethics evaluation, and system governance.

## Core Components

### 1. Main API (`api.hpp`)

The primary interface for Axion kernel interaction.

#### Key Structures

```cpp
struct Version {
  uint16_t major{0};
  uint16_t minor{0}; 
  uint16_t patch{0};
  std::string str() const;
};
```

#### Main Functions

```cpp
// Initialize Axion kernel with optional policy
bool initialize(std::optional<Policy> policy = std::nullopt);

// Evaluate a syscall context for governance decision
Verdict evaluate(const SyscallContext& ctx);

// Get current Axion kernel version
Version version();

// Shutdown Axion kernel
void shutdown();
```

#### Usage Example

```cpp
#include "t81/axion/api.hpp"

// Initialize Axion with default policy
if (!t81::axion::initialize()) {
  std::cerr << "Failed to initialize Axion\n";
  return 1;
}

// Create syscall context
t81::axion::SyscallContext ctx;
ctx.caller = "my_program";
ctx.syscall = "file_write";
ctx.snapshot = current_snapshot;

// Evaluate for governance decision
auto verdict = t81::axion::evaluate(ctx);
if (verdict.kind == t81::axion::VerdictKind::Deny) {
  std::cout << "Operation denied: " << verdict.reason << "\n";
}

// Cleanup
t81::axion::shutdown();
```

### 2. Policy Engine (`policy_engine.hpp`)

Core policy evaluation and bytecode execution.

#### Class: PolicyEngine

```cpp
class PolicyEngine {
public:
  explicit PolicyEngine(std::optional<Policy> policy);
  
  // Execute policy bytecode against context
  Verdict execute_bytecode(const SyscallContext& ctx);
  
  // Check if policy requires loop hint satisfaction
  bool requires_loop_hints() const;
  
  // Get current loop satisfaction status
  std::vector<bool> get_loop_status() const;
};
```

#### Usage Example

```cpp
#include "t81/axion/policy_engine.hpp"

// Load policy from file
auto policy = t81::axion::load_policy("security.axp");
t81::axion::PolicyEngine engine(policy);

// Evaluate syscall
t81::axion::SyscallContext ctx;
ctx.caller = "t81vm";
ctx.syscall = "AgentInvoke";

auto verdict = engine.execute_bytecode(ctx);
std::cout << "Decision: " << verdict.reason << "\n";
```

### 3. Ethics System (`ethics.hpp`)

Θ₁-Θ₉ ethics principles evaluation.

#### Ethics Principles

```cpp
enum class EthicsPrinciple {
  Safety = 1,        // Θ₁: Safety and harm prevention
  Privacy = 2,       // Θ₂: Privacy and data protection  
  Fairness = 3,      // Θ₃: Fairness and non-discrimination
  Transparency = 4, // Θ₄: Transparency and explainability
  Accountability = 5,// Θ₅: Accountability and responsibility
  Beneficence = 6,   // Θ₆: Beneficence and well-being
  Justice = 7,       // Θ₇: Justice and equity
  Autonomy = 8,      // Θ₈: Autonomy and consent
  Dignity = 9        // Θ₉: Dignity and respect
};
```

#### Ethics Evaluation

```cpp
struct Verdict {
  VerdictKind kind;
  std::string reason;
};

Verdict check_ethics(EthicsPrinciple principle, const SyscallContext& ctx);
```

#### Usage Example

```cpp
#include "t81/axion/ethics.hpp"

t81::axion::SyscallContext ctx;
ctx.caller = "ai_model";
ctx.syscall = "AgentInvoke";

// Check safety principle
auto safety_verdict = t81::axion::check_ethics(
  t81::axion::EthicsPrinciple::Safety, ctx);

if (safety_verdict.kind == t81::axion::VerdictKind::Deny) {
  std::cout << "Safety violation: " << safety_verdict.reason << "\n";
}
```

### 4. Verdict System (`verdict.hpp`)

Deterministic verdict generation and canonical reasons.

#### Verdict Types

```cpp
enum class VerdictKind {
  Allow,   // Operation permitted
  Deny,    // Operation forbidden
  Warn     // Operation allowed with warning
};

struct Verdict {
  VerdictKind kind;
  std::string reason;  // Canonical reason string
};
```

#### Usage Example

```cpp
t81::axion::Verdict verdict;
verdict.kind = t81::axion::VerdictKind::Allow;
verdict.reason = "Operation within policy bounds";

if (verdict.kind == t81::axion::VerdictKind::Allow) {
  // Proceed with operation
}
```

### 5. Context System (`context.hpp`)

Syscall context and execution environment information.

#### SyscallContext Structure

```cpp
struct SyscallContext {
  std::string caller;     // Who is making the request
  std::string syscall;    // What operation is requested
  t81::canonfs::CanonRef snapshot;  // Current system state
  std::map<std::string, std::string> metadata;  // Additional context
};
```

#### Usage Example

```cpp
t81::axion::SyscallContext ctx;
ctx.caller = "t81vm";
ctx.syscall = "AgentInvoke";
ctx.snapshot = current_system_snapshot;
ctx.metadata["tier"] = "T729";
ctx.metadata["cognitive_load"] = "0.73";
```

## Integration Patterns

### 1. VM Integration

```cpp
// In T81VM execution loop
for (const auto& instruction : program) {
  if (instruction.opcode == TISC::AgentInvoke) {
    t81::axion::SyscallContext ctx;
    ctx.caller = "t81vm";
    ctx.syscall = "AgentInvoke";
    ctx.snapshot = vm.current_snapshot();
    
    auto verdict = t81::axion::evaluate(ctx);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      throw t81::vm::Trap::EthicsViolation;
    }
  }
  
  execute_instruction(instruction);
}
```

### 2. Language Integration

```cpp
// In T81Lang compiler for agent/behavior constructs
void compile_agent_invoke(const AST::AgentInvoke& node) {
  // Generate Axion governance check
  auto ctx = create_syscall_context("t81lang", "AgentInvoke");
  auto verdict = t81::axion::evaluate(ctx);
  
  if (verdict.kind == t81::axion::VerdictKind::Allow) {
    generate_agent_invoke_code(node);
  } else {
    report_compilation_error("Agent invoke denied: " + verdict.reason);
  }
}
```

### 3. Filesystem Integration

```cpp
// In CanonFS operations
t81::canonfs::WriteResult write_object(const Object& obj) {
  t81::axion::SyscallContext ctx;
  ctx.caller = "canonfs";
  ctx.syscall = "object_write";
  ctx.snapshot = current_root();
  ctx.metadata["object_size"] = std::to_string(obj.size());
  ctx.metadata["object_type"] = std::to_string(obj.type());
  
  auto verdict = t81::axion::evaluate(ctx);
  if (verdict.kind == t81::axion::VerdictKind::Deny) {
    return {t81::canonfs::WriteStatus::Denied, verdict.reason};
  }
  
  return perform_write(obj);
}
```

## Policy Authoring

### Policy Structure

```cpp
struct Policy {
  std::string name;
  std::string version;
  std::vector<uint8_t> bytecode;
  std::vector<LoopHint> loops;
  std::vector<Capability> capabilities;
};
```

### Example Policy

```cpp
// Security policy for AI model operations
t81::axion::Policy security_policy;
security_policy.name = "ai_security";
security_policy.version = "1.0.0";

// Bytecode for: deny AgentInvoke unless in tier >= T729
security_policy.bytecode = {
  0x01, 0x00, 0x00, 0x00,  // LOAD tier
  0x02, 0x02, 0xD9, 0x02,  // CONST T729 (729)
  0x03, 0x00, 0x00, 0x00,  // CMP_GE
  0x04, 0x01, 0x00, 0x00,  // JUMP_IF_TRUE 1
  0x05, 0x00, 0x00, 0x00,  // DENY
  0x06, 0x00, 0x00, 0x00   // ALLOW
};
```

## Error Handling

### Deterministic Errors

```cpp
enum class Error {
  PolicyViolation,
  EthicsViolation,
  CapabilityDenied,
  SyntaxError,
  RuntimeError
};
```

### Error Recovery

```cpp
try {
  auto verdict = t81::axion::evaluate(ctx);
  if (verdict.kind == t81::axion::VerdictKind::Deny) {
    // Log denial with canonical reason
    log_axion_event(ctx, verdict);
    return false;
  }
} catch (const t81::axion::AxionError& e) {
  // Handle Axion-specific errors
  std::cerr << "Axion error: " << e.what() << "\n";
  return false;
}
```

## Performance Considerations

### Optimization Tips

1. **Policy Caching**: Cache compiled policies for repeated use
2. **Context Reuse**: Reuse SyscallContext objects when possible
3. **Batch Evaluation**: Evaluate multiple operations together
4. **Early Termination**: Structure policies for early decision points

### Performance Metrics

```cpp
struct PerformanceMetrics {
  uint64_t evaluations_per_second;
  uint64_t average_evaluation_time_ns;
  uint64_t policy_cache_hit_rate;
  uint64_t memory_usage_bytes;
};

PerformanceMetrics get_performance_metrics();
```

## Best Practices

### 1. Policy Design
- Keep policies simple and focused
- Use canonical reason strings
- Test policies thoroughly
- Document policy intent clearly

### 2. Integration
- Always check Axion verdicts before privileged operations
- Log all governance decisions for audit
- Handle denial gracefully
- Maintain deterministic behavior

### 3. Testing
- Test all policy branches
- Verify ethics principle compliance
- Test with various contexts
- Validate deterministic behavior

## Troubleshooting

### Common Issues

1. **Policy Not Loading**: Check policy syntax and bytecode format
2. **Unexpected Denials**: Verify context metadata and policy logic
3. **Performance Issues**: Optimize policy structure and caching
4. **Memory Leaks**: Ensure proper cleanup of policy resources

### Debug Tools

```cpp
// Enable debug logging
t81::axion::set_debug_level(t81::axion::DebugLevel::Verbose);

// Trace policy execution
t81::axion::trace_policy_execution(ctx, true);

// Dump internal state
t81::axion::dump_internal_state();
```

This documentation provides comprehensive coverage of the Axion API for effective integration and policy authoring in deterministic ternary computing systems.
