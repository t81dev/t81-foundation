---
name: t81_ai_tasks
description: Execute bounded AI task chains (assess-fixed, route-fixed, classify-fixed) with deterministic guarantees
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 AI Tasks Skill

This skill executes bounded AI task chains that provide deterministic decision-making with complete provenance and policy enforcement.

## Usage

When the user needs to run structured AI decision workflows with guaranteed determinism, use this skill to execute bounded task chains.

### Commands

**Assess-Fixed Task:**
```
assess <input_data> [--model <model_file>] [--policy <policy_file>] [--output <output_file>]
```

**Route-Fixed Task:**
```
route <input_data> [--routes <route_config>] [--policy <policy_file>] [--output <output_file>]
```

**Classify-Fixed Task:**
```
classify <input_data> [--classes <class_config>] [--policy <policy_file>] [--output <output_file>]
```

**Chain Execution:**
```
chain <chain_config> [--input <input_data>] [--policy <policy_file>] [--bundle-output <bundle_file>]
```

**Task Status:**
```
status <task_id> [--detailed] [--provenance]
```

**Task History:**
```
history [--limit <count>] [--filter assess|route|classify] [--json]
```

### Examples

```bash
# Assess fixed decision with model
assess market_data.csv --model trading_model.t81w --policy risk_assessment.apl

# Route input through predefined decision paths
route customer_request.json --routes routing_config.json --policy compliance.apl

# Classify data with fixed categories
classify medical_data.json --classes diagnosis_classes.json --policy medical.apl

# Execute complete decision chain
chain trading_workflow.json --input market_data.csv --bundle-output decisions.bundle

# Check task execution status
status task_12345 --detailed --provenance

# View task execution history
history --limit 10 --filter assess --json
```

### Task Types

**Assess-Fixed:**
- Evaluates input against fixed criteria
- Produces deterministic assessment results
- Used for risk analysis, compliance checking
- Complete audit trail for decisions

**Route-Fixed:**
- Routes inputs through predetermined paths
- Deterministic routing decisions
- Used for workflow orchestration
- Policy-gated path selection

**Classify-Fixed:**
- Classifies inputs into fixed categories
- Deterministic classification results
- Used for categorization and labeling
- Consistent classification across runs

### Chain Composition

**Sequential Chains:**
```
Input → Assess → Route → Classify → Output
```

**Parallel Chains:**
```
Input → [Assess1, Assess2, Assess3] → Aggregate → Output
```

**Conditional Chains:**
```
Input → Assess → Route(Condition) → [ClassifyA|ClassifyB] → Output
```

### Configuration Format

**Task Configuration:**
```json
{
  "task_type": "assess-fixed",
  "model": "model.t81w",
  "policy": "security.apl",
  "parameters": {
    "confidence_threshold": 0.95,
    "determinism_mode": "strict"
  },
  "output_format": "bundle"
}
```

**Chain Configuration:**
```json
{
  "chain_id": "trading_workflow",
  "tasks": [
    {
      "type": "assess-fixed",
      "model": "risk_model.t81w",
      "policy": "risk_assessment.apl"
    },
    {
      "type": "route-fixed", 
      "routes": "trading_routes.json",
      "policy": "execution_policy.apl"
    },
    {
      "type": "classify-fixed",
      "classes": "trade_classes.json",
      "policy": "classification.apl"
    }
  ],
  "bundle_output": true
}
```

### Output Format

**Task Result:**
```json
{
  "task_id": "CanonHash81...",
  "task_type": "assess-fixed",
  "status": "completed",
  "timestamp": "2026-04-04T15:30:00Z",
  "input_hash": "CanonHash81...",
  "result": {
    "assessment": "high_risk",
    "confidence": 0.98,
    "determinism_verified": true
  },
  "provenance": {
    "model_hash": "CanonHash81...",
    "policy_hash": "CanonHash81...",
    "execution_trace": "CanonHash81..."
  },
  "bundle": {
    "created": true,
    "bundle_hash": "CanonHash81...",
    "bundle_location": "/bundles/assessment_123.bundle"
  }
}
```

**Chain Result:**
```json
{
  "chain_id": "trading_workflow",
  "status": "completed",
  "timestamp": "2026-04-04T15:30:00Z",
  "tasks_executed": 3,
  "tasks_completed": 3,
  "tasks_failed": 0,
  "final_result": {
    "decision": "execute_trade",
    "parameters": {
      "symbol": "AAPL",
      "quantity": 100,
      "confidence": 0.97
    }
  },
  "bundle": {
    "bundle_hash": "CanonHash81...",
    "contains_provenance": true,
    "determinism_guaranteed": true
  }
}
```

### Determinism Guarantees

- **Input Determinism**: Same inputs produce identical outputs
- **Temporal Determinism**: Execution timing is reproducible
- **Policy Determinism**: Same policy decisions for identical states
- **Chain Determinism**: Complete chain execution is reproducible

### Policy Integration

**Pre-Execution Checks:**
- Input validation against policies
- Model authorization verification
- Resource access permissions

**Runtime Enforcement:**
- Step-by-step policy validation
- Resource usage monitoring
- Decision point auditing

**Post-Execution Validation:**
- Output compliance checking
- Bundle integrity verification
- Provenance completeness

### Error Handling

- **Task Failures**: Detailed error information with recovery suggestions
- **Policy Violations**: Specific rule violations with remediation steps
- **Model Errors**: Loading and execution problem handling
- **Chain Breaks**: Partial completion handling and rollback

### Security Notes

- All tasks subject to Axion policy enforcement
- Complete audit trail for regulatory compliance
- Immutable bundle storage for provenance
- Deterministic execution prevents manipulation

### Integration

This skill wraps T81's bounded AI task chains:
```bash
t81 ai task <task_type> --input <data> --model <model> --policy <policy> --bundle
```
