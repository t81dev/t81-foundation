# T81 Foundation Skills for OpenClaw

This directory contains OpenClaw skills that provide access to T81 Foundation's deterministic runtime capabilities through OpenClaw's skill system.

## Available Skills

### Core Runtime Skills

**T81VM Runner (`t81_vm_runner`)**
- **Purpose**: Execute T81Lang programs through T81VM with deterministic guarantees
- **Commands**: `run`, `debug`, `trace`, `inspect`, `run-batch`
- **Key Features**: Policy enforcement, breakpoint management, execution tracing, batch processing
- **Use Cases**: Program execution, debugging, performance analysis, batch workflows
- **Integration**: Direct bridge to T81VM with Axion policy validation

**T81 Compiler (`t81_compiler`)**
- **Purpose**: Compile T81Lang source to TISC bytecode with optimization
- **Commands**: `build`, `compile`, `disasm`, `optimize`, `build-batch`, `analyze`
- **Key Features**: Multi-target compilation, ternary optimization, disassembly, batch builds
- **Use Cases**: Development workflow, performance optimization, code analysis
- **Integration**: Complete development pipeline with TISC ISA v1.9.0 support

### AI & Decision Skills

**AI Task Chains (`t81_ai_tasks`)**
- **Purpose**: Execute bounded AI decision workflows with deterministic guarantees
- **Commands**: `assess`, `route`, `classify`, `chain`, `status`, `history`
- **Key Features**: Fixed decision types, chain composition, provenance tracking
- **Use Cases**: Risk assessment, workflow routing, data classification, decision auditing
- **Integration**: Bounded AI OS-object family with complete audit trails

**Model Loader (`t81_model_loader`)**
- **Purpose**: Import external AI models into T81 with secure conversion
- **Commands**: `load-hf`, `load-gguf`, `load-safetensors`, `probe-model`, `validate-model`, `convert-model`, `batch-load`
- **Key Features**: Format conversion, security scanning, deterministic validation, batch processing
- **Supported Formats**: Hugging Face, GGUF, SafeTensors, PyTorch, TensorFlow, ONNX
- **Use Cases**: Model integration, format migration, security validation, bulk conversion
- **Integration**: CanonFS storage with cryptographic verification

### Storage & Data Skills

**CanonFS Import (`t81_canonfs_import`)**
- **Purpose**: Import files into T81's immutable, hash-verified storage system
- **Commands**: `import`, `import-batch`, `list`
- **Key Features**: Content-addressed storage, cryptographic hashing, policy validation
- **Use Cases**: Secure file storage, batch imports, provenance establishment
- **Integration**: CanonFS with CanonHash81 verification

**CanonFS Export (`t81_canonfs_export`)**
- **Purpose**: Export files from CanonFS with hash verification and integrity validation
- **Commands**: `export`, `export-batch`, `verify-export`, `list`, `search`, `export-with-meta`
- **Key Features**: Integrity verification, metadata export, search capabilities, batch operations
- **Use Cases**: File retrieval, integrity validation, metadata extraction, bulk exports
- **Integration**: CanonFS with complete provenance chains

**Tensor Operations (`t81_tensor_ops`)**
- **Purpose**: Perform deterministic tensor operations with ternary precision
- **Commands**: `tensor-math`, `tensor-reshape`, `tensor-validate`, `tensor-convert`, `tensor-stats`, `tensor-batch`
- **Key Features**: Ternary arithmetic, bit-identical results, format conversion, batch processing
- **Supported Operations**: Arithmetic, unary, comparison, aggregation operations
- **Use Cases**: Mathematical computations, model preprocessing, data analysis, batch transformations
- **Integration**: Deterministic math with cross-platform consistency

### Governance & Security Skills

