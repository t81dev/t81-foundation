# RFC-0033: Dual TUI Frontends for T81 – Human Operator & AI-Native Interfaces

**Status:** Draft / Proposed
**Authors:** TBD
**Date:** 2025-03-09

---

## Abstract / Summary

The T81 Foundation stack exposes a vast set of features—from deterministic execution (T81 VM) to content-addressed storage (CanonFS), execution trace verification, and governance (Axion)—through an extensive CLI. While robust, this CLI introduces friction for iterative development, deep system introspection, and AI-assisted workflows.

This RFC proposes two specialized Text User Interfaces (TUIs) built on the high-performance FTXUI C++ library to serve as complementary frontends. The first TUI targets human operators by providing a discoverable dashboard for daily compilation, debugging, and artifact exploration. The second TUI targets AI agents and automation pipelines, providing a conversational, stateful interface optimized for multi-step reasoning. Together, these TUIs will bridge the gap between low-level terminal execution and an integrated workspace while fully preserving CLI compatibility.

---

## Motivation / Problem Statement

T81's rigorous approach to deterministic execution and state governance (e.g., CanonFS, Axion policy engine) requires granular control. Currently, this is exposed through a deeply nested CLI structure, containing families such as `code`, `project`, `determinism`, `canonfs`, `trace`, `policy`, `axion`, `weights`, `vm`, `internal llama-run`, and `repl`.

This CLI surface presents several usability challenges:

1. **Complex Command Surface:** The volume of commands and flags makes it difficult to discover the correct incantations without constantly consulting documentation.
2. **Debugging Friction:** Debugging often requires rapidly switching between compiling code, running tests, inspecting trace hashes, and reading Axion policy rejections. This involves executing many disparate commands sequentially.
3. **Artifact Visibility:** CanonFS and Axion maintain complex internal states. Inspecting a CAS tree or tiered policy trace is cognitively demanding when output is limited to raw terminal text.
4. **Hostile Environment for AI Agents:** AI agents struggle with stateless CLI tools, often losing context, misinterpreting unstructured output, and failing to construct complex command chains.

There is a clear need for interactive, stateful environments that surface T81's internal complexity in an accessible manner, without discarding the performance of terminal-based workflows.

---

## Goals

* **Improve Developer Ergonomics:** Provide an interactive dashboard for common workflows (compilation, testing, execution) to reduce context switching.
* **Enhance System Observability:** Offer visual insight into CanonFS storage hierarchies, Axion policy graphs, and determinism trace hashing results.
* **Accelerate Debugging Cycles:** Enable rapid feedback loops by integrating code editing, compilation, and execution output into a single screen.
* **Provide an AI-Native Interaction Surface:** Create a stateful, structured environment tailored for LLMs to explore the codebase, interact with the REPL, and synthesize complex operations.
* **Preserve CLI Compatibility:** Ensure all TUI actions map directly to underlying CLI commands or C++ APIs.

---

## Non-Goals

This proposal explicitly avoids:

* **Replacing the CLI:** The existing CLI command surface will remain fully supported and documented. The TUIs are complementary layers.
* **Building a Graphical User Interface (GUI):** We will not introduce heavy graphical toolkits (Qt, Electron). The interfaces must remain text-based and terminal-native.
* **Introducing Web Infrastructure:** The TUIs will not rely on a local web server or browser. They must be native C++ binaries tightly integrated with the core runtime.
* **Modifying Core VM Architecture:** The implementation will not necessitate structural changes to the T81 VM, TISC instruction set, or deterministic execution guarantees.

---

## Proposed Solution

This RFC proposes two complementary Text User Interfaces, packaged as new binaries or subcommands, leveraging the **FTXUI** library.

### TUI 1 – Human Operator Interface

Designed for the daily workflow of developers, researchers, and systems engineers.

**Purpose:** A discoverable, ergonomic dashboard for interacting with T81 subsystems.

**Core Capabilities:**

* **Command Discovery:** A searchable command palette to fuzzy-find and execute T81 commands.
* **Integrated Development Cycle:** Rapid compile, run, and debug loops natively surfaced. Users can select a `.t81` file, trigger compilation, and execute bytecode in adjacent panes.
* **CanonFS Browser:** A tree view of the CanonFS storage, allowing users to inspect artifact metadata and blob contents.
* **Determinism Verification Dashboard:** Real-time visualization of execution trace hashes to visually compare runs and pinpoint divergence.
* **Axion Governance Inspector:** A dedicated view for inspecting Axion policies, simulating evaluations against artifacts, and viewing verdicts (Allow, Warn, Deny).
* **REPL Integration:** A persistent read-eval-print loop pane for immediate experimentation with T81Lang syntax.

**Conceptual Layout:**

