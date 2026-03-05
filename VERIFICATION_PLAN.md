# T81 AI Minimal Integration Verification Plan

## Exact Commands to Build and Test

### 1. Configure Build
```bash
cd /Users/t81dev/Code/t81-foundation
mkdir -p build
cd build
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
```

### 2. Compile
```bash
make t81_ai
```

### 3. Test Help Command
```bash
./t81_ai --help
```

**Expected Output:**
```
T81 AI CLI - Minimal Implementation

Usage:
  t81_ai --help                           Show this help
  t81_ai model inspect <file>           Inspect model file
  t81_ai verify <file>                    Verify model integrity

Examples:
  t81_ai --help
  t81_ai model inspect model.gguf
  t81_ai verify model.gguf
```

### 4. Test Model Inspect
```bash
echo "mock model data" > test_model.gguf
./t81_ai model inspect test_model.gguf
```

**Expected Output:**
```
=== Model Inspection ===
File: test_model.gguf
Size: 16 bytes
Format: Unknown (mock implementation)
Parameters: Mock data
Created: Mock timestamp
Status: Inspection completed
```

### 5. Test Verify
```bash
./t81_ai verify test_model.gguf
```

**Expected Output:**
```
=== Model Verification ===
File: test_model.gguf
Size: 16 bytes
Hash: sha256:mock_hash_[hash_value]
Signature: Not verified (mock implementation)
Integrity: Basic file check passed
Status: Verification completed
```

## Build Verification Checklist

- [ ] CMake configuration succeeds with `-DT81_ENABLE_AI_EXPERIMENTS=ON`
- [ ] `t81_ai` target is created
- [ ] `make t81_ai` compiles without errors
- [ ] `t81_ai --help` shows correct usage
- [ ] `t81_ai model inspect <file>` works
- [ ] `t81_ai verify <file>` works
- [ ] Binary is installed to `experiments/ai/bin/`
- [ ] No external dependencies required
- [ ] Core T81 build remains unchanged
