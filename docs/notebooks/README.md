# 🧠 T81 Interactive Laboratory

This directory contains the canonical interactive laboratory for the T81 Foundation. These notebooks serve as an executable systems compendium, covering all implemented subsystems, aligning with the Strict Determinism Profile, and demonstrating architecture coalescence.

## 📚 Tracks and Structure

The laboratory is organized into five tracks, progressing from the numeric substrate to advanced system features.

### TRACK I — CORE NUMERIC SUBSTRATE

*   **[00_Numeric_Foundations.ipynb](00_Numeric_Foundations.ipynb)**: Balanced ternary principles, `T81Int` layout, arithmetic invariants, and canonical serialization.
*   **[01_Arbitrary_Precision_and_Fractions.ipynb](01_Arbitrary_Precision_and_Fractions.ipynb)**: `T81BigInt` internals, Karatsuba multiplication, Knuth division, and `T81Fraction` canonical reduction.
*   **[02_Floating_and_Soft_Math.ipynb](02_Floating_and_Soft_Math.ipynb)**: `T81Float` layout, NaE state, strict vs. host-dependent operations, and the `dmath` backend.

### TRACK II — DATA STRUCTURES & MEMORY DOMAINS

*   **[03_Containers_and_Allocation.ipynb](03_Containers_and_Allocation.ipynb)**: `T81List`, `T81Map`, `T81Set`, deterministic behavior, and memory quota simulations.
*   **[04_Tensors_and_Graphs.ipynb](04_Tensors_and_Graphs.ipynb)**: `T81Tensor` rank/dimension model, hybrid storage, deterministic matmul, and graph algorithms.
*   **[05_Symbols_Identity_and_Serialization.ipynb](05_Symbols_Identity_and_Serialization.ipynb)**: Intern table mechanics, content-addressed identity, and canonical export forms.

### TRACK III — VM, ISA, AND EXECUTION

*   **[06_TISC_and_VM_Execution.ipynb](06_TISC_and_VM_Execution.ipynb)**: TISC instruction encoding, register model, execution stepping, and disassembly.
*   **[07_Trap_Taxonomy_and_Failure.ipynb](07_Trap_Taxonomy_and_Failure.ipynb)**: Deterministic traps, `OOM_QUOTA` simulation, strict violations, and replay invariants.
*   **[08_Strict_Determinism_Profile.ipynb](08_Strict_Determinism_Profile.ipynb)**: Tier A enforcement, forbidden operations, strict float division, and entropy traps.

### TRACK IV — GOVERNANCE & PROVENANCE

*   **[09_Axion_Policy_Engine.ipynb](09_Axion_Policy_Engine.ipynb)**: Policy grammar, resource limits, policy denial demonstration, and trace analysis.
*   **[10_CanonFS_and_Content_Addressing.ipynb](10_CanonFS_and_Content_Addressing.ipynb)**: Hash derivation, snapshot lineage, deterministic storage, and provenance verification.
*   **[11_Trace_Replay_and_Audit.ipynb](11_Trace_Replay_and_Audit.ipynb)**: generating Axion traces, replay execution, bit-for-bit validation, and cross-arch discussion.

### TRACK V — SYSTEM COALESCENCE & ADVANCED FEATURES

*   **[12_Reflection_and_Tier4_Cognition.ipynb](12_Reflection_and_Tier4_Cognition.ipynb)**: MetaRead/MetaWrite, reflection cycles, and policy-gated refinement.
*   **[13_Distributed_and_Sharded_Tensors.ipynb](13_Distributed_and_Sharded_Tensors.ipynb)**: Multi-shard tensor logic, deterministic distribution, and strict mode boundaries.
*   **[14_Agent_Model_Integration.ipynb](14_Agent_Model_Integration.ipynb)**: `T81Agent` overview, entropy accounting, and deterministic vs. policy-governed behavior.
*   **[15_Performance_and_Benchmarking.ipynb](15_Performance_and_Benchmarking.ipynb)**: Deterministic benchmarking, memory behavior, SIMD acceleration, and reproducibility.

## 🛠 Usage

1.  **Build Python Bindings**: Ensure the `t81_python` module is built.
    ```bash
    cmake -S . -B build -DT81_BUILD_TESTS=OFF
    cmake --build build --target t81_python
    ```
2.  **Environment**: Add the build directory to your `PYTHONPATH`.
    ```bash
    export PYTHONPATH=$PYTHONPATH:$(pwd)/build
    ```
3.  **Launch Jupyter**:
    ```bash
    jupyter lab
    ```

## 📜 Migration Strategy

The previous notebooks have been moved to `notebooks/legacy/`. These new notebooks replace them as the authoritative executable documentation for the T81 system. Users should transition to these new notebooks for all current development and verification tasks.
