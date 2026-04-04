# T81 Foundation Skills for OpenClaw

This directory contains OpenClaw skills that provide access to T81 Foundation's deterministic runtime capabilities through OpenClaw's skill system.

## Available Skills

### Core Runtime Skills

**T81VM Runner (`t81_vm_runner`)**
- **Purpose**: Execute T81Lang programs through the T81VM
- **Commands**: `run`, `debug`, `trace`, `inspect`, `run-batch`
- **Use Case**: Deterministic program execution with policy enforcement

**T81 Compiler (`t81_compiler`)**
- **Purpose**: Compile T81Lang source to TISC bytecode
- **Commands**: `build`, `compile`, `disasm`, `optimize`, `build-batch`, `analyze`
- **Use Case**: Complete development workflow for T81 programs

### AI & Decision Skills

**AI Task Chains (`t81_ai_tasks`)**
- **Purpose**: Execute bounded AI decision workflows
- **Commands**: `assess`, `route`, `classify`, `chain`, `status`, `history`
- **Use Case**: Deterministic AI decision-making with provenance

**Model Loader (`t81_model_loader`)**
- **Purpose**: Import external AI models into T81 ecosystem
- **Commands**: `load-hf`, `load-gguf`, `load-safetensors`, `probe-model`, `validate-model`, `convert-model`, `batch-load`
- **Use Case**: Bridge external models to T81's immutable storage

### Storage & Data Skills

**CanonFS Import (`t81_canonfs_import`)**
- **Purpose**: Import files into T81's immutable storage system
- **Commands**: `import`, `import-batch`, `list`
- **Use Case**: Store files with cryptographic verification and provenance

**CanonFS Export (`t81_canonfs_export`)**
- **Purpose**: Export files from CanonFS with hash verification
- **Commands**: `export`, `export-batch`, `verify-export`, `list`, `search`, `export-with-meta`
- **Use Case**: Retrieve immutable files with integrity validation

**Tensor Operations (`t81_tensor_ops`)**
- **Purpose**: Perform deterministic tensor operations with ternary precision
- **Commands**: `tensor-math`, `tensor-reshape`, `tensor-validate`, `tensor-convert`, `tensor-stats`, `tensor-batch`
- **Use Case**: Mathematical operations with bit-identical results

### Governance & Security Skills

**Policy Checker (`t81_policy_checker`)**
- **Purpose**: Check Axion policy compliance for T81 operations
- **Commands**: `check`, `validate-policy`, `test-rule`, `list-policies`, `simulate`
- **Use Case**: Validate operations against governance policies

**Bundle Validator (`t81_bundle_validator`)**
- **Purpose**: Validate and verify T81 decision bundles
- **Commands**: `validate`, `verify-provenance`, `check-determinism`, `validate-batch`
- **Use Case**: Ensure decision bundle integrity and compliance

**Bundle Creator (`t81_bundle_creator`)**
- **Purpose**: Create decision bundles with complete provenance
- **Commands**: `create-bundle`, `bundle-from-execution`, `bundle-info`, `sign-bundle`, `verify-bundle`, `merge-bundles`, `extract-bundle`
- **Use Case**: Package decisions with cryptographic verification

### Development & Analysis Skills

**T81 Debugger (`t81_debugger`)**
- **Purpose**: Debug T81 execution with full traceability
- **Commands**: `debug`, `breakpoint`, `step`, `continue`, `inspect`, `trace`, `analyze-violations`, `profile`
- **Use Case**: Analyze execution flows and policy violations

**Governance Demo (`t81_governance_demo`)**
- **Purpose**: Demonstrate policy-gated computation
- **Commands**: `demo-governance`, `demo-policy`, `demo-determinism`, `tutorial-policy`, `interactive-policy`, `compliance-demo`
- **Use Case**: Educational examples of T81's core value proposition

## Installation

1. Copy this directory to your OpenClaw workspace:
   ```bash
   cp -r /path/to/t81-foundation/skills ~/.openclaw/workspace/skills/
   ```

2. Restart OpenClaw gateway:
   ```bash
   openclaw gateway restart
   ```

3. Verify skills are loaded:
   ```bash
   openclaw skills list
   ```

## Prerequisites

- T81 Foundation CLI installed and available in PATH
- CanonFS root directory (default: `~/.t81_canonfs`)
- Appropriate Axion policy files for your use case

## Configuration

Set environment variables for customization:

```bash
export T81_CANONFS_ROOT="/path/to/canonfs"
export T81_POLICY_PATH="/path/to/policies"
```

## Usage Examples

### CanonFS Import
```bash
# Import a model file
import model.t81w --policy secure.apl

# List imported objects
list --format json
```

### Bundle Validation
```bash
# Validate a decision bundle
validate decision_bundle.v1.json --strict

# Verify provenance chain
verify-provenance bundle.v1.json --canonfs-root /secure/canonfs
```

### Policy Checking
```bash
# Check operation compliance
check import model.t81w --policy security.apl

# Simulate policy decision
simulate inference neural_net.t81w --dry-run
```

## Security Considerations

- All operations are subject to Axion policy validation
- Files are stored with content-addressed hashing
- Complete audit trails are maintained for compliance
- Skills never modify T81 core components

## Integration Architecture

```
OpenClaw Agent → Skill Wrapper → T81 CLI → T81 Core Components
```

The skills act as bridges between OpenClaw's `exec` tool and T81's command-line interface, maintaining:

- **Deterministic execution**: Same inputs produce identical outputs
- **Policy enforcement**: All operations validated by Axion
- **Immutable provenance**: Complete audit trails for all operations
- **Cross-system portability**: Skills work across different environments

## Error Handling

All skills provide:
- Clear error messages with specific error types
- Suggestions for resolving common issues
- JSON output for machine parsing
- Deterministic error reporting

## Development

To add new skills:

1. Create directory: `skills/new-skill/`
2. Add `SKILL.md` with proper metadata
3. Implement wrapper script that calls T81 CLI
4. Test with various input scenarios
5. Update this README

## Support

For issues with:
- **T81 Foundation**: Check T81 documentation and issue tracker
- **OpenClaw Skills**: Report issues in this repository
- **Integration**: Verify T81 CLI installation and permissions
