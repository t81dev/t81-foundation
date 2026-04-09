# T81 Examples & Demonstrations

Welcome to the `examples/` directory for the **T81 Project** — a deterministic, ternary-native, policy-gated runtime for governed AI cognition.

This directory is not just a collection of disconnected demos; it is a **living map of progress**. It traces the evolution of T81 from foundational language primitives and strict determinism checks, through the Axion Governance Kernel's policy enforcement, all the way up to complex tensor inference and aspirational agentic systems. Whether you are validating bit-exact ternary arithmetic or exploring self-reflective, policy-gated AI models, these examples demonstrate how T81 enforces safety and determinism at the lowest levels of execution.

## 📊 Status Legend

To help set expectations regarding maturity and stability, examples are categorized using the following tags:

*   ✅ **Stable / Working:** Core features, basic math, verified governance execution, and fundamental data structures. Highly reliable.
*   ⚠️ **Partial / Work-in-Progress:** Complex model loading, deep C++ host integrations, and evolving Axion policy constraints. May require specific build flags.
*   🔥 **Experimental / Aspirational:** High-level agentic cognition, multi-agent consensus, and T81 "Genesis" concepts. Primarily for visionary exploration and architectural planning.
*   🧪 **Conformance / Test-only:** IR roundtripping, C-API testing, and strict determinism verification scripts.

## 🧭 Guided Navigation

Depending on your background and goals, we recommend the following learning paths:

*   **New Contributors & T81Lang Learners:**
    Start in `core-language/` to see basic syntax (`hello_world.t81`), native types (`data_types.t81`), and error handling (`option_result_match.t81`).
*   **Safety & Governance Researchers:**
    Dive into `governance/` to see the Axion Kernel in action. Start with `governance/governance-demo/governed_matmul.t81` to understand how mathematical operations are policy-gated, then review the `.apl` (Axion Policy Language) traces. This path also needs a `--weights-model` runtime model or the demo script.
*   **AI/ML & Systems Engineers:**
    Explore `ai-and-inference/` and `model-load-canonfs/`. These examples demonstrate how T81 securely parses external `.safetensors` or `.gguf` weights, probes layers immutably via CanonFS, and executes tensor operations deterministically.
    For the current review-first model-artifact wedge, start with
    `ai-and-inference/model-artifact-review/README.md`.
*   **Architects & Visionaries:**
    Read `aspirational/T81Genesis.cpp` and explore the `system-integration-agi/` directory. These outline the ultimate North Star of T81: self-reflective, distributed, governed artificial general intelligence.

## 🌟 Standout Demonstrations

*   **The Governance Demo (`governance/governance-demo/`)** ✅
    Proves our core thesis: computation can be dynamically policy-gated. Run `bash examples/governance/governance-demo/run_governance_demo.sh` to see an Axion policy allow and deny the same weights-backed tensor program.
*   **Model Loading via CanonFS (`ai-and-inference/model-load-canonfs/`)** ⚠️
    Demonstrates bridging the outside world into T81's immutable storage (CanonFS). Features scripts like `run_real_hf_tiny_model.sh` and tools that parse and probe real Hugging Face model weights deterministically.
*   **Model Artifact Review (`ai-and-inference/model-artifact-review/`)** ✅
    Shows the current artifact-intake wedge with `model import`, persisted
    manifests, raw representation-sensitive diff, and opt-in normalized
    `.gguf` versus `safetensors` comparison. Includes
    `run_model_artifact_review.sh` for a single smokeable walkthrough.
*   **LLaMA Bridge & AI Integration (`ai-and-inference/ai-integration/`)** ⚠️
    Showcases C++ host integrations wrapping standard AI runtimes (like `llama.cpp`) inside T81's deterministic governance boundaries.
*   **T81 Genesis (`aspirational/T81Genesis.cpp`)** 🔥
    A conceptual masterclass. This C++ file sketches the bootstrap sequence of a fully autonomous, policy-governed T81 runtime environment initiating its own cognitive loops.

## 🚀 How to Run the Demos

### Prerequisites
Ensure your project is built, typically in `Release` mode to avoid timeouts on heavy tensor ops, though `Debug` is useful for tracking assertions:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

Verify your environment and initialize your local CanonFS state:
```bash
./build/t81 env doctor
```

### Running `.t81` Scripts
To execute a standard T81 script via the VM:
```bash
./build/t81 code run examples/core-language/hello_world.t81
```

### Running C++ Host Integrations
Compiled C++ demos (like `T81Genesis` or `llama_runner_demo`) output binaries to your build directory. Run them directly:
```bash
./build/examples/T81Genesis
```

## 🤝 Contribution Guidelines

We welcome new examples! When adding to this directory:
1. **Maintain Determinism:** Avoid relying on system time, non-deterministic PRNGs, or un-gated network access in your `.t81` scripts.
2. **Find the Right Home:** Place your example in the appropriate subdirectory rather than the root.
3. **Include Comments:** Explain *why* the example exists. If it tests a specific Axion trap or TISC opcode, note it clearly at the top of the file.
4. **Update the Legend:** If your demo represents a new capability, tag it appropriately (✅, ⚠️, 🔥, 🧪).