**Policy Checker (`t81_policy_checker`)**
- **Purpose**: Check Axion policy compliance for T81 operations and decisions
- **Commands**: `check`, `validate-policy`, `test-rule`, `list-policies`, `simulate`
- **Key Features**: Policy validation, rule testing, simulation, dry-run execution
- **Use Cases**: Compliance checking, policy testing, security validation, risk assessment
- **Integration**: Axion governance kernel with pre-execution enforcement

**Bundle Validator (`t81_bundle_validator`)**
- **Purpose**: Validate and verify T81 decision bundles for deterministic execution
- **Commands**: `validate`, `verify-provenance`, `check-determinism`, `validate-batch`
- **Key Features**: Bundle integrity, provenance verification, determinism checking, batch validation
- **Use Cases**: Bundle validation, compliance verification, integrity checking, bulk processing
- **Integration**: Decision bundles with cryptographic verification

**Bundle Creator (`t81_bundle_creator`)**
- **Purpose**: Create decision bundles with complete provenance and cryptographic verification
- **Commands**: `create-bundle`, `bundle-from-execution`, `bundle-info`, `sign-bundle`, `verify-bundle`, `merge-bundles`, `extract-bundle`
- **Key Features**: Bundle creation, signing, verification, merging, component extraction
- **Bundle Structure**: Result, provenance, policy evidence, metadata, cryptographic hashes
- **Use Cases**: Decision packaging, compliance documentation, audit trail creation, bundle management
- **Integration**: Complete decision lifecycle with immutable storage

### Development & Analysis Skills

**T81 Debugger (`t81_debugger`)**
- **Purpose**: Debug T81 execution with full traceability and policy violation analysis
- **Commands**: `debug`, `breakpoint`, `step`, `continue`, `inspect`, `trace`, `analyze-violations`, `profile`
- **Key Features**: Breakpoint management, step execution, state inspection, performance profiling
- **Breakpoint Types**: Address, conditional, policy-based, function entry/exit
- **Use Cases**: Program debugging, performance analysis, policy violation investigation, optimization
- **Integration**: T81VM with complete execution visibility

**Governance Demo (`t81_governance_demo`)**
- **Purpose**: Demonstrate T81's policy-gated computation with educational examples
- **Commands**: `demo-governance`, `demo-policy`, `demo-determinism`, `tutorial-policy`, `interactive-policy`, `compliance-demo`
- **Key Features**: Interactive demos, policy tutorials, compliance examples, industry scenarios
- **Scenarios**: Basic governance, financial compliance, healthcare privacy, advanced security
- **Use Cases**: Education, compliance training, policy development, stakeholder demonstration
- **Integration**: Complete T81 value proposition demonstration

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

### Core Runtime Workflows

**T81VM Execution:**
```bash
# Basic program execution with policy enforcement
run hello_world.t81 --policy security.apl

# Debug with breakpoints and full tracing
debug complex_program.t81 --breakpoints 0x1000,0x2000 --trace-level full

# Batch process multiple programs
run-batch ./programs/ --recursive --policy batch.apl --output-dir ./results/
```

**T81 Compilation Pipeline:**
```bash
# Compile with optimization
build source.t81l --output program.tisc --optimization-level 3 --strategy ternary

# Disassemble existing bytecode
disasm program.tisc --format assembly --addresses

# Batch compilation
build-batch ./src/ --target ternary --format tisc --output-dir ./build/
```

### AI & Decision Workflows

**AI Task Chains:**
```bash
# Execute bounded assessment task
assess market_data.csv --model trading_model.t81w --policy risk_assessment.apl

# Route through decision workflow
route customer_request.json --routes routing_config.json --policy compliance.apl

# Execute complete decision chain
chain trading_workflow.json --input data.csv --bundle-output decisions.bundle

# Monitor task execution
status task_12345 --detailed --provenance
```

**Model Management:**
```bash
# Import Hugging Face model with security scanning
load-hf "bert-base-uncased" --output bert.t81w --policy model_import.apl

# Convert between formats
convert-model model.safetensors --to-format t81w --output model.t81w

# Batch model processing
batch-load ./models/ --recursive --output-dir ./converted/ --canonfs-root ./canonfs/

# Validate model integrity
validate-model model.t81w --check-determinism --security-scan
```

