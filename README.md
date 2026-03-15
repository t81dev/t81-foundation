<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation — Deterministic Ternary Computing Stack

![Project Status](https://img.shields.io/badge/status-Active_Engineering-gold)
![ISA Version](https://img.shields.io/badge/ISA-v1.1.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)

The **T81 Foundation** is an experimental computing architecture built around **balanced ternary computation** and **deterministic execution**.

Instead of relying solely on binary arithmetic, the system models computation using **balanced ternary values (-1, 0, +1)** encoded through **Base-81 data structures**, enabling a reproducible and auditable execution environment designed for:

• deterministic virtual machine execution  
• governed AI inference environments  
• symbolic and high-precision computation  
• reproducible research systems  

The project implements a full stack including:

- a ternary instruction set (**TISC**)  
- a deterministic runtime (**T81VM**)  
- a governance kernel (**Axion**)  
- a canonical filesystem (**CanonFS**)  
- a ternary programming language (**T81Lang**)  

The long-term objective is to explore **deterministic AI execution and governed computation environments** built on top of a stable ternary execution substrate.

---

# 🚀 Project Status (March 2026)

The core execution architecture is largely stabilized and the project is transitioning toward **language, AI runtime, and operating system integration**.

| Component | Completion | Status | Focus |
| :--- | :--- | :--- | :--- |
| **TISC ISA** | 100% | ❄️ Frozen | Stable instruction architecture |
| **Axion Kernel** | 95% | ✅ Beta | Governance enforcement & event pipeline |
| **T81VM** | 95% | ✅ Stable | Deterministic runtime optimization |
| **T81Lang** | 90% | 🧪 Beta | Cross-platform bit-identity verification |
| **Standard Library** | 85% | 🧪 Beta | Async and agent primitives |
| **AI Runtime** | 60% | 🔬 Active | Backend adapter determinism |
| **Ternary OS** | 35% | 💡 Prototype | HAL and hardware bring-up |

Overall stack maturity estimate:

```

Deterministic Core       ~97%
Language Layer           ~90%
Runtime Ecosystem        ~75%
AI Integration           ~60%
Operating Platform       ~35%

```

---

# 🏗 Architecture Overview

The T81 architecture is intentionally layered to maintain a **deterministic boundary** between low-level execution and higher-level intelligence systems.

```

Cognitive / AGI Research Layers
│
AI Runtime & Symbolic Systems
│
T81Lang Compiler + Standard Library
│
Axion Governance Kernel
│
T81 Virtual Machine
│
TISC Instruction Set Architecture

```

### TISC ISA
The **Ternary Instruction Set Architecture** defines the fundamental execution model of the system.  
The ISA is frozen and provides the deterministic base layer for the entire stack.

### T81 Virtual Machine
A deterministic interpreter implementing the TISC ISA.  
The VM guarantees **bit-identical execution across supported platforms**.

### Axion Kernel
A governance kernel responsible for:

- policy enforcement
- deterministic scheduling
- safety invariants
- auditable execution traces

### CanonFS
A content-addressed filesystem designed to ensure deterministic provenance for:

- code
- model weights
- datasets
- runtime artifacts

### T81Lang
A high-level programming language designed for ternary computation with native support for:

```

BigInt
Fractions
Tensors
Deterministic collections

````

### Cognitive Tiers (Experimental)

A research framework exploring higher-level reasoning systems and governed AI runtime layers.

These tiers remain experimental and are **not part of the deterministic core**.

---

# 🛠 Developer Handoff & Governance

If you are contributing to the project or assuming repository stewardship, the following protocols are mandatory.

## Determinism Verification

All execution logic must produce identical output across supported platforms.

Run the determinism verification suite:

```bash
./scripts/ci/run_determinism_slice.sh
````

Current verification platforms:

```
Linux x86_64
macOS ARM64
```

Any divergence in VM trace hashes must be treated as a critical defect.

---

# 📅 Near-Term Roadmap

### March 20

**Standard Library Promotion**

Stabilization of:

```
std::agent
std::async
```

---

### March 31

**C2 Month Close**

Execution of the cryptographic audit for the repository governance ledger.

---

### April 15

**Tier-4 Loop Closure**

Verification of self-modeling safety invariants within the experimental cognitive tier framework.

---

### May 01

**First Bare-Metal Boot Target**

Initial Axion boot attempts following current VirtualBox probing and HAL development.

---

# ⚠ Technical Debt Watchlist

### JIT Equivalence Gap

Potential divergence between interpreter and JIT execution during deep recursive `BigInt` divisions.

All JIT blocks must maintain **state-hash parity** with interpreter execution.

---

### VM Monolith Decomposition

Ongoing effort to split:

```
core/vm/vm.cpp
```

into modular execution handlers.

---

# 📖 Documentation

| Topic               | Location                                            |
| ------------------- | --------------------------------------------------- |
| Getting Started     | `docs/user-guide/getting-started/cpp-quickstart.md` |
| ISA Specification   | `spec/tisc-spec.md`                                 |
| Axion Policy Manual | `docs/user-guide/tutorials/axion-policy-manual.md`  |
| Research Frontier   | `docs/research/README.md`                           |
| Governance Charter  | `docs/governance/README.md`                         |

---

# ⚖ Governance

The T81 Foundation operates under a **Continuous Governance (C2)** model.

All contributions must maintain:

* deterministic execution parity
* architectural coherence
* reproducibility guarantees

Changes affecting the deterministic surface require formal review.

---

# License

MIT License

That version dramatically increases **GitHub engagement and contributor curiosity**.
```
