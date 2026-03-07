# T81 Hardware Roadmap (Conceptual)

T81 is currently implemented as a hardware-agnostic, spec-first stack. It targets
balanced ternary arithmetic and a ternary VM model, but runs today on conventional
binary hardware.

## 1. Current state

- Execution via C++ VM and host CPU
- Deterministic semantics independent of underlying ISA

## 2. Ternary hardware landscape

T81 is designed to be compatible with future ternary / non-binary hardware, such as:

- Ternary logic research and balanced ternary arithmetic in silicon and nanowire devices. :contentReference[oaicite:3]{index=3}
- Energy-efficient ternary DNN accelerators and in-memory computing proposals (e.g., TiM-DNN, CUTIE). :contentReference[oaicite:4]{index=4}
- Recent patents and announcements around ternary logic for AI chips, including Huawei’s ternary logic efforts aimed at reducing transistor count and energy use. :contentReference[oaicite:5]{index=5}

T81’s IR and VM are intentionally designed so they can be mapped onto such devices
without changing the high-level language or specs.

## 3. Mapping T81 to future ternary chips

Future work items:

- Formal mapping from TISC instructions to candidate ternary logic gate libraries.
- Energy and performance modeling for T81 programs on ternary AI accelerators.
- Potential collaborations or backend modules to target specific ternary hardware,
  while keeping the T81 specification hardware-neutral.

## 4. Hardware Investigation & Abstraction

### Investigate FPGA/Hardware-Backed Execution
We are actively investigating pathways to map T81 execution onto FPGA fabrics.
-   **Goal**: Demonstrate native ternary logic emulation on reprogrammable hardware.
-   **Approach**: Develop VHDL/Verilog modules for `T81Int` ALU operations and measure efficiency gains over binary emulation.

### Hardware Abstraction Boundaries
Defining pragmatic boundaries is crucial to support diverse backends without fragmentation.
-   **Constraint**: Any hardware acceleration *must* be bit-exact with the canonical scalar implementation.
-   **Method**: Formal verification of hardware modules against the software specification.
-   **Outcome**: A verified hardware interface that allows pluggable accelerators (FPGA, ASIC) to drop in for performance-critical kernels.