### Storage & Data Operations

**CanonFS Management:**
```bash
# Import with policy validation
import sensitive_data.csv --policy data_protection.apl --canonfs-root /secure/canonfs

# Export with integrity verification
export CanonHash81_ABC123... --output restored_file.t81w --verify

# Search and retrieve objects
search ".*\.t81w$" --field name --max-results 10
export-with-meta CanonHash81_DEF456... --include-provenance --output bundle_with_meta.zip
```

**Tensor Processing:**
```bash
# Deterministic tensor operations
tensor-math add tensor_a.t81w tensor_b.t81w --output result.t81w --precision ternary

# Batch tensor processing
tensor-batch normalize ./tensors/ --recursive --output-dir ./normalized/

# Convert tensor formats
tensor-convert model.gguf --to-format t81w --output model.t81w

# Analyze tensor properties
tensor-stats weights.t81w --detailed --histogram --output analysis.json
```

### Governance & Security Workflows

**Policy Compliance:**
```bash
# Check operation compliance
check import model.t81w --operation file_access --policy security.apl --resource /models/

# Validate policy syntax
validate-policy security_rules.apl --schema policy_schema.json

# Simulate policy decisions
simulate inference neural_net.t81w --policy compliance.apl --dry-run

# Test specific policy rules
test-rule "allow_if_user_role_admin" --context '{"user": {"role": "user"}}'
```

**Bundle Lifecycle:**
```bash
# Create bundle from execution
create-bundle execution_result.json --policy security.apl --include-provenance

# Validate bundle integrity
validate decision_bundle.v1.json --strict --check-determinism

# Sign bundle for distribution
sign-bundle decision.bundle --key private_key.pem --certificate cert.pem

# Merge multiple bundles
merge-bundles bundle1.bundle bundle2.bundle --output merged.bundle --conflict-resolution merge
```

### Development & Analysis Workflows

**Debugging and Analysis:**
```bash
# Start debugging session
debug program.tisc --policy debug.apl --breakpoints 0x1000,0x2000

# Step through execution
step --count 5 --into

# Inspect VM state
inspect --state registers,memory --address 0x1000

# Profile performance
profile --start --granularity instruction --output profile.json

# Analyze policy violations
analyze-violations --detailed --suggest-fixes --output violation_report.json
```

**Educational Demonstrations:**
```bash
# Basic governance demonstration
demo-governance --policy security.apl --model demo_model.t81w --interactive

# Industry-specific compliance demo
demo-policy --scenario financial --show-violations

# Determinism verification
demo-determinism --iterations 10 --compare-results

# Interactive policy learning
tutorial-policy --level intermediate --examples
```

## Security & Compliance Considerations

### T81 Security Guarantees
- **Deterministic Execution**: Same inputs always produce identical outputs across all platforms
- **Policy Enforcement**: All operations validated by Axion governance kernel before execution
- **Immutable Provenance**: Complete audit trails stored in CanonFS with cryptographic verification
- **Content-Addressed Storage**: Files identified by CanonHash81 for tamper-evidence
- **Zero Trust Architecture**: No implicit trust - all operations explicitly validated

### OpenClaw Security Integration
- **Skill Isolation**: Each skill operates in isolated environment with minimal permissions
- **Input Validation**: All wrapper scripts validate inputs and prevent injection attacks
- **Policy Boundaries**: Skills cannot bypass T81's governance mechanisms
- **Audit Logging**: All skill operations logged through T81's audit system
- **Secure Defaults**: Conservative default settings prevent accidental privilege escalation

### Compliance Framework Alignment
- **Financial Services**: SOX, Basel III, MiFID II compliance through deterministic audit trails
- **Healthcare**: HIPAA compliance through immutable provenance and access controls
- **Government**: FISMA compliance through cryptographic verification and audit logging
- **Data Protection**: GDPR compliance through data minimization and immutable storage

