# T81 Research Program — Master Continuity Charter

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Research Program — Master Continuity Charter](#t81-research-program-—-master-continuity-charter)
  - [0. Read this first: instructions for the next research instance](#0-read-this-first-instructions-for-the-next-research-instance)
  - [1. Foundational mission and boundaries](#1-foundational-mission-and-boundaries)
  - [2. Intellectual and architectural lineage](#2-intellectual-and-architectural-lineage)
    - [Stage A — Knuth and discrete mathematics](#stage-a-—-knuth-and-discrete-mathematics)
    - [Stage B — Hanoi (CWEB precursor)](#stage-b-—-hanoi-cweb-precursor)
    - [Stage C — Early base-81 / ternary ecosystem](#stage-c-—-early-base-81--ternary-ecosystem)
    - [Stage D — Modern T81 Foundation](#stage-d-—-modern-t81-foundation)
  - [3. The October 25 validation-integrity warning](#3-the-october-25-validation-integrity-warning)
  - [4. Hypothesis register — initial edition](#4-hypothesis-register-—-initial-edition)
  - [5. Experimental standards and controls](#5-experimental-standards-and-controls)
  - [6. Two coordinated tracks, not a forced choice](#6-two-coordinated-tracks-not-a-forced-choice)
  - [7. Immediate executable research sequence](#7-immediate-executable-research-sequence)
  - [8. Source inventory and confidence](#8-source-inventory-and-confidence)
  - [9. Session handoff protocol (append each time)](#9-session-handoff-protocol-append-each-time)
  - [10. Ready-to-paste prompt for the next ChatGPT instance](#10-ready-to-paste-prompt-for-the-next-chatgpt-instance)

<!-- T81-TOC:END -->


**Version:** 1.0 · **As of:** 2026-10-25
**Status:** Research continuity and hypothesis register; not a certification of implementations
**Intended readers:** Future ChatGPT instances, Codex, Jules, collaborating researchers, and project maintainers.

## 0. Read this first: instructions for the next research instance

**Do not begin by asking the researcher to restate the project's purpose or choose between productization, governance, and ternary research.** That question has already been resolved for the present phase. The primary mission is to determine whether ternary or alternative computational representations provide a defensible optimization or new architectural advantage, possibly on hardware not yet available. Governance/runtime engineering is a supporting, partly independent research branch, not proof of ternary superiority.

Start with this charter, the attached historical analyses, and actual repository evidence. Continue work by producing a concrete finding, falsifiable hypothesis, correction, or bounded experiment. Ask questions only when an answer is truly blocking and cannot be recovered from records. Do not generate large codebases or more elaborate runtime architecture merely to demonstrate activity. Explicitly separate source claims, reproduced tests, modeled outcomes, hypotheses, and speculation. Do not treat AI-generated prior analyses as independent verification.

**Immediate continuation task:** Reconstruct a source- and commit-grounded lineage of Hanoi's original computational hypotheses through T81Lang, T81VM/TISC, Axion, and T81 Foundation. Build a versioned hypothesis register, then select a small discriminating experiment that tests ternary-specific advantage against equally optimized binary/adaptive baselines. Preserve ongoing PR/CI investigations separately; check their current status before taking action.

## 1. Foundational mission and boundaries

The researcher was studying Donald Knuth, discrete mathematics, recursion, computational models, and literate programming before the current T81 Foundation architecture existed. The earliest known precursor, **Hanoi**, was written in `.cweb` and explored a recursive ternary VM with symbolic dispatch and workload-dependent tiers. Subsequent projects explored base-81 arithmetic, language and VM infrastructure, adaptive AI control, determinism, artifact identity, authorization, and governed execution.

The user describes the research as an iterative, sometimes hodgepodge exploration of what generative AI can accomplish as a design and research collaborator. This is exploratory research, not a commitment to deliver a market-ready operating system, CPU, or commercial AI product. It is acceptable for experiments to fail, architectures to change, and competing ideas to coexist. What is not acceptable is to confuse implemented software with validated scientific advantage.

**Core question:** Under what assumptions, workloads, cost functions, and possible physical substrates can ternary computation offer advantages over comparable binary or other-radix alternatives?

**Companion question:** Can representation-independent computational objects retain stable identity, deterministic semantics, capabilities, and evidence across execution environments? This is useful systems research but does not establish the core ternary hypothesis.

**Research dimensions:** (1) mathematical/algorithmic: what can be represented and at what abstract cost; (2) architectural: what native primitives, scheduling, data paths, and memory models could exploit it; (3) physical: what devices and engineering costs could realize it. A negative result on binary emulation does not refute a hypothetical native ternary device; a positive result on binary hardware does not by itself establish radix-specific advantage.

## 2. Intellectual and architectural lineage

### Stage A — Knuth and discrete mathematics

Research into algorithms, recursive functions, representations, finite-state machines, graphs, relations, automata, formal languages, circuits, and complexity. The previously discussed *Discrete Mathematics for Enhancing Large Language Models* is part of this intellectual history; it is not itself a formal ternary-vs-binary proof. The original paper should be consulted directly when available.

### Stage B — Hanoi (CWEB precursor)

Repository: https://github.com/t81dev/Hanoi

Reported source-level findings from the October 25 archaeology:

- `t81recursion.cweb`: recursive factorial/Fibonacci/dispatch and depth tracking; correctness not independently established.
- `tier_scheduler.cweb`: heuristic tier selection using entropy, recursion depth, and symbolic intent. Reported thresholds: T729 for entropy/depth >100 or `AI` intent; T243 for >40; T81 otherwise. These are rules, not proven optima.
- `hvm_promotion.cweb`: promotion/demotion logic between modes, including tensor-oriented paths; integration requires verification.
- `hanoivm_vm.cweb`: opcode dispatch with null handlers and repeated operation-table definitions reported; possible reachability/completeness problems.
- `ternary_arithmetic_optimization.cweb`: balanced digit semantics reportedly inconsistent with normalization to 0/1/2; multiplication reportedly componentwise rather than positional integer multiplication. Treat these as source-review findings requiring executable reproduction.

**Historical tier intent (as described):** T81 for operand/register-like logic; T243 for symbolic large-integer and finite-state computation; T729 for tensors, AI macros, and semantic dispatch. The tier labels are not by themselves three distinct models of computation. Domains, operations, conversion maps, invariants, and transition costs must be specified. Scheduler tier labels 81/243/729 and promotion modes 0/1/2 were reported as inconsistent absent explicit mapping.

**Disposition:** Preserve Hanoi as an archive of hypotheses and source evidence; do not rewrite it before extracting formal specifications and provenance.

### Stage C — Early base-81 / ternary ecosystem

Primary historical artifact: *TYRNARY - T81Analysis(20261025-145658).pdf*, 34 pages, titled *Base-81 (T81) and Trinary Computing: A Theoretical Analysis*. Its discussion covers radix economy, representation density, balanced vs unbalanced ternary, arithmetic/carry, hardware assumptions, SIMD, multithreading, Axion scheduling, low-precision AI, floating point, cryptography, and post-binary interoperability. It also contains a T81 system profile.

The historical ecosystem includes T81 data types, T81Lang, TISC/T81VM, and AxionAI. Treat its architecture diagrams and descriptions as *documented design intent* until code and tests establish the corresponding features. In particular, claims concerning AVX2, Karatsuba, JIT, GPU, adaptive precision, cryptographic utility, or speedups must be tied to exact commits, runnable tests, workload specifications, and baselines. Do not assume all named components were simultaneously integrated.

**Mathematical facts:** 81 = 3^4, so a base-81 digit groups four trits; log2(3) ≈ 1.58496 bits per trit; log2(81) ≈ 6.33985 bits per base-81 digit. At least 256 values require 8 bits or 6 trits. These are representation facts, not energy, area, latency, or physical-density measurements. On binary hardware a base-81 digit requires at least 7 fixed bits, often 8 for convenient storage. Radix economy depends on the assumed per-digit cost model; base-3 being optimal under a classical b·log_b(N) model does not imply an optimal manufactured processor.

**Important technical caution:** A binary comparator can encode a three-way result in two bits and can often compute it in one instruction. Do not equate a ternary three-way comparator with an automatic one-cycle or instruction-count advantage without an equivalent hardware/ISA baseline. Likewise, grouping four trits into a base-81 digit does not imply one physical 81-level storage device or free four-trit arithmetic.

### Stage D — Modern T81 Foundation

Primary repository: https://github.com/t81dev/t81-foundation ; related public repositories include t81-vm, t81-lang, t81lib, t81-hardware, ternary-fabric, ternary-memory-research, duotronic-thesis, digital-archaeology, and other work at https://github.com/t81dev . Their exact current state must be checked at pinned commits.

Modern emphases: ISA/VM, immutable/hash-verified artifacts, deterministic execution, CanonFS identity, integer provenance, witness durability/revocation, authorization/capabilities, pre-side-effect policy, evidence receipts, and representation-independent interfaces. The project has also investigated whether this is better described as a governed model runtime rather than a general-purpose substrate. These are distinct hypotheses from radix superiority.

Reported historical milestones (not freshly verified in this charter): PR #551 CanonFS identity experiment; #552 integer provenance; #553 representation/conversion RFC; #554 witness revocation; #560 witness corrective work; #561 next phase. PRs #565 and #566 were under validation/review in the October 25 handoff. Research Protocol 001 was marked **NOT READY** pending its evidence requirements. Do not infer that PRs were merged, checks repaired, or readiness changed without inspecting current GitHub state.

## 3. The October 25 validation-integrity warning

The previous instance summarized Codex reports indicating that failures could reproduce on `main`, including Windows sparse-checkout/CMake mismatches, AVX2/ASan dependency or toolchain issues, macOS ARM64 C++ standard configuration, missing tracked model fixtures, Linux missing-fixture preconditions, and a shell pipeline masking command failure through `tee`. These were **reported by Codex and summarized in conversation**, not independently audited here. Reproduce against pinned commits before asserting root cause.

Three independent properties must be distinguished:

1. **Implementation correctness:** code behaves as intended when a meaningful test executes.
2. **Validation integrity:** a reported pass actually means the intended checks ran and passed, without masked failures.
3. **Reproducibility:** a clean checkout with documented dependencies and fixtures yields independently inspectable results.

A failure that predates a PR establishes possible non-regression for that failure, not general merge safety or branch health. A CI job blocked before the target assertion yields no new evidence about that target property. Define a minimum supported clean-checkout baseline (potentially Linux x86_64 if evidence supports it); classify other platforms separately. Keep this maintenance milestone bounded and do not allow it to replace the original scientific mission.

## 4. Hypothesis register — initial edition

Each entry must eventually record: identifier; earliest source/commit; formal claim; implementation evidence; baseline; assumptions; measurement and falsifier; result; confidence; next action.

| ID | Hypothesis | Current status | Discriminating test |
|---|---|---|---|
| H01 | Native ternary storage/logic can reduce normalized cost at equal information capacity | Mathematical density established; physical advantage open | Sweep 3-state element area, energy, sensing, reliability, conversion and ECC vs 2-state alternatives |
| H02 | Balanced ternary improves signed arithmetic or rounding | Algebraic properties known; implementation benefit open | Equivalent signed workloads, exact semantics, optimized binary and ternary adders, carry-depth/energy comparisons |
| H03 | Base-81 grouping yields useful performance or interface advantages | 4-trit grouping mathematical; system-level benefit open | Compare grouped/ungrouped representations and binary limb designs under equal information/precision |
| H04 | Adaptive T81/T243/T729 tiering beats fixed representation | Hanoi heuristic exists; optimization unproven | Fixed binary, fixed ternary, adaptive binary, adaptive ternary; include detection, conversion, switching |
| H05 | Three-state algebra yields useful primitives or architectures | Exploratory | Formalize operation families; compare circuit complexity with equivalent Boolean synthesis |
| H06 | Ternary weights/activations improve AI inference | Plausible domain, not demonstrated here | Accuracy-constrained inference vs binary low-bit quantization, matched kernels and bandwidth |
| H07 | Representation-independent governed execution improves auditability | Separate engineering research | Clean-room replay, receipts, tamper/revocation tests, cross-platform conformance |
| H08 | AI-assisted iterative invention can generate trustworthy novel research | Methodological experiment | Track provenance, error rates, independent reproduction, failed and corrected hypotheses |

Do not conflate mathematical equivalence, software implementation, abstract machine models, and physical-device feasibility. Changing radix does not change universal computability by itself; an advantage would normally be in complexity constants, data movement, circuit topology, physical costs, or task-specific operations.

## 5. Experimental standards and controls

**Before implementation:** Define a mathematical domain and exact semantics; equivalent input ranges and precision; workload distribution; binary, ternary, and relevant higher-radix alternatives; cost units; conversion overhead; failure criteria; uncertainty; reproducibility procedure. Use a reference oracle independent of the candidate implementation.

**Primary comparisons:** fixed-width binary; fixed-width balanced/unbalanced ternary; variable-precision binary; variable-precision ternary. Consider other radices where they expose a confounder. Include ablations that remove adaptive scheduling or radix-specific primitives independently.

**Measurement categories:** digits/storage cells; primitive gate count and depth; data movement; conversion operations; latency; energy per operation; area; sensing/noise margin; error correction; precision and numerical error; task accuracy; throughput. Keep abstract-model quantities distinct from measurements on actual hardware.

**Idealized storage boundary:** with per-element costs C2 and C3, an asymptotic equal-information storage comparison favors ternary when C3/C2 < log2(3) ≈ 1.585, assuming independent symbols and no overhead. This is *not* a universal energy/latency threshold. For a finite capacity M, compare ceil(log3 M)·C3 against ceil(log2 M)·C2, then add encoding, reliability, sensing, and control costs. A workload-specific advantage requires a fuller model.

**Acceptance standard:** A favorable result must survive an equivalent optimized binary baseline, sensitivity analysis over device assumptions, correctness checks, and explicit overhead. A negative result is publishable research. Never use a README badge or green CI icon as the sole proof.

## 6. Two coordinated tracks, not a forced choice

**Track R — Invention and scientific research (primary):** Recover and formalize original Hanoi/T81 ideas; identify the most discriminating experiments; publish negative and positive results; remain open to specialized accelerators, memory-centric, hybrid binary-control/ternary-datapath systems, or future devices rather than assuming a general-purpose ternary CPU.

**Track F — Foundation and evidence apparatus (supporting):** Stabilize a minimal reproducible test baseline; preserve artifact identity and provenance; ensure receipts correspond to executed checks; complete bounded security and correctness investigations. Track F should make Track R easier to falsify and reproduce, not define its scientific goals.

**Cross-cutting Track M — AI-assisted research methodology:** Log prompt-to-code provenance, which claims originated from models, which were checked by source/tests, and which remain unverified. Avoid repeating research cycles because prior context was lost.

## 7. Immediate executable research sequence

1. **Freeze evidence references:** identify pinned commit SHAs for Hanoi, the early T81 ecosystem, and T81 Foundation; capture original analyses, source links, and prior Codex logs. Do not rewrite history.
2. **Recover genealogy:** map every substantive early claim to source files and earliest identifiable commits; mark whether direct code inheritance is proven or only conceptual continuity.
3. **Formalize H04 and H01:** specify Hanoi's tier domains and transition rules; independently derive the cost threshold for ternary elements with sensitivity analysis.
4. **Choose one minimal experiment:** prefer a test that distinguishes radix effects from adaptive-precision effects. Include adaptive binary as a control.
5. **Produce a reproducible research packet:** specification, oracle, minimal code/model, assumptions, data, commands, negative findings, and decision threshold.
6. **Maintain CI evidence separately:** verify PR #565/#566 and Protocol 001 statuses against current source before recommending merge or declaring readiness.

**Stop conditions:** Do not expand the VM or build a new scheduler if mathematical semantics remain ambiguous, if the test lacks a strong binary control, if a result depends on undisclosed fixtures, or if the apparent improvement disappears after conversion/selection costs. Revise the hypothesis or report a negative finding instead.

## 8. Source inventory and confidence

**Primary user-provided historical records:**

- `Pasted markdown(20261025-151355).md`: transcript excerpt from the preceding ChatGPT instance, including the researcher's explicit mission correction, Hanoi investigation, and October 25 architectural archaeology.
- `Pasted text(20261025-144502).txt`: standalone October 25 architectural archaeology summary.
- `TYRNARY - T81Analysis(20261025-145658).pdf`: 34-page theoretical analysis of base-81 and ternary computing, including early ecosystem claims and future hardware discussion.
- *Discrete Mathematics for Enhancing Large Language Models*: referenced in prior conversation; obtain original artifact before quoting or assessing specific passages.

**Public code entry points:** https://github.com/t81dev/Hanoi ; https://github.com/t81dev/t81-foundation ; https://github.com/t81dev . Source/commit verification is required for any current-status claim.

**Evidence tiers:** A = reproduced on pinned source with artifacts; B = inspected source with clear but unexecuted logic; C = historical report or AI-generated analysis; D = mathematical/model derivation under explicit assumptions; E = speculation/design proposal. A mathematical derivation can be strong without code, but it must state its assumptions. Historical findings in this charter are mostly C and should be promoted through independent work.

## 9. Session handoff protocol (append each time)

```text
DATE / SESSION:
REPOSITORIES AND PINNED COMMITS:
QUESTION OR HYPOTHESIS TESTED:
EVIDENCE EXAMINED (file, line, test, log, dataset):
WHAT WAS REPRODUCED:
WHAT WAS ONLY REPORTED OR MODELED:
RESULT / NEGATIVE RESULT:
KNOWN DEFECTS / CONFOUNDERS:
OPEN PRS AND CI STATUS (as checked, with timestamp):
NEXT SINGLE ACTION:
STOPPING CRITERION:
```

Keep a short changelog; never silently overwrite a failed hypothesis with a revised success story. Distinguish researcher intent from assistant interpretation. When context becomes scarce, update this handoff rather than asking broad questions that were answered already.

## 10. Ready-to-paste prompt for the next ChatGPT instance

> Continue the T81 research program using the attached **T81 Research Program — Master Continuity Charter** as the continuity record, not as proof of its technical claims. The central objective is to establish whether ternary or alternative computational representations yield defensible mathematical or architectural advantages, potentially independent of current hardware. The historical lineage begins with Knuth/discrete mathematics and Hanoi CWEB, continues through T81Lang/T81VM/TISC/Axion and early base-81 analysis, and leads to modern T81 Foundation. Preserve both scientific invention and independent governance/evidence engineering without conflating them. Do not restart with broad questions about my goals. First inspect pinned repository source and reconcile the hypothesis register against implementation evidence. Produce one substantive discovery, contradiction, or minimal falsifiable experiment with an optimized binary control. Do not assert CI readiness or PR status without checking current evidence. Separate what is mathematically established, implemented, tested, modeled, and speculative. Conclude with a concise append-only session handoff.

---

**Editorial note:** This charter synthesizes supplied historical documents and conversation records. It is a continuity artifact, not a complete independent audit of all public repositories or a verified chronology of every commit. Its purpose is to prevent context loss and make the next evidence-producing step unambiguous.
