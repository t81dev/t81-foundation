# T81 Real-Model Validation Report — Phase 1

This document records the results of the **first real-world validation experiment for T81** using an externally sourced Hugging Face model artifact under Axion policy governance and CanonFS identity tracking.

---

## Artifact

- **Model Name:** `hf-internal-testing/tiny-random-LlamaForCausalLM`
- **Source Repository:** `https://huggingface.co/hf-internal-testing/tiny-random-LlamaForCausalLM`
- **Source File:** `model.safetensors`
- **Source Byte Size:** 4,131,280 bytes
- **Source SHA3-512:** `7437b55f2161bf9dd41ec7adcdf9bcfe0b6b127c341b7ff3753daf06a1554ed891b94678c3c4baec35d981dba4ff68eaadf0e28fcf582cbe8d16f6d9d2c1e4f1`
- **Native Import Format:** `SafeTensors(float-quantized; profile=native-dense-v1)` -> `.t81w`
- **Imported `.t81w` Size:** 174,016 bytes
- **Imported SHA3-512:** `sha3-512:98ba9e19a58b652b82b3fa1c09cd25c7a6001e4c7d21a4cd1b71144291d293b74adc19ead3164c56c993c70a2d74edc4fb93e6757ad37d927a280d903e082976`
- **CanonFS Object Ref:** `sha3-256:3D9-uy5≥−miEzKW4ys2A≠RqZ≤lnC6sτ∞g≈iDVxσω≠`

---

## Environment

- **OS:** Linux (x86_64)
- **CPU Architecture:** `x86_64`
- **Compiler:** GCC 13.3.0 (`-std=c++23`)
- **T81 Stack Version:** `v1.9.1`
- **ISA Version:** `TISC v1.1.0` (frozen 209 canonical opcodes)
- **Determinism Mode:** `strict_deterministic`
- **Selected Backend:** `t81_reference_vm`

---

## Execution

- **Task Family:** `task:answer_fixed.v1`
- **Prompt / Input:** `"hello world"`
- **Execution Output:** `UNKNOWN` (token ID `10729`)
- **Result Reference:** `sha3-256:Zf≈τ÷MppZ1≥q−∞lh≈NYnzY8JJz5W9GkJ4≤Plr≠hσ`
- **Provenance Reference:** `sha3-256:1sW=35sxoτ≤KO8qLsτUow−A≤8J6XsuCSSλfJW1≈4i`
- **Execution Status:** `pass`
- **Deterministic Replay Result:** `PASS` (`run1_result_ref == run2_result_ref`, `run1_prov_ref == run2_prov_ref`)

---

## Governance

- **Policy Definition:**
  ```lisp
  (policy
    (tier 1)
    (allowed-ternary-model-hashes ["sha3-512:98ba9e19a58b652b82b3fa1c09cd25c7a6001e4c7d21a4cd1b71144291d293b74adc19ead3164c56c993c70a2d74edc4fb93e6757ad37d927a280d903e082976"])
    (require-axion-event (reason "task:answer_fixed.v1")))
  ```
- **Policy Digest:** `sha3-256:0548e4a62e540816ad9812ac645346733b76d57285a0cd0ceea84582f4b5a174`
- **Approved Artifact Hash:** `sha3-512:98ba9e19a58b652b82b3fa1c09cd25c7a6001e4c7d21a4cd1b71144291d293b74adc19ead3164c56c993c70a2d74edc4fb93e6757ad37d927a280d903e082976`
- **Policy Verdict on Approved Model:** `allowed` (Execution status: `pass`)
- **Policy Verdict on Tampered Model:** `REJECT` (`SecurityFault` trapped before inference; exit code 13; `compute executed: no`)

---

## Provenance & Bundle Verification