### Operational Security
- **Least Privilege**: Skills request minimum necessary permissions
- **Defense in Depth**: Multiple layers of security validation
- **Fail Secure**: Operations fail rather than bypass security controls
- **Transparent Security**: All security decisions logged and auditable

### Threat Mitigation
- **Code Injection**: Input sanitization and parameter validation
- **Privilege Escalation**: Policy-based access controls prevent unauthorized actions
- **Data Tampering**: CanonFS immutability prevents post-execution modification
- **Side-Channel Attacks**: Deterministic execution prevents timing-based attacks

## Integration Architecture

```
OpenClaw Agent → Skill Wrapper → T81 CLI → T81 Core Components
```

### Component Interactions

**OpenClaw Layer:**
- **Agent Interface**: Natural language processing and intent recognition
- **Skill Discovery**: Automatic skill loading and routing
- **Tool Execution**: Secure `exec` tool invocation with parameter validation
- **Session Management**: Context preservation across skill interactions

**Skill Wrapper Layer:**
- **Argument Parsing**: Robust command-line argument processing and validation
- **Error Handling**: Comprehensive error detection with user-friendly messages
- **Security Controls**: Input sanitization and policy boundary enforcement
- **JSON Output**: Machine-readable responses for integration consistency

**T81 CLI Layer:**
- **Command Dispatch**: Direct bridge to T81's native command-line interface
- **Policy Integration**: Automatic Axion policy loading and enforcement
- **Resource Management**: CanonFS access and bundle creation
- **Status Reporting**: Detailed operation feedback and telemetry

### Data Flow

```
User Request → OpenClaw Agent → Skill Selection → Argument Validation → T81 CLI → Core Component → Result Processing → Response
```

### Security Boundaries

**Skill Isolation:**
- Each skill operates in isolated execution environment
- No direct access to T81 core components
- All interactions mediated through T81 CLI

**Policy Enforcement:**
- Skills cannot bypass Axion policy validation
- All operations subject to governance rules
- Policy violations logged and reported

**Provenance Preservation:**
- All operations create audit trails in CanonFS
- Decision bundles maintain complete execution history
- Cryptographic verification ensures integrity

### Performance Characteristics

**Deterministic Guarantees:**
- Bit-identical results across all platforms
- Temporal consistency in execution timing
- Cross-platform reproducibility

**Resource Efficiency:**
- Minimal overhead from skill wrapper layer
- Direct T81 CLI integration
- Efficient memory and CPU utilization

**Scalability:**
- Batch processing capabilities for large workloads
- Parallel execution where supported
- Resource pooling and management

### Integration Benefits

**For Developers:**
- Conversational access to T81 capabilities
- Natural language interface for complex operations
- Reduced learning curve for T81 adoption

**For Organizations:**
- Rapid deployment through OpenClaw's skill system
- Consistent interface across T81 operations
- Built-in compliance and audit capabilities

**For Compliance:**
- Automatic audit trail generation
- Policy enforcement by default
- Cryptographic verification of all operations

## Error Handling & Troubleshooting

### Error Categories

**Configuration Errors:**
- **T81 CLI Not Found**: Install T81 Foundation and ensure in PATH
- **Permission Denied**: Check file permissions and user access rights
- **Policy File Missing**: Verify policy file paths and syntax
- **CanonFS Access Issues**: Check CanonFS root directory permissions

**Execution Errors:**
- **Program Not Found**: Verify file paths and extensions (.t81, .tisc, .t81w)
- **Policy Violations**: Review policy rules and adjust permissions
- **Memory Constraints**: Check available RAM for model loading
- **Model Loading Failures**: Validate model formats and integrity

**Network & Service Errors:**
- **Ollama Connection Failed**: Verify Ollama is running on localhost:11434
- **Model Download Issues**: Check internet connection and model availability
- **API Timeouts**: Increase timeout values or check service status

### Troubleshooting Workflows

