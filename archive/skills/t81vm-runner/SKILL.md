---
name: t81_vm_runner
description: Execute T81 code with deterministic guarantees and policy enforcement
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81VM Runner Skill

This skill executes T81Lang programs through the T81VM with deterministic guarantees, policy enforcement, and complete audit trails.

## Usage

When the user needs to run T81 code with guaranteed determinism and governance, use this skill to execute programs through the T81VM.

### Commands

**Run a T81 program:**
```
run <program_file> [--policy <policy_file>] [--weights-model <model_file>] [--trace] [--json]
```

**Run with debug information:**
```
debug <program_file> [--breakpoints <addr1,addr2>] [--policy <policy_file>]
```

**Batch execution:**
```
run-batch <program_dir> [--recursive] [--policy <policy_file>] [--output-dir <dir>]
```

**Trace execution:**
```
trace <program_file> [--trace-level basic|full|policy] [--output <trace_file>]
```

**Inspect VM state:**
```
inspect <program_file> [--state registers|memory|stack] [--address <addr>]
```

### Examples

```bash
# Basic program execution
run hello_world.t81

# Run with policy enforcement and model weights
run inference.t81 --policy security.apl --weights-model model.t81w

# Debug with breakpoints
debug complex_program.t81 --breakpoints 0x1000,0x2000 --policy debug.apl

# Full execution trace
trace inference.t81 --trace-level full --output execution_trace.json

# Batch process all programs
run-batch ./programs/ --recursive --policy batch.apl --output-dir ./results/

# Inspect VM registers after execution
inspect program.t81 --state registers
```

### Execution Modes

**Standard Mode:**
- Deterministic execution with policy enforcement
- Bit-identical results for same inputs
- Complete audit trail generation

**Debug Mode:**
- Step-through execution with breakpoints
- Register and memory inspection
- Policy violation analysis

**Trace Mode:**
- Detailed execution flow recording
- Policy decision logging
- Performance metrics collection

### Output Format

**Successful Execution:**
```json
{
  "status": "completed",
  "program": "inference.t81",
  "execution_id": "CanonHash81...",
  "timestamp": "2026-04-04T15:30:00Z",
  "result": {
    "output": "tensor_result_data",
    "exit_code": 0,
    "determinism_hash": "CanonHash81..."
  },
  "policy": {
    "enforced": true,
    "violations": 0,
    "decisions_made": 1247
  },
  "performance": {
    "instructions_executed": 5421,
    "execution_time_ms": 23,
    "memory_peak_bytes": 1048576
  }
}
```

**Policy Violation:**
```json
{
  "status": "policy_violation",
  "program": "unauthorized.t81",
  "execution_id": "CanonHash81...",
  "timestamp": "2026-04-04T15:30:00Z",
  "error": {
    "type": "policy_denial",
    "instruction": 0x1234,
    "reason": "unauthorized_resource_access",
    "policy_rule": "deny_file_write"
  },
  "policy": {
    "enforced": true,
    "violations": 1,
    "decision_point": "instruction_0x1234"
  }
}
```

### Supported File Types

**T81Lang Source (.t81):**
- High-level ternary language programs
- Requires compilation before execution

**TISC Bytecode (.tisc):**
- Compiled T81 instruction set
- Direct execution by T81VM

**Bundle Files (.bundle):**
- Decision bundles with provenance
- Executed with bundle consumption contract

### Determinism Guarantees

- **Bit-identical execution**: Same inputs produce identical outputs
- **Reproducible traces**: Execution logs are deterministic
- **Policy consistency**: Same policy decisions for identical states
- **Temporal determinism**: Execution timing is predictable

### Error Handling

- **Compilation errors**: Clear syntax and semantic error reporting
- **Runtime errors**: Detailed exception information with stack traces
- **Policy violations**: Specific rule violations with remediation suggestions
- **Resource errors**: Memory and resource limit handling

### Security Notes

- All execution subject to Axion policy validation
- No side effects without policy approval
- Complete audit trail for compliance
- Sandboxed execution environment

### Integration

This skill wraps the T81VM execution engine:
```bash
t81 vm run <program> --policy <policy> --trace --json
```