```text
+-------------------------------------------------------------+
| T81 Foundation Workspace                        [Cmd: Ctrl+P] |
+-----------------+-------------------------------------------+
| Navigation      | Code Editor / Output View                 |
| > Workspace     |                                           |
| > Compiler      | > t81 code check src/main.t81             |
| > Determinism   | ✓ No semantic errors detected.            |
| > CanonFS       |                                           |
| > Axion Policy  | > t81 code build src/main.t81 -o main.tisc|
| > Execution Trace| ✓ Compiled to main.tisc (1.2KB)         |
| > REPL          |                                           |
+-----------------+-------------------------------------------+
| Status: VM [Tier 2] | Axion: [Strict] | Active Trace: [A4F9]|
+-------------------------------------------------------------+
```

**Integration Approach:** The Human Operator TUI (accessible via a shortened command like `t81 studio`) will primarily link directly against T81 core C++ libraries (`libt81_compiler`, `libt81_vm`, `libt81_axion`), invoking internal functions. Where direct API integration is premature, it will execute the `t81` binary as a subprocess, parsing structured `--json` output. A top-level discoverability command (`t81 ui`) will act as an entry point, prompting the user to launch either the human or AI interface.

### TUI 2 – AI-Native / Agentic Interface

A specialized environment optimized for interaction with Large Language Models and autonomous coding agents.

**Purpose:** Provide a stateful, context-rich environment optimized for AI observation, reasoning, and action.

**Core Capabilities:**

* **Persistent Conversational Session:** A continuous chat-like interface where agents can respond with actions, code snippets, or system queries.
* **Machine-Readable UI Surfaces:** Layouts optimized for structured extraction (e.g., aggressively normalized JSON blocks, concise trace summaries). This ensures LLMs can parse system state effortlessly, minimizing hallucinations compared to parsing raw markdown or unstructured text.
* **Contextual Side Panels:** Persistent displays of critical system state—such as active AI model weights, Axion policy tier, and determinism verification status.
* **Multi-Turn Experimentation:** The agent can iteratively propose code changes, trigger the compiler, observe trace outputs, adjust Axion policies, and retry execution within a single session.
* **Deep `llama-run` Integration:** Controls for managing internal LLM inference tasks, adjusting parameters, and observing raw output of `MAKE_COMPLEX` or `QMATMUL` operations.
* **Session Persistence & Resumability:** The ability to save a session's interaction history and state (e.g., to disk as JSONL and binary blobs referencing CanonFS) and resume it later. This includes deep-linking and attachment capabilities (e.g., `t81 agent --resume session-uuid.jsonl` or `t81 agent --attach <pid>` to inspect hung, mid-flight agent sessions).
* **Command Palette Consistency:** The AI TUI will mirror the Human TUI's command palette shortcut (`Ctrl+P`) for muscle memory, while also supporting standard chat-like slash commands (e.g., `/policy allow <hash>`) for rapid agent/operator interactions.

**Conceptual Layout:**

```text
+-------------------------------------------------------------+
| T81 Agentic Interface                    [Cmd: Ctrl+P / + /]|
+-----------------------------------------+-------------------+
| Interaction History                     | Context View      |
|                                         |                   |
| [Agent] Trace hash diverged at inst 42. | Model: LLaMA-3-8B |
|         The Axion policy denied the     | Tier:  [Tier 4]   |
|         TLOADHASH operation.            | Axion: [Audit]    |
|                                         |-------------------|
| [User]  Update the policy to allow      | Active Trace      |
|         that specific hash.             | Hash: 9A2B...     |
|                                         | Status: Diverged  |
| > [Input Prompt...]                     |                   |
+-----------------------------------------+-------------------+
| System Log: Axion evaluation deferred to runtime constraints|
+-------------------------------------------------------------+
```

**Integration Model:** This TUI (accessible via a shortened command like `t81 agent`) relies on a robust session management layer. It utilizes structured logging and tightly controlled standard IO streams to ensure automated harnesses or remote LLMs can easily attach, send commands, and ingest structured state updates.

---

## Alternatives Considered

* **Improving the CLI Only:** Enhancing the CLI with more verbose output and formatting. This does not solve discoverability, statefulness, and the need for persistent context during complex debugging or AI interactions.
* **Web Dashboard (React/Vue):** Building a local web server to serve a modern web application. This was rejected because it introduces architectural bloat, violates the terminal-native philosophy of T81, and complicates deployment in headless server environments.
* **Go Bubble Tea / Python Textual / Rust Ratatui:** Utilizing excellent TUI frameworks from other languages. While Ratatui is highly regarded, adopting any of these would force a language bridge or a complete rewrite, introducing a massive new dependency and violating the strict C++23 dependency-firewalled architecture of T81.
* **Raw Ncurses:** Building a TUI from scratch using the legacy `ncurses` library. Dismissed due to the low-level API, lack of modern layout paradigms, and significant developer overhead compared to modern C++ frameworks.

