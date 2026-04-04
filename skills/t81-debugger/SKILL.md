---
name: t81_debugger
description: Debug T81 execution with full traceability and policy violation analysis
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 Debugger Skill

This skill provides comprehensive debugging capabilities for T81 execution with full traceability, breakpoint management, and policy violation analysis.

## Usage

When the user needs to debug T81 program execution or analyze policy violations, use this skill for detailed execution analysis.

### Commands

**Start debugging session:**
```
debug <program_file> [--policy <policy_file>] [--breakpoints <addr1,addr2>] [--trace-level basic|full|policy]
```

**Set breakpoints:**
```
breakpoint set <address> [--condition <condition>] [--policy-only]
```

**Remove breakpoints:**
```
breakpoint remove <address> [--all]
```

**List breakpoints:**
```
breakpoint list [--enabled-only]
```

**Step execution:**
```
step [--count <n>] [--into] [--over] [--out]
```

**Continue execution:**
```
continue [--to <address>] [--until <condition>]
```

**Inspect state:**
```
inspect [--state registers|memory|stack|policy] [--address <addr>] [--range <start-end>]
```

**Trace execution:**
```
trace [--start] [--stop] [--save <trace_file>] [--filter policy|memory|instructions]
```

**Analyze policy violations:**
```
analyze-violations [--detailed] [--suggest-fixes] [--output <report_file>]
```

**Profile performance:**
```
profile [--start] [--stop] [--output <profile_file>] [--granularity instruction|function|policy]
```

### Examples

```bash
# Start debugging with breakpoints
debug program.tisc --policy security.apl --breakpoints 0x1000,0x2000

# Set conditional breakpoint
breakpoint set 0x1234 --condition "register.r1 > 100" --policy-only

# Step through execution
step --count 5 --into

# Continue to specific address
continue --to 0x2000

# Inspect VM state
inspect --state registers,memory --address 0x1000

# Start execution tracing
trace --start --filter policy --save execution_trace.json

# Analyze policy violations
analyze-violations --detailed --suggest-fixes --output violation_report.json

# Profile execution performance
profile --start --granularity instruction
```

### Debugging Features

**Breakpoint Management:**
- Address-based breakpoints
- Conditional breakpoints
- Policy violation breakpoints
- Function entry/exit breakpoints

**Execution Control:**
- Step-by-step execution
- Step into/over/out
- Continue to address/condition
- Pause and resume

**State Inspection:**
- Register values
- Memory contents
- Stack frames
- Policy state

**Trace Analysis:**
- Instruction-level tracing
- Policy decision logging
- Memory access tracking
- Performance profiling

### Breakpoint Types

**Address Breakpoints:**
- Stop at specific memory addresses
- Support for multiple breakpoints
- Enable/disable individual breakpoints

**Conditional Breakpoints:**
- Stop based on register values
- Memory content conditions
- Policy state conditions

**Policy Breakpoints:**
- Stop on policy violations
- Stop on policy checks
- Filter by policy type

### Output Format

**Debug Session Start:**
```json
{
  "session_id": "CanonHash81_DEBUG...",
  "program": "program.tisc",
  "policy": "security.apl",
  "timestamp": "2026-04-04T15:30:00Z",
  "breakpoints_set": 2,
  "vm_state": {
    "registers": {
      "r0": 0,
      "r1": 1024,
      "r2": 2048,
      "pc": 4096
    },
    "memory_size": 1048576,
    "stack_pointer": 1048576
  }
}
```

**Execution Step Result:**
```json
{
  "step_count": 1,
  "instruction": {
    "address": 4096,
    "opcode": "ADD_TERNARY",
    "operands": ["r1", "r2", "r3"],
    "disassembly": "ADD_TERNARY r1, r2, r3"
  },
  "vm_state": {
    "registers": {
      "r0": 0,
      "r1": 1024,
      "r2": 2048,
      "r3": 3072,
      "pc": 4097
    }
  },
  "policy_check": {
    "checked": true,
    "allowed": true,
    "policy_rule": "allow_arithmetic"
  }
}
```

**Policy Violation Analysis:**
```json
{
  "violations_found": 2,
  "violations": [
    {
      "address": 5120,
      "instruction": "WRITE_MEMORY",
      "policy_violation": "unauthorized_memory_write",
      "policy_rule": "deny_write_to_protected_memory",
      "context": {
        "target_address": 0x80000,
        "access_type": "write",
        "user_permissions": "read_only"
      },
      "suggestions": [
        "Check if memory access is necessary",
        "Request write permissions through policy",
        "Use alternative memory region"
      ]
    }
  ],
  "remediation_steps": [
    "Review policy configuration",
    "Update program to avoid protected memory",
    "Request policy exception if needed"
  ]
}
```

### Trace Analysis

**Trace Levels:**
- **Basic**: Instruction addresses and outcomes
- **Full**: Complete instruction details and state changes
- **Policy**: All policy decisions and violations

**Trace Filters:**
- **Policy**: Only policy-related events
- **Memory**: Memory access operations
- **Instructions**: All instruction execution

### Performance Profiling

**Profiling Granularity:**
- **Instruction**: Individual instruction timing
- **Function**: Function-level performance
- **Policy**: Policy check overhead

**Performance Metrics:**
- Execution time per instruction
- Memory access patterns
- Policy check overhead
- Cache miss rates

### Error Handling

- **Breakpoint Errors**: Invalid address handling
- **State Errors**: Memory access validation
- **Policy Errors**: Violation analysis failures
- **Trace Errors**: Storage and formatting issues

### Security Considerations

**Debug Access Control:**
- Policy-based debugging permissions
- Secure breakpoint management
- Protected memory inspection limits

**Trace Security:**
- Sensitive data filtering
- Secure trace storage
- Audit trail for debugging sessions

### Integration

This skill wraps T81's debugging and tracing capabilities:
```bash
t81 debug <program> --policy <policy> --breakpoints <addrs> --trace-level <level>
```
