# T81 Foundation Project Profile

## 1. Executive Positioning

T81 Foundation delivers bit-exact, policy-enforced deterministic execution for governed computation surfaces, with a ternary-native data model and immutable provenance substrate in CanonFS.

T81 Foundation is a deterministic, policy-gated runtime stack for auditable computation, with a long-term systems direction rooted in ternary-native design. The project should not be read as a conventional application framework or as a finished operating system. Its strongest current claim is narrower and more concrete: for explicitly governed surfaces, identical inputs, policy state, and configuration should produce identical outputs and evidence across supported platforms.

Today, the most credible reading of T81 is:

- a deterministic execution environment,
- a policy enforcement layer,
- an immutable content/provenance substrate,
- and an emerging systems architecture that aims to make those properties native rather than bolted on.

The problem class is deterministic execution under governance constraints. In practical terms, T81 concentrates on a set of concerns that are usually fragmented across unrelated tools:

- canonical representation and numeric behavior,
- reproducible virtual-machine and runtime execution,
- enforceable runtime policy decisions,
- immutable storage and provenance capture,
- and explicit evidence about what was executed, imported, exported, or denied.

This remains uncommon in mainstream stacks because most environments optimize first for throughput and convenience, then try to patch determinism, evidence, and governance afterward. That usually leaves output drift, policy gaps, and poor post-mortem auditability. T81 inverts that order: determinism, policy observability, and provenance are intended to be structural properties of the stack.

T81 also aligns philosophically with high-assurance engineering practices reflected in sources such as **NIST SP 800-218 (SSDF)** and **NIST SP 800-53**, but its value should be understood in implementation terms rather than compliance language alone. The practical question T81 is trying to answer is: can an AI-oriented runtime be made reproducible, governed, and inspectable by design?

---

## 2. Architectural Overview

T81 is organized as a layered stack with explicit authority boundaries and maturity labels.

### Base-81 / balanced ternary data model

The data model is centered on ternary semantics and canonical forms. In implementation terms, the codec layer includes balanced Base-81 packing and unpacking constraints, while the type system specifies deterministic canonicalization requirements for core numerics such as `T81BigInt`, `T81Fraction`, and bounded `T81Float` behavior.

Why this layer exists: deterministic systems fail when representational ambiguity leaks upward. Canonicalization at the data boundary reduces that ambiguity before execution begins.

### Deterministic runtime and virtual machine

The runtime layer provides the most important currently usable value in the project. T81 includes a deterministic VM/runtime surface, CLI tooling, and supporting tests intended to keep core execution behavior stable across platforms. This is the part of the project that is easiest to explain to a technically skeptical reader because it produces repeatable behavior, contract tests, and CI-visible outcomes.

### Axion policy enforcement

Axion is the policy system that turns T81 from a deterministic runtime into a governed runtime. The project’s architectural stance is that policy should be able to deny or constrain actions before side effects occur, not merely annotate those actions after the fact. This policy-first stance is central to how T81 frames import/export, execution, and future networking work.

### CanonFS immutable storage and provenance

CanonFS is the immutable storage and provenance substrate. It gives T81 a way to talk about content identity, manifests, and evidence without depending on mutable host state as the sole source of truth. Recent work around RFC-00D1 moved this from a purely architectural idea toward a real interchange lane, with import/export flows, schemas, validation, and policy-profile recording.

### TernaryOS and systems direction

The kernel, HAL, QEMU guest, and related subsystems represent the long-term systems direction. They matter strategically because they show how T81’s determinism, policy, and provenance ideas could extend below a user-space runtime boundary. They should still be understood as earlier and less stable than the core runtime story. They are real and increasingly demonstrable, but they are not yet the simplest way for a newcomer to understand or adopt the project.

---

## 3. What Works Now

The strongest current capabilities are:

- a working CLI/runtime surface,
- deterministic execution and contract validation infrastructure,
- Axion policy machinery,
- CanonFS content and provenance concepts with a growing implementation surface,
- Docker and install paths for trying the project,
- and QEMU guest demos that make the broader systems direction concrete.

This matters because T81 is no longer just a specification corpus. A newcomer will find code, tests, CI, workflows, RFCs, validation tooling, and concrete subsystem seams that can be extended.

A practical way to read this section is: T81 already supports a real governed interchange path. A contributor can import host content into CanonFS, attach policy, emit validated JSON artifacts, and export back out again through the same contract surface.

Recent progress has been especially meaningful in the CanonFS interchange lane:

- `canonfs import` and `canonfs export` now exist as real CLI flows,
- RFC-00D1 has companion schema artifacts,
- JSON contracts are validated in tooling and at runtime,
- the interchange logic has been extracted into reusable core modules,
- and policy profiles are recorded in the resulting artifacts.

RFC-00D1 status: partially implemented. Import/export flows, schemas, runtime validation, and policy-profile recording are live in the CLI and core interchange modules.

This is currently the clearest example of a T81 draft RFC turning into a buildable subsystem.

---

## 4. What Is Stable, Draft, And Experimental

### Stable enough to build against

The following areas are the safest places for another engineer to start:

- the CLI/runtime path,
- core deterministic data and VM surfaces,
- Axion policy concepts,
- CanonFS interchange contracts introduced under RFC-00D1,
- and the accepted RFC corpus that already anchors major architecture decisions.

“Stable enough” here does not mean frozen forever. It means the project now has enough implementation and test weight in these areas that contributions can be made without re-litigating the whole architecture.

### Draft but meaningful

The most important active draft areas are:

- RFC-00D1 CanonFS foreign filesystem interchange,
- RFC-00D0 base-81-aware TCP/IP stack architecture,
- and contributor-facing handoff/roadmap material that is still being tightened.

RFC-00D1 is already partially embodied in code. RFC-00D0 is still primarily architectural and should remain design-led until a narrower implementation seed is chosen.

### Experimental

The following should still be treated as experimental:

- broader TernaryOS ambitions beyond the currently demonstrated boot surfaces,
- native hardware bring-up beyond emulated or tightly controlled environments,
- any assumption that the full stack is product-hardened end to end,
- and any interpretation that T81 is already a general-purpose operating system.

This distinction matters. The project is most credible when it is explicit about what is proven, what is emerging, and what is still aspirational.

---

## 5. Why T81 Is Distinct

T81 is unusual because it combines several concerns that are often separated:

- ternary-native representation and Base-81 semantics,
- deterministic execution as a first-class requirement,
- policy enforcement before side effects,
- immutable content/provenance handling,
- and a serious attempt to carry those properties downward into system architecture rather than leaving them as application-level conventions.

Many reproducibility efforts focus mainly on build artifacts, packaging, or container boundaries. Many policy systems are advisory or post-facto rather than pre-side-effect controls. Many provenance systems capture outputs after execution rather than making canonical representation and content identity structural from the start.

T81 is trying to close those gaps in one coherent stack. That ambition is what makes the project distinctive, but it is also what creates the main adoption challenge: the project is easier to respect than to classify quickly.

---

## 6. Current Risks And Friction

The main risks are not conceptual emptiness; they are transfer and execution risks.

- Scope risk: the repo spans runtime, language, policy, storage, inference, kernel, HAL, and RFC work.
- Interpretation risk: the architecture is more legible to the original author than to a new maintainer unless the handoff path is kept current.
- Maturity skew: some areas are well specified and increasingly implemented, while others are still design-heavy.
- Perception risk: newcomers may read T81 as either an overextended research project or an operating-system effort first, when its strongest current value is the deterministic governed runtime.

These risks are manageable if the public story stays narrow and the next work remains focused on buildable lanes rather than constant breadth expansion.

---

## 7. Best Near-Term Development Shape

If another engineer were picking up T81 today, the most productive path would be:

1. start from the CLI/runtime and CanonFS interchange lane,
2. use the handoff and roadmap documents to choose a bounded task,
3. treat RFC-00D1 as the best current draft-to-implementation bridge,
4. defer RFC-00D0 implementation breadth until the resolver/descriptor surface is narrowed further,
5. and keep `main` operationally boring through strong CI discipline.

That is the correct development shape for the project at this stage. T81 does not need maximum subsystem count. It needs a few finishable lanes that prove the broader architecture can be handed off and extended by someone other than the original author.

---

## 8. Recommended Reading Order

For someone discovering the project, the best reading order is:

1. `README.md` for the current public story and first-run paths,
2. `docs/HANDOFF.md` for what is mature, what is draft, and how to approach the codebase,
3. `docs/ROADMAP.md` for the current sequencing of work,
4. `docs/BUILDABLE_NEXT_STEPS.md` for bounded contributor lanes,
5. RFC-00D1 for the best current example of a draft architecture turning into code,
6. RFC-00D0 for the networking direction that remains design-led.

This order is important. T81 becomes easier to understand when it is approached as a governed deterministic runtime with a long-term systems direction, rather than as a bare-metal OS project first.
