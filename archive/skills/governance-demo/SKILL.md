---
name: t81_governance_demo
description: Demonstrate T81's policy-gated computation with educational examples
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 Governance Demo Skill

This skill provides educational demonstrations of T81's core value proposition: policy-gated deterministic computation with complete audit trails.

## Usage

When the user wants to understand T81's governance capabilities or see policy enforcement in action, use this skill for interactive demonstrations.

### Commands

**Run basic governance demo:**
```
demo-governance [--policy <policy_file>] [--model <model_file>] [--interactive]
```

**Demonstrate policy enforcement:**
```
demo-policy [--scenario basic|advanced|financial|healthcare] [--show-violations]
```

**Show determinism guarantees:**
```
demo-determinism [--iterations <count>] [--compare-results]
```

**Policy creation tutorial:**
```
tutorial-policy [--level beginner|intermediate|advanced] [--examples]
```

**Interactive policy testing:**
```
interactive-policy [--policy-template <template>] [--test-scenarios]
```

**Compliance showcase:**
```
compliance-demo [--industry finance|healthcare|government] [--regulations]
```

### Examples

```bash
# Basic governance demonstration
demo-governance --policy security.apl --interactive

# Advanced financial compliance demo
demo-policy --scenario financial --show-violations

# Test determinism with multiple runs
demo-determinism --iterations 10 --compare-results

# Learn policy creation
tutorial-policy --level intermediate --examples

# Interactive policy testing
interactive-policy --policy-template security_template.apl

# Healthcare compliance demonstration
compliance-demo --industry healthcare --regulations
```

### Demo Scenarios

**Basic Governance Demo:**
- Simple matrix multiplication with policy gates
- Real-time policy decision visualization
- Clear allow/deny examples
- Audit trail generation

**Financial Compliance Demo:**
- Trading decision with risk management policies
- Regulatory compliance checking
- Complete audit trail for regulators
- Deterministic decision reproduction

**Healthcare Demo:**
- Medical diagnosis with privacy policies
- HIPAA compliance enforcement
- Patient data protection
- Clinical decision auditability

**Advanced Security Demo:**
- Multi-level security policies
- Resource access control
- Privilege escalation prevention
- Security violation analysis

### Policy Templates

**Security Policy Template:**
```apl
# Basic security policy
allow read if user.role in ["admin", "user"];
allow write if user.role = "admin";
deny execute if resource.classification > user.clearance;
allow inference if model.hash in approved_models;
```

**Financial Policy Template:**
```apl
# Financial compliance policy
allow trade if position_size <= risk_limits.max_position;
allow trade if model.confidence >= 0.95;
deny trade if time in market_closure_hours;
require audit if trade.value > 1000000;
```

**Healthcare Policy Template:**
```apl
# Healthcare privacy policy
allow diagnosis if user.role = "physician";
deny diagnosis if patient.consent = false;
allow research if data.anonymized = true;
require audit if access.patient_data = true;
```

### Output Format

**Governance Demo Result:**
```json
{
  "demo_type": "basic_governance",
  "status": "completed",
  "timestamp": "2026-04-04T15:30:00Z",
  "scenario": {
    "operation": "matrix_multiplication",
    "policy": "security.apl",
    "model": "demo_model.t81w"
  },
  "policy_decisions": {
    "total_checks": 1247,
    "allowed": 1198,
    "denied": 49,
    "violations": 0
  },
  "determinism": {
    "verified": true,
    "hash_consistent": true,
    "reproducible": true
  },
  "audit_trail": {
    "generated": true,
    "entries": 1247,
    "comprehensive": true,
    "cryptographic": true
  },
  "educational_notes": [
    "Policy checks occur before each instruction execution",
    "Denials prevent computation without side effects",
    "All decisions are logged for complete auditability"
  ]
}
```

**Interactive Policy Testing:**
```json
{
  "session_id": "CanonHash81_POLICY...",
  "policy_template": "security_template.apl",
  "test_scenarios": [
    {
      "scenario": "unauthorized_file_access",
      "input": {
        "user": "guest",
        "operation": "write",
        "resource": "sensitive_file.txt"
      },
      "policy_decision": "deny",
      "reason": "user.role not in allowed_roles",
      "explanation": "Guest users cannot write to sensitive files"
    }
  ],
  "learning_outcomes": [
    "Understanding role-based access control",
    "Policy decision logic visualization",
    "Real-time policy evaluation"
  ]
}
```

### Educational Features

**Step-by-Step Explanations:**
- Policy rule breakdown
- Decision tree visualization
- Real-time enforcement demonstration

**Interactive Learning:**
- Modify policies and see effects
- Test different scenarios
- Compare policy configurations

**Compliance Education:**
- Industry-specific regulations
- Policy best practices
- Audit trail importance

### Determinism Demonstrations

**Bit-Identical Results:**
- Multiple execution runs
- Hash comparison
- Result verification

**Temporal Determinism:**
- Execution timing consistency
- Performance predictability
- Resource usage patterns

**Cross-Platform Consistency:**
- Different system verification
- Architecture independence
- Result portability

### Compliance Showcases

**Financial Services:**
- SEC compliance examples
- Risk management policies
- Trading decision auditing
- Regulatory reporting

**Healthcare:**
- HIPAA compliance
- Patient privacy protection
- Clinical decision support
- Research data governance

**Government:**
- Access control policies
- Classification handling
- Audit requirements
- Security clearances

### Error Handling

- **Demo Setup Issues**: Clear configuration guidance
- **Policy Errors**: Syntax and logic problem explanation
- **Model Loading**: File access and format issues
- **Execution Failures**: Debugging and troubleshooting help

### Learning Paths

**Beginner Path:**
1. Basic governance demo
2. Simple policy creation
3. Determinism verification
4. Audit trail exploration

**Intermediate Path:**
1. Advanced policy scenarios
2. Industry compliance examples
3. Interactive policy testing
4. Performance analysis

**Advanced Path:**
1. Complex policy composition
2. Multi-level security models
3. Regulatory compliance deep dive
4. Custom policy development

### Integration

This skill wraps T81's demonstration and educational tools:
```bash
t81 demo governance --policy <policy> --interactive --verbose
```
