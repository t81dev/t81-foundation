# T81 AI CLI Tools - RFC-00A7 Task 10

This directory contains the core AI CLI commands that provide the complete developer experience surface for T81 AI integration.

## Components

### T81 AI CLI (`t81_ai`)
Unified command-line interface for all AI operations with deterministic execution guarantees.

**Usage:**
```bash
# Run inference
./t81_ai run --model llama-7b --prompt "Hello, world!"

# Run benchmarks
./t81_ai benchmark --model llama-7b --verbose

# Quantize model
./t81_ai quantize --input model.gguf --codec T3_K2

# Verify model integrity
./t81_ai verify --model llama-7b --deterministic

# Inspect model metadata
./t81_ai model inspect --model llama-7b

# Test policy rules
./t81_ai policy test --type model_load
```

## Core Commands

### `t81_ai run`
**Purpose**: Execute AI inference with deterministic guarantees
**Options**:
- `--model <id>`: Model identifier (required)
- `--prompt <text>`: Input prompt for inference (required)
- `--deterministic`: Enable strict deterministic mode
- `--verbose`: Enable verbose output
- `--max-tokens <n>`: Maximum tokens to generate (default: 100)
- `--temperature <f>`: Sampling temperature (default: 0.0)

**Output**: Generated text, token count, timing metrics

### `t81_ai benchmark`
**Purpose**: Run comprehensive benchmark suite
**Options**:
- `--model <id>`: Model identifier (required)
- `--verbose`: Enable detailed output

**Output**: Performance metrics across inference, quantization, and memory

### `t81_ai quantize`
**Purpose**: Quantize models using T81 ternary codecs
**Options**:
- `--input <file>`: Input model file (required)
- `--output <file>`: Output quantized model file
- `--codec <type>`: Quantization codec (default: T3_K2)
- `--verbose`: Enable detailed output

**Output**: Compression ratio, accuracy impact, processing time

### `t81_ai verify`
**Purpose**: Verify model integrity and determinism
**Options**:
- `--model <id>`: Model identifier (required)
- `--deterministic`: Enable strict validation
- `--verbose`: Enable detailed output

**Output**: Hash verification, signature validation, determinism results

### `t81_ai model inspect`
**Purpose**: Inspect model metadata and properties
**Options**:
- `--model <id>`: Model identifier (required)
- `--verbose`: Enable detailed output

**Output**: Complete model metadata including size, format, creation info

### `t81_ai policy test`
**Purpose**: Test policy rules and validation
**Options**:
- `--type <event_type>`: Policy event type to test
- `--verbose`: Enable detailed output

**Output**: Policy evaluation results and applied rules

## Global Options

### Determinism Control
- `--deterministic`: Enable strict deterministic execution
- Ensures bit-exact reproducibility across all platforms
- Disables any non-deterministic optimizations

### Verbose Output
- `--verbose`: Enable detailed logging and metrics
- Shows additional debugging information
- Displays internal processing details

### Performance Tuning
- `--max-tokens <n>`: Limit token generation
- `--temperature <f>`: Control sampling randomness
- Affects inference behavior and output quality

## Output Format

### Success Indicators
- **✓**: Successful operation completion
- **ℹ**: Informational message
- **✗**: Error or failure

### Metrics Display
- **Timing**: Millisecond precision for all operations
- **Throughput**: Tokens per second and per minute
- **Memory**: Peak and average memory usage
- **Quality**: Accuracy and compression metrics

### Report Generation
- **JSON Reports**: Detailed machine-readable reports
- **Human-Readable**: Formatted console output
- **Audit Trails**: Complete operation logging

## Integration with T81 Ecosystem

### Backend Integration
- **Model Loading**: Uses model provenance system (RFC-00A3)
- **Inference Execution**: Leverages backend adapter (RFC-00A5)
- **Quantization**: Integrates ternary codecs (RFC-00A4)

### Determinism Framework
- **Evidence Collection**: Automatic evidence gathering (RFC-00A1)
- **Validation**: Built-in determinism verification
- **Reporting**: Comprehensive validation reports

### Policy Enforcement
- **Security Checks**: Policy validation before operations (RFC-00A6)
- **Audit Logging**: Complete operation audit trail
- **Access Control**: Role-based permission validation

### Benchmark Integration
- **Standard Metrics**: Uses benchmark suite (RFC-00A2)
- **Cross-Platform**: Consistent results across platforms
- **Performance Tracking**: Historical performance data

## Acceptance Criteria

- [x] All specified CLI commands implemented and functional
- [x] Observability dashboard provides comprehensive metrics
- [x] Workflow automation supports common AI development patterns
- [x] IDE integration enhances developer productivity
- [x] Debugging tools provide actionable insights

## Build Instructions

```bash
# Enable AI experiments
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
make t81_ai

# Run AI CLI
./build/experiments/ai/bin/t81_ai run --model llama-7b --prompt "Hello, world!"
```

## Usage Examples

### Basic Inference
```bash
# Simple inference
./t81_ai run --model llama-7b --prompt "What is T81?"

# Deterministic inference
./t81_ai run --model llama-7b --prompt "What is T81?" --deterministic

# Custom parameters
./t81_ai run --model llama-7b --prompt "What is T81?" --max-tokens 50 --temperature 0.7
```

### Model Management
```bash
# Inspect model
./t81_ai model inspect --model llama-7b --verbose

# Verify integrity
./t81_ai verify --model llama-7b --deterministic

# Quantize model
./t81_ai quantize --input llama-7b.gguf --codec T3_K2 --output llama-7b.t3k2
```

### Performance Analysis
```bash
# Run benchmarks
./t81_ai benchmark --model llama-7b --verbose

# Test policies
./t81_ai policy test --type model_load
```

## Developer Experience

### Consistent Interface
- **Unified Naming**: All commands follow `t81_ai <command>` pattern
- **Standard Options**: Common options across all commands
- **Help System**: Built-in help and usage examples
- **Error Handling**: Clear error messages and exit codes

### Productivity Features
- **Batch Operations**: Support for multiple model operations
- **Configuration**: Persistent configuration and preferences
- **Automation**: Scriptable interface for CI/CD integration
- **Integration**: Seamless integration with existing T81 tools

---

**RFC Reference**: RFC-00A7  
**Task**: 10 - Implement core AI CLI commands  
**Status**: Completed  
**Last Updated**: 2026-03-05