- **Downstream Bundle Schema:** `t81.ai.task.assess-fixed.bundle.v1`
- **Downstream Record Schema:** `t81.ai.task.assess-fixed.host-action-record.v1`
- **Bundle Reference:** `sha3-256:2Z2≈6≤dpoΓ9Q6p3Gdσ7r−Iμwiσp6IY2GΓwD≈P3≈Dz`
- **Bundle Verification Command:** `t81 artifact validate-record <bundle_file> --schema t81.ai.task.assess-fixed.bundle.v1`
- **Original Bundle Verification:** `pass` (`"ok": true`)
- **Tampered Bundle Verification:** `VERIFICATION FAILURE` (`invalid CanonFS ref`, exit code 1)

---

## Summary Result

```text
REAL MODEL VALIDATION
=====================

Artifact:      hf-internal-testing/tiny-random-LlamaForCausalLM (model.safetensors, 4131280 bytes)
Identity:      sha3-256:3D9-uy5≥−miEzKW4ys2A≠RqZ≤lnC6sτ∞g≈iDVxσω≠ (sha3-512:98ba9e19a58b652b82b3fa1c09cd25c7a6001e4c7d21a4cd1b71144291d293b74adc19ead3164c56c993c70a2d74edc4fb93e6757ad37d927a280d903e082976)
Policy:        sha3-256:0548e4a62e540816ad9812ac645346733b76d57285a0cd0ceea84582f4b5a174 (sha3-512:98ba9e19a58b652b82b3fa1c09cd25c7a6001e4c7d21a4cd1b71144291d293b74adc19ead3164c56c993c70a2d74edc4fb93e6757ad37d927a280d903e082976 bound)
Execution:     pass (sha3-256:Zf≈τ÷MppZ1≥q−∞lh≈NYnzY8JJz5W9GkJ4≤Plr≠hσ)
Replay:        pass (run1_result_ref == run2_result_ref)
Bundle:        pass (sha3-256:2Z2≈6≤dpoΓ9Q6p3Gdσ7r−Iμwiσp6IY2GΓwD≈P3≈Dz)
Model Tamper:  pass (SecurityFault trapped before inference; exit 13; compute executed: no)
Bundle Tamper: pass (VERIFICATION FAILURE detected on tampered ref)

Overall Result:
PASS
```

---

## Limitations

1. **Unsupported Model Features:**
   The current reference VM (`t81_reference_vm`) enforces a strict, bounded architecture profile (`llama-dense-v1`). Complex non-dense architectures, Mixture-of-Experts (MoE), sliding-window attention, and non-standard RoPE embeddings are not currently supported by the core interpreter lane.

2. **Performance Limitations:**
   Execution is performed purely on the CPU reference interpreter. Floating-point and ternary tensor ops run without GPU or dedicated NPU acceleration. Large models (>1B parameters) will experience severe latency overhead.

3. **Memory Limitations:**
   Weights are loaded into host RAM and converted into `.t81w` contiguous limb structures. Memory consumption scales linearly with model parameter count, making models >3B parameters impractical on standard developer memory envelopes.

4. **Unsupported Operators:**
   Dynamic tensor slicing, flexible reshape ops without pre-validated layout descriptors, and arbitrary non-linear activation functions outside the frozen TISC ISA set are rejected at compilation or pre-dispatch validation.

5. **Host-Specific Behavior:**
   Floating-point quantization during import (`weights import`) relies on IEEE-754 host float math. Bit-exact cross-platform determinism requires strict soft-float or native ternary `T3_K` quantized representations.

6. **Missing FFI Isolation:**
   Host-side action executor handoffs rely on CanonFS ref dereferencing and host-process file writes rather than sandboxed WebAssembly or hardware Enclave boundaries.

---

## Recommended Next Experiment

Execute a real-world multi-token GGUF model (such as `Qwen/Qwen2.5-0.5B-Instruct-GGUF`) through the native `T3_K` quantized GGUF import lane (`t81 weights import --format gguf`) with carried KV-cache state under strict deterministic Axion governance.
