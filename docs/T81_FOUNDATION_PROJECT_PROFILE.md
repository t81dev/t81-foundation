# T81 Foundation Project Profile

## 1. Executive Positioning

T81 Foundation is a deterministic computing substrate built around a ternary-native model, not a conventional binary-first application stack. Its core claim is narrower and more technical: for explicitly verified surfaces, identical inputs and configuration should produce identical outputs and traces across supported platforms.

The problem class is deterministic execution under governance constraints. In practical terms, T81 focuses on three hard problems that are usually treated separately:

- canonical numeric and serialization behavior,
- reproducible virtual-machine execution,
- enforceable policy controls at runtime.

By treating these three pillars as foundational, T81 aligns directly with the core tenets of **NIST SP 800-218 (SSDF)** and **NIST SP 800-53** controls. This deliberate alignment establishes T81 as a high-assurance, auditable AI infrastructure where security and policy enforcement are intrinsic to the instruction set, rather than bolted on.

This is still poorly solved in most mainstream systems because the typical stack is layered from components that optimize for throughput and convenience first, then attempt to patch determinism and auditability afterward. Binary/IEEE-754 ecosystems, host math libraries, runtime scheduling variance, and weak policy enforcement boundaries all contribute to drift. The result is familiar in AI and systems research: outputs that are "close enough" but not bit-identical, and governance policies that are advisory rather than mandatory.

T81's architecture attempts to invert that order: determinism and policy observability are first-class system constraints, and performance/experimental features are explicitly separated from guaranteed surfaces.

---

## 2. Architectural Overview

T81 is organized as a layered stack with explicit authority boundaries and maturity labels.

### Base-81 / balanced ternary data model

The data model is centered on ternary semantics and canonical forms. In implementation terms, the codec layer includes balanced Base-81 packing and unpacking constraints (for example, balanced digit ranges and canonical decode rules), while the type system specifies deterministic canonicalization requirements for core numerics (`T81BigInt`, `T81Fraction`, and bounded `T81Float` behavior).

Why this layer exists: deterministic systems fail when representational ambiguity leaks upward. Canonicalization at the data boundary reduces that ambiguity before execution begins.

Tradeoff: stronger normalization and canonical encoding rules reduce flexibility and may constrain interoperability with conventional binary-native tooling unless adapters are used.

### TISC instruction set

TISC is the machine contract. In the current profile, instructions use a fixed-width 13-byte encoding (`opcode + 3 int32 operands`), with explicit decode-fault behavior rather than undefined behavior. The ISA spec is treated as stable/frozen for its core semantics.

Why this layer exists: determinism depends on an instruction semantics surface that is total and auditable. TISC is that surface.

Tradeoff: strict semantic stability slows down "move fast" ISA evolution and pushes changes into governed extension processes.

### T81VM interpreter

The VM is the execution engine over TISC programs. The interpreter path is the deterministic reference surface, with trap-based fault semantics and policy-aware dispatch hooks. Axion verdict evaluation is integrated into the step path before protected effects.

Why this layer exists: determinism claims must be tied to a concrete execution path, not just a language spec.

Tradeoff: interpreter-first rigor can cost throughput versus aggressively optimized runtimes; optimization paths exist but are maturity-bounded.

### Axion governance kernel

Axion is a runtime policy kernel, not a static policy document. It evaluates syscall contexts and can allow, warn, or deny based on policy bytecode/rules, limits, and required trace evidence. In practice this means governance decisions can terminate execution deterministically.

Why this layer exists: governance credibility requires enforcement, not prose.

Tradeoff: tighter enforcement increases operational complexity and requires carefully maintained policy/test artifacts to avoid false assurances or policy drift.

### T81Lang surface

T81Lang is the language frontend that compiles to TISC. The implementation is Beta while the normative language spec remains Draft. Deterministic compilation behavior is partially verified through reproducibility gates and fixture hashes.

Why this layer exists: researchers and runtime engineers need a programmable surface that compiles into the deterministic substrate.

Tradeoff: language ergonomics and feature breadth are constrained by deterministic lowering requirements and active spec/implementation drift control.

### CanonFS (bounded relevance)

CanonFS provides content-addressed storage drivers (in-memory and persistent) with hash-based identity and read-path re-verification enabled by default (`T81_CANONFS_READ_VERIFY` can disable verification for diagnostics). Capabilities and Axion hooks are part of the write/read control flow.

Why this layer exists: deterministic execution without deterministic persistence boundaries is incomplete for audit/replay workflows.

Tradeoff: immutability and capability gates increase correctness and provenance, but add overhead and operational discipline requirements.

### Determinism gates and CI enforcement

Determinism is not asserted globally. It is scoped through a Determinism Surface Registry and DCP (Deterministic Core Profile), then enforced through tests and CI gates (including cross-architecture reproducibility checks and dedicated repro scripts). The current local snapshot exposes a large conformance/test surface (283 CTest entries).

Why this layer exists: reproducibility claims need executable evidence and merge-time enforcement.

Tradeoff: governance and gate maintenance become part of core engineering workload.

---

## 3. What Makes This Project Different

T81 differs from traditional stacks less by novelty and more by boundary discipline.

