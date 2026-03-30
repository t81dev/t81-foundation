# Deterministic AI Governance Demo

This demo answers the question: **"Wait... how is that possible?"**

Many systems claim to "govern" AI, but they do it by wrapping a massive, opaque C++ inference library (like `llama.cpp` or PyTorch) in a Python or proxy layer. Once the tensor operation is handed off, the policy engine is blind.

T81 is different. It is a deterministic, policy-gated ternary runtime. The governance doesn't live outside the execution; it lives *inside the VM loop*.

This demo proves that the Axion Policy Kernel intercepts execution natively, blocking a side effect exactly at the instruction boundary before an unapproved tensor can ever touch the `std.tensor.matmul` runtime.

## Running the Demo

```bash
# Build the project first
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Run the demo
bash examples/governance-demo/run_governance_demo.sh
```

## Expected Output

You will see the demo compile a synthetic ternary model (`.t81w`), then execute it under two policies:
1. **ALLOW Policy**: The exact model hash is explicitly approved. The runtime loads it and successfully executes `std.tensor.matmul`.
2. **DENY Policy**: The model hash does not match the approved list. The execution terminates deterministically with a `SecurityFault`.

The critical takeaway is that this fault isn't thrown by an external Python wrapper—it's trapped deep inside the interpreter loop by the native execution lane.

## Why this makes people care

- **Assumption Broken:** That AI inference has to be a black box opaque to policy.
- **Why Engineers Pause:** T81 doesn't bolt policy onto the side; it intercepts the actual deterministic compute graph at the opcode level. It proves you can have strict, bit-exact governance on deep neural network math without sacrificing the runtime.
- **Why It Matters:** In high-stakes or regulated environments (finance, defense, auditable systems), you cannot trust a "wrapper" policy. If the AI engine bypasses the wrapper, you lose. T81 makes bypassing mathematically impossible because the engine *is* the policy.

## The Pitch

**1-Tweet Version:**
Most "Governed AI" is just a Python script wrapping a black-box C++ engine. We built a runtime where the policy engine lives *inside* the tensor execution loop. Same inputs, identical trace, mathematically undeniable. Try `bash examples/governance-demo/run_governance_demo.sh` to see it trap a rogue tensor deterministically. #T81

**1-Paragraph README Insertion:**
**Deterministic Runtime Governance:** T81 doesn't wrap AI inference in brittle external proxies. The Axion Policy engine is integrated directly into the deterministic VM execution loop. When a policy restricts a model payload or computation tier, the T81 runtime traps the fault at the exact instruction boundary *before* the side effect occurs, proving that AI operations can be deeply, undeniably auditable.

**1-Sentence Hook:**
Watch the T81 runtime deterministically trap an unapproved AI payload deep inside the native tensor execution loop—proving true governance isn't a wrapper, it's an architecture.
