---
name: t81_compiler
description: Compile T81Lang source code to TISC bytecode with deterministic optimization
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 Compiler Skill

This skill compiles T81Lang source code to TISC bytecode with deterministic optimization, policy-aware compilation, and complete build provenance.

## Usage

When the user needs to compile T81Lang programs to executable TISC bytecode, use this skill for deterministic compilation with optimization.

### Commands

**Compile a T81 program:**
```
build <source_file> [--output <output_file>] [--optimize level] [--json]
```

**Compile with specific optimization:**
```
compile <source_file> [--optimization space|time|determinism] [--target arch]
```

**Disassemble bytecode:**
```
disasm <bytecode_file> [--format binary|ternary|mixed] [--addresses]
```

**Optimize existing bytecode:**
```
optimize <bytecode_file> [--level 1|2|3] [--preserve-semantics]
```

**Batch compilation:**
```
build-batch <source_dir> [--recursive] [--output-dir <dir>]
```

**Analyze compilation:**
```
analyze <source_file> [--metrics] [--complexity] [--policy-check]
```

### Examples

```bash
# Basic compilation
build hello_world.t81

# Compile with optimization and custom output
build inference.t81 --output inference_opt.tisc --optimize 2

# Compile for deterministic execution
compile secure_code.t81 --optimization determinism --target t81v1

# Disassemble with addresses
disasm program.tisc --format mixed --addresses

# Optimize existing bytecode
optimize basic.tisc --level 3 --preserve-semantics

# Batch compile directory
build-batch ./src/ --recursive --output-dir ./build/

# Analyze compilation metrics
analyze complex_program.t81 --metrics --complexity
```

### Compilation Pipeline

**1. Parsing & Semantic Analysis:**
- T81Lang syntax validation
- Type checking and inference
- Policy constraint analysis

**2. Intermediate Representation:**
- T81IR generation
- Optimization passes
- Determinism verification

**3. Code Generation:**
- TISC bytecode emission
- Register allocation
- Instruction scheduling

**4. Post-Processing:**
- Bytecode validation
- Hash generation
- Provenance recording

### Optimization Levels

**Level 1 (Basic):**
- Dead code elimination
- Constant folding
- Basic block reordering

**Level 2 (Standard):**
- Loop optimizations
- Inlining
- Register allocation improvements

**Level 3 (Aggressive):**
- Advanced inlining
- Memory layout optimization
- Ternary-specific optimizations

### Optimization Strategies

**Space Optimization:**
- Code size reduction
- Memory footprint minimization
- Compact encoding

**Time Optimization:**
- Execution speed improvement
- Cache-friendly layout
- Parallel execution opportunities

**Determinism Optimization:**
- Bit-identical result preservation
- Temporal determinism
- Policy-aware compilation

### Output Format

**Successful Compilation:**
```json
{
  "status": "success",
  "source": "inference.t81",
  "output": "inference_opt.tisc",
  "compilation_id": "CanonHash81...",
  "timestamp": "2026-04-04T15:30:00Z",
  "compiler": {
    "version": "1.9.0",
    "optimization_level": 2,
    "target_arch": "t81v1"
  },
  "metrics": {
    "source_lines": 1247,
    "bytecode_size": 8192,
    "compilation_time_ms": 45,
    "optimization_reduction": 23
  },
  "provenance": {
    "source_hash": "CanonHash81...",
    "bytecode_hash": "CanonHash81...",
    "compilation_env": "t81_v1.9.0"
  }
}
```

**Compilation Error:**
```json
{
  "status": "error",
  "source": "invalid.t81",
  "error": {
    "type": "syntax_error",
    "line": 42,
    "column": 15,
    "message": "Unexpected token '}' in function definition",
    "context": "function calculate_result() {"
  },
  "suggestions": [
    "Check for missing semicolon before '}'",
    "Verify function parameter syntax",
    "Ensure balanced braces"
  ]
}
```

### Supported File Types

**Input Files:**
- `.t81` - T81Lang source code
- `.t81ir` - T81 intermediate representation

**Output Files:**
- `.tisc` - TISC bytecode
- `.t81opt` - Optimized bytecode
- `.json` - Compilation metadata

### Language Features

**T81Lang Constructs:**
- Ternary data types and operations
- Policy-gated function calls
- Deterministic control flow
- Tensor operations

**Policy Integration:**
- Compile-time policy checking
- Policy-aware optimizations
- Secure compilation modes

### Error Handling

- **Syntax errors**: Detailed location and context information
- **Semantic errors**: Type checking and validation failures
- **Optimization errors**: Constraint violation handling
- **Policy errors**: Governance compliance issues

### Security Notes

- Compilation respects policy constraints
- No unsafe optimizations without approval
- Complete build provenance maintained
- Deterministic compilation guarantees

### Integration

This skill wraps the T81 compiler toolchain:
```bash
t81 code build <source> --output <output> --optimize <level> --json
```
