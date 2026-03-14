<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation: Ternary-Native Computing Stack

![Project Status](https://img.shields.io/badge/status-Active_Engineering-gold)
![ISA Version](https://img.shields.io/badge/ISA-v1.1.0_Frozen-blue)
![Deterministic](https://img.shields.io/badge/Execution-Deterministic-green)

The **T81 Foundation** is a comprehensive, ternary-native computing ecosystem designed for deterministic AI inference, formal symbolic reasoning, and governed AGI research. Unlike binary systems, T81 utilizes Base-81 logic to provide a natively balanced arithmetic framework suitable for high-precision neural architectures and provably safe execution environments.

---

## 🚀 Quick Project Status (March 2026)

The project is currently in a transition phase from ISA stabilization to higher-level AI and language integration.

| Component | Completion | Status | Key Focus |
| :--- | :--- | :--- | :--- |
| **TISC ISA** | 100% | ❄️ Frozen | v1.1.0 Bit-identity stable |
| **Axion** | 95% | ✅ Beta | Alpha→Beta promotion ready |
| **T81VM** | 85% | 🧪 Beta | Monolith decomposition |
| **T81Lang** | 70% | 🧪 Beta | Spec finalized, Normative pending |
| **Standard Library** | 60% | 🚧 Alpha | API stabilization (March Promotion) |
| **AI Subsystem** | 50% | 🔬 Exp. | Deterministic evidence protocol |
| **Ternary OS** | 20% | 💡 Proto. | Bare-metal HAL development |

---

## 🏗 System Architecture

The stack is organized into a "Layer Cake" architecture, ensuring that every operation from the hardware abstraction up to cognitive reasoning remains deterministic and auditable.

1.  **TISC ISA**: The Ternary Instruction Set Architecture. Bit-frozen and audited.
2.  **Axion Kernel**: The safety and governance layer. It enforces "fail-closed" policies and maintains the deterministic execution contract.
3.  **CanonFS**: A content-addressed filesystem providing deterministic provenance for model weights and code.
4.  **T81Lang**: A high-level frontend for ternary logic, featuring native support for `BigInt`, `Fractions`, and `Tensors`.
5.  **Cognitive Tiers**: An experimental layer (Tiers 1-5) addressing symbolic reasoning, self-modeling, and recursive safety.

---

## 🛠 Developer Handoff & Governance

If you are new to the project or taking over stewardship, please review the following critical protocols:

### Determinism Verification
All logic changes must be validated across architectures (`Linux-x86_64` and `macOS-arm64`).
```bash
./scripts/ci/run_determinism_slice.sh

```

### Strategic Roadmap: Next 30 Days

* **STDLIB Promotion (March 20):** Transitioning `std::agent` and `std::async` to the Stable profile.
* **C2 Month Close (March 31):** Executing the cryptographic audit of the repository ledger.
* **Tier 4 Loop Closure (April 15):** Validation of self-modeling safety invariants.
* **Native HAL Boot (May 01):** First boot of Axion on bare metal.

### Technical Debt Alerts

* **JIT Equivalence Gap:** Watch for state hash mismatches in deep recursive BigInt divisions.
* **Monolith Decomposition:** Ongoing work to split `core/vm/vm.cpp` into discrete handlers.

---

## 📖 Documentation Index

* **[Getting Started](https://www.google.com/search?q=./docs/user-guide/getting-started/cpp-quickstart.md)**: Build instructions and environment setup.
* **[ISA Specification](https://www.google.com/search?q=./spec/tisc-spec.md)**: Full details on ternary opcodes and encoding.
* **[Axion Policy Manual](https://www.google.com/search?q=./docs/user-guide/tutorials/axion-policy-manual.md)**: How to write and enforce safety guards.
* **[Research Frontier](https://www.google.com/search?q=./docs/research/README.md)**: Deep dives into Ternary AGI and Tiered Cognition.

---

## ⚖️ Governance & Policy

The T81 Foundation operates under a **Continuous Governance (C2)** model. All contributions are audited for architectural coherence and deterministic parity. Refer to the [Governance Charter](https://www.google.com/search?q=./docs/governance/README.md) for more information.

**End of Status Report.**
