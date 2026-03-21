# T81 Axion Policy Hooks for AI Events - RFC-00A6 Task 8

This directory contains the Axion policy system extensions for AI inference and tooling events with comprehensive audit logging.

## Components

### Axion Policy Hooks (`t81_ai_policy`)
CLI tool for managing AI-specific policy rules and event evaluation with audit logging.

**Usage:**
```bash
# Initialize policy hooks
./t81_ai_policy init ./policy_config.json ./audit.log

# Evaluate policy for event
./t81_ai_policy evaluate model_load '{"model_size": "1048576"}'

# List all policy rules
./t81_ai_policy list-rules

# Show audit log
./t81_ai_policy audit [filter]

# Generate policy report
./t81_ai_policy report
```

## AI Event Types

### Model Events
- **MODEL_LOAD**: Model loading and initialization
- **MODEL_UNLOAD**: Model unloading and cleanup

### Inference Events
- **INFERENCE_START**: Beginning of inference execution
- **INFERENCE_COMPLETE**: Completion of inference with results

### Tool Use Events
- **TOOL_USE_START**: AI agent tool usage initiation
- **TOOL_USE_COMPLETE**: Tool usage completion with results

### Processing Events
- **QUANTIZATION_START**: Beginning of model quantization
- **QUANTIZATION_COMPLETE**: Quantization process completion

### Security Events
- **POLICY_VIOLATION**: Policy rule violation detection

## Policy Decision Types

### ALLOW
- **Description**: Action is permitted
- **Use Case**: Normal operations within policy bounds
- **Audit Level**: Standard logging

### DENY
- **Description**: Action is explicitly forbidden
- **Use Case**: Security violations, policy breaches
- **Audit Level**: High priority alert

### LOG_ONLY
- **Description**: Action permitted but logged for monitoring
- **Use Case**: Sensitive operations requiring oversight
- **Audit Level**: Enhanced monitoring

### REQUIRE_APPROVAL
- **Description**: Action requires manual approval
- **Use Case**: High-risk operations needing human review
- **Audit Level**: Pending approval workflow

## Policy Rule Structure

### Rule Definition
```json
{
  "rule_id": "model_size_limit",
  "name": "Model Size Limit",
  "description": "Limit model loading to models under 1GB",
  "applicable_events": ["model_load"],
  "conditions": {
    "model_size_limit": "1073741824"
  },
  "default_decision": "deny",
  "enabled": true,
  "created_timestamp": "2026-03-05 01:00:00",
  "modified_timestamp": "2026-03-05 01:00:00"
}
```

### Condition Types
- **model_id**: Specific model identifier matching
- **user_id**: User-based access control
- **time_of_day**: Time-based restrictions (HH:MM-HH:MM)
- **model_size_limit**: Model file size limits
- **model_type**: Model format or type restrictions
- **user_role**: Role-based access control
- **session_duration**: Maximum session time limits

## Default Security Policies

### Model Size Limit
- **Rule ID**: model_size_limit
- **Purpose**: Prevent loading of oversized models
- **Default**: 1GB limit
- **Status**: Enabled by default

### Business Hours Restriction
- **Rule ID**: business_hours
- **Purpose**: Restrict AI operations to business hours
- **Default**: 9 AM - 5 PM
- **Status**: Disabled by default

### Model Whitelist
- **Rule ID**: model_whitelist
- **Purpose**: Only allow approved models
- **Default**: Empty whitelist
- **Status**: Disabled by default

## Audit Logging

### Audit Entry Format
```json
{
  "event_id": "event_1642345678900",
  "event_type": "model_load",
  "timestamp": "2026-03-05 01:00:00",
  "model_id": "model_12345",
  "user_id": "user_67890",
  "session_id": "session_abc123",
  "event_data": {
    "model_size": "1048576",
    "model_format": "gguf"
  },
  "context": {
    "ip_address": "192.168.1.100",
    "user_agent": "t81_ai_backend/1.0.0"
  },
  "policy_result": {
    "decision": "allow",
    "rule_id": "model_size_limit",
    "reasoning": "Model size within limits",
    "metadata": {
      "evaluation_time_ms": "5"
    },
    "violations": []
  }
}
```

### Audit Features
- **Complete Event Tracking**: All AI events with full context
- **Policy Evaluation**: Rule matching and decision reasoning
- **Violation Detection**: Automatic policy violation identification
- **Performance Metrics**: Policy evaluation timing and overhead
- **Search and Filter**: Query audit log by event type, model, user

## Integration with T81 Ecosystem

### Backend Adapter Integration
```cpp
// Example integration with LLM backend
AxionPolicyHook hooks(config_path, audit_path);
hooks.initialize();

// Before model loading
auto event = hooks.create_model_load_event(model_id, user_id, session_id, model_data);
auto result = hooks.evaluate_event(event);

if (result.decision == PolicyDecision::ALLOW) {
    // Proceed with model loading
    load_model(model_path);
} else {
    // Handle policy violation
    handle_policy_denial(result);
}
```

### Determinism Framework Integration
- **Event Consistency**: All policy evaluations are deterministic
- **Audit Integrity**: Tamper-evident audit logging
- **Reproducible Results**: Same inputs produce same decisions

## Acceptance Criteria

- [x] AI policy hooks handle all specified event types
- [x] Policy decisions are deterministic and auditable
- [x] Audit logging captures all required information
- [x] Performance overhead within acceptable limits (<10%)
- [x] Integration with existing Axion system seamless

## Build Instructions

```bash
# Enable AI experiments
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
make t81_ai_policy

# Initialize policy hooks
./build/experiments/ai/bin/t81_ai_policy init ./policy_config.json ./audit.log

# Test policy evaluation
./build/experiments/ai/bin/t81_ai_policy evaluate model_load '{"model_size": "1048576"}'
```

## Security Considerations

- **Policy Integrity**: Policy rules are cryptographically signed
- **Audit Protection**: Audit logs are append-only and tamper-evident
- **Access Control**: Policy management requires administrative privileges
- **Compliance**: All policy decisions are logged for compliance auditing
- **Performance**: Policy evaluation overhead is minimal and monitored

---

**RFC Reference**: RFC-00A6  
**Task**: 8 - Implement Axion policy hooks for AI events  
**Status**: Completed  
**Last Updated**: 2026-03-05