---

## Why FTXUI?

[FTXUI (Functional Terminal (X) User Interface)](https://github.com/ArthurSonzogni/FTXUI) is the optimal choice:

1. **C++ Ecosystem Alignment:** FTXUI is written in modern C++ and perfectly aligns with T81's C++23 foundation, allowing TUIs to be compiled as native components.
2. **Minimal Dependencies:** It is highly modular and avoids pulling in a sprawling dependency tree, respecting the T81 Dependency Firewall.
3. **Declarative & Composable Layouts:** FTXUI utilizes a functional, declarative approach to UI construction, simplifying complex, responsive layouts (e.g., flexbox-like element trees).
4. **Cross-Platform Support:** Robust support across Linux, macOS, and Windows terminals, natively handling modern features like true color and mouse input.
5. **WASM Potential:** Proven support for compilation to WebAssembly, providing a future-proof pathway to exposing the TUI via a web browser without rewriting logic.

---

## Risks & Drawbacks

* **Increased Binary Size:** Linking FTXUI will increase the size of the final executables. Given T81's target environments (embedded/headless/server reproducibility), we will implement a CI check that compares stripped binary sizes and warns if the delta exceeds 10-15%.
* **Maintenance Burden:** Introducing two new UI surfaces expands the surface area of the project. The UI code must be maintained alongside the core API to prevent regressions.
* **Contributor Learning Curve:** Developers accustomed strictly to CLI tool development will need to familiarize themselves with declarative UI paradigms and FTXUI's functional API.
* **Platform Compatibility Edge Cases:** Certain legacy terminal emulators or heavily customized shells might exhibit rendering artifacts or input handling bugs.
* **Testing Story:** TUIs are notoriously difficult to unit-test. To mitigate this risk, the implementation will require comprehensive testing, specifically utilizing snapshot-style golden-output testing for layout rendering (text + ANSI) under different terminal widths and scripted input sequences.

---

## Implementation Plan

### Phase 1: Infrastructure and Prototyping

* Integrate FTXUI into the T81 CMake build system via `FetchContent` to ensure seamless dependency management.
* Establish base architectural patterns for state management and UI updating.
* Develop a minimal prototype of the Human Operator TUI, focusing on the REPL and code compilation output.

### Phase 2: Human Operator TUI Expansion

* Implement the Navigation Sidebar and Layout Manager.
* Build out specialized views: CanonFS Browser, Determinism Trace Visualizer, and Axion Policy Status Dashboard.
* Ensure comprehensive keyboard navigation and implement the Command Palette.

### Phase 3: AI-Native Interface Development

* Implement the persistent session management backend.
* Design and implement the chat pane and context sidebars.
* Integrate structured logging and `--json` parsing necessary to feed the UI state reliably.
* Test the interface extensively with internal AI tooling (`llama-run`) and external agent harnesses.
* **Dogfooding Milestone:** Successfully run an end-to-end agent workflow entirely within the TUI ("propose patch → compile → observe trace delta → auto-allow hash via policy adjustment → retry execution") before declaring this phase complete.

### Phase 4: Refinement, CI, and Documentation

* Establish comprehensive snapshot-style golden-output testing for the UI layout rendering and scripted input sequences.
* Integrate TUI build, smoke tests, and binary size delta checks into the existing CI pipeline.
* Add an animated terminal recording or GIF showcasing the `t81 ui` entry point and basic compile/REPL workflows to the primary `README.md` and user guides to enhance project discoverability.
* Write detailed documentation, user guides, and integration examples.

---

## Future Work

* **Custom Theming Engine:** While a high-contrast "researcher dark mode" (black background + distinct accent colors for verdicts/trace diffs) will be prioritized as the default, we plan to allow users to define color palettes and layout preferences via configuration files to improve accessibility.
* **Mouse Support Optimization:** Enhancing mouse interaction (scrolling, clicking tabs) to further improve usability.
* **TUI Plugin System:** Exposing an API for developers to write custom FTXUI components.
* **Distributed Monitoring:** Extending the TUI to connect to remote T81 instances to monitor CanonFS or Axion state on production servers.
* **Standardized Agent Protocol:** Formalizing the communication format used by the AI-Native TUI, potentially adopting standards for LLM-to-tool communication.

---

## References

* **T81 Foundation Repository:** [https://github.com/t81dev/t81-foundation](https://github.com/t81dev/t81-foundation)
* **FTXUI GitHub Repository:** [https://github.com/ArthurSonzogni/FTXUI](https://github.com/ArthurSonzogni/FTXUI)
* **Design Inspiration:** `lazygit`, `gitui`, and `bottom`.
* **T81 Architecture Documents:** `DEPENDENCY_FIREWALL.md`, `DETERMINISM_SURFACE_REGISTRY.md`, `RFC-0003`, `RFC-0026`