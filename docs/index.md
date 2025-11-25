# T81 Foundation Documentation Hub

Welcome to the central documentation hub for the T81 Foundation. This page provides a curated guide to the project's documentation, organized by purpose and audience.

---

### 1. Newcomers & Onboarding

If you are new to the project, start here. These documents provide the high-level overview and practical steps needed to get started.

-   **[Developer Handover & Progress Report](./handover.md)**
    -   **Purpose:** The single best starting point for a new developer.
    -   **Content:** A comprehensive overview of the project's vision, architecture, current status, development processes, and onboarding guide.

-   **[C++ Quickstart Guide](./cpp-quickstart.md)**
    -   **Purpose:** A concise guide to building the C++ project and running the examples.
    -   **Content:** Practical, hands-on build and execution commands using CMake.

---

### 2. In-Depth Developer Guides

These guides provide detailed, step-by-step walkthroughs for common and complex development tasks.

-   **[Guide: Adding a Feature to T81Lang](./guides/adding-a-language-feature.md)**
    -   **Covers:** The full lifecycle of adding a new language feature, from the lexer to the IR generator.

-   **[Guide: Adding a TISC Opcode to the VM](./guides/vm-opcodes.md)**
    -   **Covers:** The process for extending the virtual machine with a new instruction.

---

### 3. API Reference

The definitive, auto-generated reference for the C++ source code.

-   **[Generated C++ API Reference (Doxygen)](./api/html/index.html)**
    -   **Purpose:** Provides a detailed, browsable reference for every class, method, and file in the `/src` and `/include` directories.
    -   **How to Generate:** Run `cmake --build build --target docs` from the repository root.

---

### 4. Formal Specifications & Architecture

These documents are the "constitution" of the T81 Foundation. They define the normative behavior of every component in the stack.

-   **[Master Specification Index](../spec/index.md)**
    -   **Purpose:** The root index that links to all formal specification documents.

-   **[System Status Board](./system-status.md)**
    -   **Purpose:** A detailed report on the current implementation status of each major component, measured against its formal specification.

---

### 5. Project Governance & Contribution

-   **[AGENTS.md (Operational Contract)](../AGENTS.md)**
    -   **Content:** The rules and guidelines for contributing code to the project. Mandatory reading for all developers.

-   **[CONTRIBUTING.md](../CONTRIBUTING.md)**
    -   **Content:** General contribution guidelines, including the RFC process for proposing changes to the specifications.