**Basic Diagnostic:**
```bash
# Check T81 installation
t81 --version

# Verify Ollama status
ollama ps

# Test OpenClaw skills
openclaw skills list

# Check CanonFS access
ls -la ~/.t81_canonfs
```

**Policy Debugging:**
```bash
# Validate policy syntax
validate-policy security_rules.apl

# Test specific rule
test-rule "allow_file_access" --context '{"file": "/test.txt"}'

# Simulate policy decision
simulate operation --policy test.apl --dry-run
```

**Performance Issues:**
```bash
# Profile execution
profile --start --granularity instruction --output perf.json

# Check resource usage
inspect --state memory,registers --verbose

# Batch processing with monitoring
run-batch ./programs/ --monitor --output-dir ./results/
```

### Common Solutions

**T81 CLI Issues:**
```bash
# Reinstall T81 Foundation
brew reinstall t81-foundation  # macOS
# OR
sudo apt-get install t81-foundation  # Linux

# Update PATH
export PATH="$PATH:/usr/local/bin"
```

**OpenClaw Integration:**
```bash
# Restart gateway
openclaw gateway restart

# Clear skill cache
rm -rf ~/.openclaw/workspace/skills/cache/

# Reinstall skills
cp -r /path/to/t81-foundation/skills ~/.openclaw/workspace/skills/
```

**Model Loading Problems:**
```bash
# Check model integrity
validate-model model.t81w --check-determinism

# Convert problematic models
convert-model broken.gguf --to-format t81w --output fixed.t81w

# Use batch processing for reliability
batch-load ./models/ --validate-each --output-dir ./converted/
```

### Support Channels

**Documentation:**
- T81 Foundation: `/Users/t81dev/Code/t81-foundation/docs/`
- OpenClaw Skills: Individual skill `SKILL.md` files
- API Reference: `t81 --help` and `openclaw --help`

**Community Resources:**
- T81 Issues: GitHub repository issue tracker
- OpenClaw Support: Official documentation and Discord
- Skill Development: OpenClaw skill creation guidelines

**Debug Information Collection:**
```bash
# Enable verbose logging
export T81_DEBUG=1
export OPENCLAW_DEBUG=1

# Collect diagnostic information
t81 diagnose --all
openclaw config show

# Generate support bundle
create-bundle diagnostic_info.json --output support_request.bundle
```

## Development & Extension

### Creating New Skills

**Skill Structure:**
```
skills/new-skill/
├── SKILL.md              # Skill metadata and documentation
├── main-command.sh       # Primary wrapper script
├── subcommand1.sh       # Symlink to main (optional)
├── subcommand2.sh       # Symlink to main (optional)
└── README.md             # Skill-specific documentation (optional)
```

**SKILL.md Template:**
```yaml
---
name: t81_new_skill
description: Brief description of skill purpose
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# New Skill Name

This skill provides [specific capability] for T81 Foundation through OpenClaw.

## Usage

When [condition], use this skill to [action].

### Commands

**Primary command:**
```
main-command <required_arg> [--optional-arg <value>] [--flag]
```

**Subcommands (if applicable):**
```
subcommand1 <args>    # Symlink to main-command.sh
subcommand2 <args>    # Symlink to main-command.sh
```

### Examples

```bash
# Basic usage
main-command input.t81 --policy security.apl

# With optional parameters
main-command data.json --output result.json --verbose

# Subcommand usage
subcommand1 --specific-flag value
```

### Output Format

```json
{
  "status": "success|error",
  "result": "operation result",
  "timestamp": "2026-04-04T15:30:00Z",
  "details": {
    "specific": "operation details"
  }
}
```

### Error Handling

- [Common error 1]: Description and resolution
- [Common error 2]: Description and resolution
```