Against traditional binary-first toolchains:
- T81 puts canonical ternary/base-81 semantics at the substrate level instead of as a late abstraction.
- ISA/data-type freeze boundaries are explicit and governed.
- Undefined behavior is intentionally replaced by deterministic faults at key execution points.

Against floating-point non-deterministic ML stacks:
- T81 explicitly scopes determinism claims and names exceptions.
- Verified surfaces prioritize bit-exactness and reproducible traces where registry-backed.
- Host-dependent operations (notably certain float division/transcendentals on current paths) are documented as bounded non-guarantees rather than quietly tolerated.

Against VM environments without policy enforcement:
- Axion policy checks are integrated in execution flow and can deny operations.
- Policy evidence is traceable through canonical reason strings and conformance tests.
- Governance is tied to operational checks, not only process documents.

Against AI runtimes without reproducibility guarantees:
- T81 includes dedicated reproducibility gates (`t81lang_repro_gate`, `t3k_repro_gate`, cross-arch checks) and release discipline artifacts.
- Maturity labels are explicit (Stable/Beta/Alpha/Experimental) and checked for coherence across status documents.
- Release posture can be held by governance criteria, not just build success.

This is a different engineering posture: claims are narrower, but harder.

---

## 4. Who This Is For

T81 is for teams that treat determinism and governance as first-order system properties.

Primary fit:
- Systems researchers studying deterministic execution models.
- Deterministic AI reproducibility researchers.
- VM and ISA designers evaluating total semantics and fault contracts.
- Language/runtime engineers interested in canonical lowering and replayable behavior.
- Governance-aware infrastructure builders needing enforceable policy boundaries.
- Organizations requiring NIST SP 800-218 and 800-53 compliant execution environments for high-assurance AI deployments.
- Experimental computing researchers working on ternary-native or nonstandard compute models.

Not a good fit:
- Casual app developers optimizing for rapid product iteration.
- Framework-only users expecting high-level abstractions without substrate constraints.
- Teams seeking prototype speed over reproducibility discipline.
- Workloads requiring unconstrained performance-first runtime behavior.

---

## 5. Current State (Honest Status)

As of February 26, 2026, T81 presents a mixed-maturity but structurally serious profile.

What is relatively stable:
- TISC ISA and core data-type surfaces are treated as frozen/stable boundaries.
- VM interpreter determinism and core trace/fault behavior have deep test coverage.
- Determinism governance documents, release discipline, and freeze enforcement are operational.

What is active but not fully closed:
- T81VM overall maturity remains Beta.
- T81Lang implementation is Beta while the language spec remains Draft.
- Axion is Alpha with partial draft-surface coverage.
- CanonFS is Beta with implemented drivers and integrity checks, with hardening ongoing.

What is explicitly experimental/non-DCP:
- JIT/trace optimization paths (including equivalence work in progress).
- Cognitive tiers and AGI-oriented runtime surfaces unless formally promoted.
- Distributed/advanced experimental pathways outside verified registry surfaces.

Where guarantees apply:
- Only to registry-verified surfaces and DCP-scoped boundaries.
- Through documented tests, gates, and cross-platform verification paths.

Where claims are bounded:
- Real-time timing determinism is not guaranteed.
- Network behavior determinism is not guaranteed.
- Performance determinism is not guaranteed.
- Hardware/host-dependent float behavior remains bounded outside strict verified subsets.

This boundary clarity is a credibility feature, not a limitation to hide.

---

## 6. Why It Matters Now

The broader computing ecosystem has a reproducibility gap, especially in AI and high-stakes inference pipelines. Cross-platform drift, floating-point variability, implicit runtime nondeterminism, and weak execution governance make it difficult to answer a basic technical question: can a result be re-executed exactly and audited end-to-end?

T81 matters in that context because it frames determinism as infrastructure, not as an after-the-fact benchmark. Its contribution is not that it has solved all of deterministic computing; it has not. Its contribution is that it provides a concrete, testable substrate where:

- deterministic claims are scoped and enforced,
- policy and execution are coupled at runtime,
- architecture maturity is labeled explicitly,
- release discipline is tied to governance evidence.

For AI reproducibility research, this creates a serious environment for experiments that need stronger replay guarantees than "statistically similar outputs." For systems research, it offers a living case study in how ISA/VM/language/governance can be co-designed under explicit determinism constraints.

T81 should be read as part of a broader determinism movement: constrained, auditable execution surfaces replacing optimistic assumptions about reproducibility.

---

## 7. Call to Action

If you work in this space, the useful response is technical pressure-testing.

- Reproduce and challenge determinism claims on verified surfaces across architectures.
- Expand conformance depth where registry status is still partial.
- Stress VM dispatch, trap semantics, and policy hooks under adversarial workloads.
- Critique opcode coherence and freeze-boundary discipline from an ISA design perspective.
- Audit Axion policy enforcement paths for fail-open/fail-closed edge cases.
- Run deterministic inference experiments and report where guarantees hold or break.
- Contribute spec/implementation drift reductions, especially on Draft-to-Beta boundaries.

T81 does not need passive endorsement. It needs rigorous scrutiny from engineers and researchers who care about deterministic infrastructure as an enforceable systems property.