**Wrapper Script Template:**
```bash
#!/bin/bash

# T81 New Skill Wrapper
# Bridges OpenClaw skill to T81 functionality

set -euo pipefail

# Default values
DEFAULT_VALUE=""
OPTIONAL_VALUE=""
VERBOSE=""

# Parse arguments
REQUIRED_ARG=""
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --optional)
            ((i++))
            OPTIONAL_VALUE="${ARGS[$i]}"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$REQUIRED_ARG" ]; then
                REQUIRED_ARG="${ARGS[$i]}"
            else
                echo "Error: Too many arguments" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate required arguments
if [ -z "$REQUIRED_ARG" ]; then
    echo "Error: Missing required argument" >&2
    echo "Usage: main-command <required_arg> [--optional <value>]" >&2
    exit 1
fi

# Check if T81 CLI is available
if ! command -v t81 &> /dev/null; then
    echo "Error: T81 CLI not found. Please install T81 Foundation." >&2
    exit 1
fi

# Build T81 command
T81_CMD="t81 new-command \"$REQUIRED_ARG\""

if [ -n "$OPTIONAL_VALUE" ]; then
    T81_CMD="$T81_CMD --optional \"$OPTIONAL_VALUE\""
fi

if [ -n "$VERBOSE" ]; then
    T81_CMD="$T81_CMD $VERBOSE"
fi

# Execute T81 command
if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "New skill operation completed successfully" >&2
    fi
else
    echo "Error: New skill operation failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
```

### Development Workflow

**1. Skill Planning:**
- Identify T81 functionality to expose
- Define command interface and arguments
- Plan error handling and edge cases
- Consider security implications

**2. Implementation:**
- Create skill directory structure
- Write comprehensive SKILL.md documentation
- Implement robust wrapper script
- Add input validation and error handling

**3. Testing:**
```bash
# Test basic functionality
openclaw agent -m "test new skill with basic input"

# Test error conditions
openclaw agent -m "test new skill with invalid input"

# Test optional parameters
openclaw agent -m "test new skill with all options"
```

**4. Integration Testing:**
```bash
# Verify skill discovery
openclaw skills list | grep new_skill

# Test through OpenClaw gateway
openclaw agent --agent main -m "use new-skill command"

# Test with different OpenClaw agents
openclaw agent --agent ops -m "use new-skill command"
```

**5. Documentation Updates:**
- Update main skills README.md
- Add skill to appropriate category
- Update installation instructions
- Document any prerequisites

### Best Practices

**Security:**
- Validate all user inputs
- Use absolute paths for file operations
- Implement principle of least privilege
- Sanitize data passed to T81 CLI

**Usability:**
- Provide clear error messages
- Include usage examples in SKILL.md
- Support both short and long argument formats
- Enable verbose output for debugging

**Integration:**
- Follow OpenClaw skill metadata standards
- Use JSON output for machine readability
- Handle T81 CLI errors gracefully
- Maintain compatibility across platforms

**Performance:**
- Minimize wrapper script overhead
- Use efficient argument parsing
- Avoid unnecessary file operations
- Leverage T81 CLI capabilities directly

### Extension Points

**Advanced Skill Features:**
- Batch processing capabilities
- Interactive mode support
- Configuration file integration
- Plugin-style extensibility
- Custom output formats

**Integration Opportunities:**
- Combine multiple T81 operations
- Chain skills together for workflows
- Integrate with external tools
- Provide automation capabilities

### Contributing Guidelines

**Code Standards:**
- Follow shell script best practices
- Use consistent error handling patterns
- Maintain compatibility with existing skills
- Document all functions and parameters

**Documentation Standards:**
- Use consistent SKILL.md format
- Provide comprehensive examples
- Include troubleshooting information
- Document all options and parameters

**Testing Requirements:**
- Test on multiple platforms (macOS, Linux)
- Verify with different T81 versions
- Test error conditions and edge cases
- Validate OpenClaw integration

## Support

For issues with:
- **T81 Foundation**: Check T81 documentation and issue tracker
- **OpenClaw Skills**: Report issues in this repository
- **Integration**: Verify T81 CLI installation and permissions
