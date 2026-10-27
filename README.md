# T81 Foundation — Experimental Computing Architecture & Systems Research

**Alternative representations · Deterministic execution · Content-addressed artifacts · Policy-governed computation · Falsifiable experiments**

[![License: Apache 2.0](https://img.shields.io/badge/license-Apache_2.0-blue.svg)](LICENSE)
[![ISA: v1.9.0 Frozen](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue.svg)](spec/tisc/opcode-registry.md)
[![Research: Active](https://img.shields.io/badge/research-active-green.svg)](docs/research/RESEARCH_CHARTER.md)

T81 Foundation is an open-source **experimental computing architecture and systems research platform** investigating alternative numerical representations—particularly balanced ternary—and their interaction with deterministic execution, programming-language tooling, content-addressed storage, and policy-governed computation.

The project originated in discrete mathematics, positional number systems, Knuth-inspired investigations of computation, and experimental ternary computing. Its development has produced a software instruction-set architecture, virtual machine, language tooling, arithmetic prototypes, storage and identity mechanisms, policy controls, and hardware experiments. These components have **different maturity levels**; their existence does not imply that every proposed end-to-end architecture is implemented or validated.

**Scientific position:** T81 has demonstrated functional research subsystems and bounded mathematical, computational, and assurance properties. It has **not** demonstrated general ternary superiority over optimized binary systems, native ternary silicon efficiency, or a comparative advantage arising from integration of the complete architecture. Negative results are retained as research evidence, not hidden or generalized beyond their tested conditions.

> **Central question:** Can alternative computational representations, deterministic execution, and explicit governance—individually **or in combination**—deliver measurable advantages over functionally equivalent conventional systems? If so, under what workloads, assumptions, hardware, and assurance requirements?

______________________________________________________________________

## 1. Research Scope

T81 pursues three connected lines of inquiry:

1. **Computational representation:** What mathematical or physical properties of balanced ternary, Base-81 groupings, low-bit tensors, and adaptive representations are useful? When do encoding and conversion costs outweigh those properties?
1. **Execution and assurance:** Which guarantees can be implemented and verified for deterministic computation, artifact identity, policy enforcement, evidence generation, and replay? How do conventional systems compare when held to the same requirements?
1. **Architectural composition:** Do integrated execution, representation, storage, and governance mechanisms confer measurable benefits beyond those of their individual components? What integration boundaries are real, and which remain experimental or absent?

These questions are deliberately distinct. A successful policy test is not evidence of ternary arithmetic efficiency. A compact ternary encoding is not proof of faster inference. A passing end-to-end workflow is not, by itself, proof of a system-level advantage.

### Canonical hypothesis lineage

The original H01–H08 identifiers remain defined by the [Master Continuity Charter](docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md). Later numbered *Scientific Investigations* are experimental work packages, **not replacements for these canonical hypothesis identifiers**.

| Track          | Hypothesis                                  | Research question                                                                         | Evidence boundary                                                                                            |
| -------------- | ------------------------------------------- | ----------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| Representation | **H01 — Native Ternary Cost & Density**     | Can native ternary storage or logic reduce normalized cost at equal information capacity? | Mathematical capacity properties do not establish physical cost; software CPU results are workload-specific. |
| Representation | **H02 — Balanced Ternary Arithmetic**       | Do symmetric digits improve arithmetic, signed operations, or rounding?                   | Algebraic properties are established; tested binary-CPU implementations have not shown speed superiority.    |
| Representation | **H03 — Base-81 Digit Grouping**            | Can grouped trits improve storage or interfaces?                                          | Representation benefits must include packing, decoding, and data-movement costs.                             |
| Representation | **H04 — Adaptive Representation Tiering**   | When can T81/T243/T729 tiering improve total cost?                                        | Conditional modeling results require workload-specific validation.                                           |
| Representation | **H05 — Three-State Algebra Primitives**    | Can three-valued primitives reduce circuit or system cost?                                | RTL and VM experiments are not native ternary silicon evidence.                                              |
| Representation | **H06 — Ternary AI Inference**              | Can ternary weights or activations improve accuracy-constrained inference?                | Storage savings are distinguishable from accuracy, throughput, and energy claims.                            |
| Foundation     | **H07 — Governed Execution & Auditability** | Can governed execution improve reproducibility, control, and audit?                       | Capabilities must be compared with conventional implementations providing equivalent guarantees.             |
| Methodology    | **H08 — AI-Assisted Research Iteration**    | Can auditable AI-assisted iteration improve research quality?                             | Traceability and generated artifacts do not substitute for independent experimental validation.              |

**Additional system-level question (Investigation 08, H-S1):** Does integration create a measurable computational, assurance, or compositional advantage? H-S1 is **not** the charter's H08 and remains **inconclusive** pending valid matched controls and ablations.

______________________________________________________________________

## 2. Why T81 Is a Research Instrument

T81 provides a common experimental setting in which representation, execution, artifact identity, and authorization can be studied both separately and together. Its value as a research instrument is **not contingent on proving that ternary computing is universally superior**. It depends on whether the project can produce reproducible experiments, independent controls, inspectable traces, and results that discriminate among competing explanations.

A conventional software stack can implement many of the same mechanisms. That is a feature of a good comparison, not a defect: the experimental task is to determine which properties come from the representation, which from implementation choices, and which from governance or composition.

### What makes the research program distinctive

| Research capability                                | Experimental use                                                                      | Required caution                                                          |
| -------------------------------------------------- | ------------------------------------------------------------------------------------- | ------------------------------------------------------------------------- |
| Shared TISC semantics and T81VM execution          | Hold program behavior constant while changing representations or execution strategies | ISA stability does not establish equal implementation quality             |
| Balanced-ternary and Base-81 prototypes            | Compare arithmetic, packing, conversion, and data movement under controlled workloads | Software emulation costs must not be extrapolated to hypothetical silicon |
| CanonFS artifact identity                          | Pin experimental inputs, outputs, and evidence to exact content                       | Hashes establish content identity, not the truth or provenance of claims  |
| Axion authorization experiments                    | Test policy enforcement, denials, and accountable execution                           | Equivalent binary/conventional controls are necessary                     |
| Research regressions and historical source lineage | Preserve failures and reproduce or falsify prior results                              | A passing regression is narrower than independent scientific replication  |
| Candidate integration paths                        | Test whether composing mechanisms produces measurable interactions                    | Conceptual adjacency is not a verified end-to-end call path               |

### Novelty and provenance

T81 combines several categories of work that should not be conflated:

1. **Established foundations:** balanced ternary mathematics, virtual machines, content addressing, deterministic execution techniques, and policy enforcement all have prior art.
1. **Project-specific engineering:** TISC semantics, T81VM, T81Lang, Axion, CanonFS, and the associated experimental harnesses constitute implementations whose design and behavior can be inspected and tested. Their existence alone does not establish scientific novelty.
1. **Open architectural hypotheses:** adaptive representation tiering, ternary-native datapaths, assurance-aware execution composition, and potential system-level interactions remain questions to test, not discoveries to advertise.

Any future claim of novelty should identify its closest prior art, precise difference, reproducible artifact, and experimentally meaningful consequence. Novel source code is not necessarily a novel computational principle.

### Experimental interfaces and boundaries

The research program distinguishes **representation** (how values are encoded), **semantics** (what operations mean), **implementation** (how operations execute), **physical substrate** (what hardware realizes them), and **assurance** (which properties can be independently checked). An advantage at one layer must not be silently attributed to another.

For example, a compact weight encoding can reduce raw storage without improving end-to-end inference latency; a policy denial can be correct without requiring TISC; and a ternary gate-count estimate can be promising without demonstrating energy savings in fabricated hardware.

______________________________________________________________________

## 3. Implemented Research Components

The repository contains C++23 components, CLI tools, tests, and experimental implementations. The following labels describe **reported implementation scope**, not a blanket production-readiness certification.

| Component                                     | Role                                                                     | Representative evidence                                                                       | Important limit                                                                                |
| --------------------------------------------- | ------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------- |
| **TISC ISA v1.9.0**                           | Frozen instruction semantics and opcode registry                         | [`spec/tisc/opcode-registry.md`](spec/tisc/opcode-registry.md), `include/t81/isa/opcodes.hpp` | A frozen software ISA does not imply native hardware or performance advantage.                 |
| **T81VM**                                     | Interpreter for TISC programs and deterministic execution profiles       | `vm/vm.cpp`, VM determinism tests                                                             | Guarantees are scoped to tested operations, platforms, and determinism contracts.              |
| **T81Lang**                                   | Parsing, intermediate representation, and compilation tooling            | `lang/frontend/`, `src/lang/CMakeLists.txt`, `t81 code build`                                 | Research language tooling, not an established general-purpose ecosystem.                       |
| **Axion**                                     | Policy evaluation and execution authorization                            | `kernel/axion/`, `src/axion/CMakeLists.txt`, controlled authorization tests                   | Tested fail-closed behavior is not universal bypass resistance or comparative superiority.     |
| **CanonFS**                                   | Content-addressed artifacts, integrity checking, and interchange         | `fs/`, [RFC-00D1](spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)               | Content identity does not by itself prove authenticity, correctness, or trust in the producer. |
| **Arithmetic and representation experiments** | Balanced ternary operations, packed weights, exact conversion prototypes | `experiments/`, `benchmarks/`, research reports                                               | Conversion cost, portability, and conventional binary controls remain central.                 |
| **Model intake**                              | GGUF/Safetensors metadata and tensor inspection                          | `tools/model/`                                                                                | Header inspection is not general-purpose LLM execution.                                        |
| **Accountable evidence**                      | Detached evidence artifacts and verification protocols                   | `examples/governance/`, [RFC-00D4](spec/rfcs/RFC-00D4-accountable-decision-bundle.md)         | Evidence protocols can be implemented independently of T81VM.                                  |
| **Witness and hardware prototypes**           | Experimental receipts, trust mechanisms, and RTL datapaths               | `experiments/` and associated PRs                                                             | Experimental code is not automatically production-integrated or physically validated.          |

A useful distinction when reading the repository:

- **Implemented:** an executable source path exists.
- **Tested:** specified cases pass in a documented environment.
- **Measured:** a reproducible experiment quantifies an outcome.
- **Comparatively established:** matched controls support a claimed advantage over an equivalent alternative.

No single test count, badge, or README diagram establishes all four levels.

______________________________________________________________________

## 4. Architecture: Components and Integration Boundaries

The following diagram is a **conceptual map**, not a claim that every edge is exercised by one verified end-to-end program:

```text
                Source / Model / Input Artifacts
                              |
             +----------------+----------------+
             |                                 |
      T81Lang / TISC                       CanonFS
      compilation path                identity & storage
             |                                 |
             v                                 |
       +-----------+                           |
       |  T81VM    | <--- artifact / tensor ---+
       | execution |                           |
       +-----------+                           |
             ^                                 |
             | authorization                   |
           Axion                               |
        policy checks                          |
             |                                 |
             +------------+--------------------+
                          |
                Results / provenance /
                evidence / verification
```

Evidence of integration must come from **actual call paths and executable tests**, not simply from the presence of adjacent subsystems.

Investigation 08 identified candidate connections among TISC, T81VM, T81Lang, Axion, CanonFS, and the `t81 ai task assess-fixed` workflow. However, its benchmark baseline and several proposed ablations were not functionally equivalent. Consequently, the investigation **does not establish** an integrated performance, assurance, or compositional advantage. See [Investigation 08](docs/research/INTEGRATED_SYSTEM_ADVANTAGE_REPORT.md) and the merged [PR #597 corrections](docs/research/INVESTIGATIONS_07_08_CORRECTIVE_ASSESSMENT.md). The corrected baseline still branches on input words rather than interpreting model weights; selected output parity does not establish equivalent computation or comparative assurance. The [alignment audit](docs/research/ROOT_DOCUMENTATION_ALIGNMENT_AUDIT.md) records the remaining evidence limits.

The central architectural research task is to establish a verified end-to-end path and compare it with a conventional implementation performing **the same computation, authorization, persistence, integrity checks, and replay**.

______________________________________________________________________

## 5. Experimental Evidence and Limits

### Balanced ternary software arithmetic

A reported Base-81 software optimization reduced tryte-addition latency from approximately **105.3 ns to 11.7 ns** in the tested implementation, while a native binary integer-addition comparison measured approximately **1.1 ns**. The evidence supports a negative conclusion for that particular software approach and baseline—not a proof that all ternary arithmetic or future native hardware is slower.

See [`docs/research/TERNARY_ARITHMETIC_AND_RADIX_COMPARISON.md`](docs/research/TERNARY_ARITHMETIC_AND_RADIX_COMPARISON.md).

### Packed ternary matrix-vector computation

A reported AVX2 packed two-bit ternary implementation achieved approximately **12.91 GFLOP/s**, versus **17.15 GFLOP/s** for an INT8 AVX2 comparison on the documented workload. The two-bit representation uses **0.25 bytes per weight**, versus four bytes for FP32; this is a **93.75% raw weight-storage reduction** relative to FP32, before metadata and other overhead. The measured CPU throughput was lower despite the compact representation. Comparisons to equally compact low-bit binary encodings must be made separately.

See [`docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md`](docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md).

### Governed execution and accountable evidence

The repository reports tests of pre-dispatch policy enforcement and detached evidence protocols. A conventional Wasmtime/Open Policy Agent comparison reportedly achieved the same tested zero-breach outcome and evidence auditability with less host-code complexity. This supports the view that **accountable computation is not inherently dependent on a ternary VM**.

See [`docs/research/consolidation/README.md`](docs/research/consolidation/README.md) and [RFC-00D4](spec/rfcs/RFC-00D4-accountable-decision-bundle.md).

### Exact numerical conversion

Investigations 01–04 explored binary64/ternary conversion boundaries, exact-rational approaches, and independent validation. These experiments establish useful numerical correctness evidence within their tested domains, alongside conversion overhead and portability limitations. They do not demonstrate a general numerical performance advantage.

### Investigation 07 — Ternary-weight hardware feasibility

[PR #595](https://github.com/t81dev/t81-foundation/pull/595) compared a general INT8 multiplier design, an optimized binary design for weights in `{-1, 0, +1}`, and an H-T1 sign/mask datapath. At `N = 64`, the report gives estimated generic synthesis areas of **36,547.5**, **6,564.5**, and **5,980.5**, respectively. The H-T1 design therefore showed approximately **1.10× lower area** than the optimized binary control, not an established intrinsic ternary advantage.

The report did **not** measure frequency, throughput-normalized area efficiency, or energy. Subsequent code review identified testbench parameterization and synthesis-failure handling defects and challenged the use of raw gate area to trigger a broader research stopping criterion. [PR #597](https://github.com/t81dev/t81-foundation/pull/597) corrected the parameter overrides and synthesis-failure handling, and retained frequency, throughput, and energy as unmeasured. The findings remain **bounded**; they do not terminate all possible ternary hardware research.

See [`experiments/ternary_hardware_feasibility/FINDINGS_REPORT.md`](experiments/ternary_hardware_feasibility/FINDINGS_REPORT.md).

### Investigation 08 — Integrated system advantage

[PR #596](https://github.com/t81dev/t81-foundation/pull/596) attempted to compare integrated T81 execution with a conventional stack and to measure subsystem interactions. Review identified a baseline that did not execute equivalent model semantics, invalid arithmetic and storage ablations, hard-coded verification times, mismatched replay operations, and an unsupported positive compositional verdict.

**Scientific disposition: INCONCLUSIVE.** Its reported latency ratios are **not valid matched architectural performance comparisons**. Neither a system-level advantage nor a system-level disadvantage has been established by that experiment.

See [`docs/research/INTEGRATED_SYSTEM_ADVANTAGE_REPORT.md`](docs/research/INTEGRATED_SYSTEM_ADVANTAGE_REPORT.md). Read it alongside the [corrective assessment](docs/research/INVESTIGATIONS_07_08_CORRECTIVE_ASSESSMENT.md). PR #597 marked C–F unimplemented, but its baseline still hard-codes classifications and its harness does not measure Config A re-execution. Comparative assurance remains unestablished.

______________________________________________________________________

## 6. What T81 Does Not Yet Establish

T81 is **not** presently demonstrated to be:

- A commercial desktop, server, or general-purpose operating system.
- A physically realized native ternary processor or proof of native ternary silicon efficiency.
- A generally faster alternative to optimized binary arithmetic or inference.
- A full general-purpose LLM inference platform solely by virtue of model metadata support.
- Universally bit-exact across every operation, host architecture, compiler, and deployment mode.
- More secure, more auditable, or more efficient than all conventional systems offering equivalent guarantees.
- A fully validated integrated architecture with measured emergent or compositional advantages.

These are **evidence boundaries**, not declarations that future research is impossible. Conversely, untested possibilities should not be advertised as demonstrated capabilities.

______________________________________________________________________

## 7. Reproduce the Existing Work

### Prerequisites

- C++23-capable compiler (for example, GCC 13+, supported Clang, or supported MSVC configurations)
- CMake 3.25+ and Ninja
- Python 3.10+
- Git

Some experimental targets and dependencies are platform-specific. A successful core build does not imply every experiment is portable.

### Build and test

```bash
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

### Research regressions

This opt-in slice requires Ninja and OpenSSL 3 with Ed25519 support. Use a separate build directory: the runner selects its own CMake generator and flags. For offline configuration, append `--asio-source /path/to/asio-1-30-2`. It runs regressions rather than reproducing historical timings.

```bash
python3 scripts/ci/run_research_regressions.py --build-dir build-research
```

### CanonFS import, export, and policy-denial example

```bash
tmp_dir="$(mktemp -d)"
canon_root="$tmp_dir/.t81_canonfs"
artifact="examples/storage-and-canonfs/canonfs-interchange/v1/model.t81w"
deny_policy="examples/storage-and-canonfs/canonfs-interchange/v1/policy-deny-all.apl"

./build/t81 canonfs import "$artifact" --canonfs-root "$canon_root" --json

canon_hash="$(./build/t81 canonfs import "$artifact" --canonfs-root "$canon_root" --json | grep -o '"hash": "[^"]*"' | head -n 1 | cut -d'"' -f4)"

./build/t81 canonfs export "$canon_hash" --canonfs-root "$canon_root" --out "$tmp_dir/restored.t81w" --json

./build/t81 canonfs import "$artifact" --canonfs-root "$canon_root" --policy "$deny_policy" --json
```

The final command is intended to exercise a denial path; a nonzero exit may be expected. Consult the example's test contract before interpreting its result.

**Reproducibility principle:** Run the original scripts and inspect their raw outputs, tool versions, inputs, and controls. A committed report is evidence of a reported result, not independent proof that its interpretation is correct.

______________________________________________________________________

## 8. Research Lineage

T81's development has evolved through several connected stages:

1. **Discrete mathematics and Knuth-inspired investigation:** Positional representations, balanced ternary arithmetic, and alternative computational processes.
1. **Hanoi CWEB experiments:** Early algorithmic exploration, later subjected to source-lineage reconstruction and arithmetic correctness probes; historical code is not presumed correct merely because it is foundational.
1. **TISC, T81VM, and T81Lang:** Formalization of a software instruction set, executable interpreter, and language toolchain.
1. **Axion, CanonFS, and evidence protocols:** Experiments in governed execution, reproducible artifacts, identity, and verification.
1. **Comparative scientific investigations:** Independent numerical oracles, optimized binary baselines, software performance studies, hardware RTL feasibility, and integrated-system evaluation.

The evolution of the project is part of its research record. Earlier ideas may be revised or rejected without erasing their provenance.

______________________________________________________________________

## 9. Open Problems and Future Experimental Opportunities

The following are **research directions**, not implemented capabilities or predictions of success. They are intended to expand the hypothesis space while keeping the evidence standard unchanged.

| Opportunity                                   | Falsifiable question                                                                                                                       | Minimum credible experiment                                                                                                       |
| --------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------- |
| Native multi-valued hardware                  | Can a physically realistic three-state device or circuit outperform a matched binary design at equal information capacity and reliability? | Characterized device assumptions, synthesis or measured silicon, normalized area/energy/delay, variability and interface overhead |
| Workload-specific balanced ternary arithmetic | Are there signed, accumulation, rounding, or decision workloads where balanced ternary lowers total cost?                                  | Correctness oracle, optimized binary baseline, conversion included, multiple workload sizes                                       |
| Adaptive T81/T243/T729 representation tiers   | Can tier transitions reduce end-to-end resource use compared with a fixed representation?                                                  | Explicit transition policy, measured switching costs, fixed-tier controls and ablations                                           |
| Low-bit inference and data movement           | Do ternary weights improve total inference efficiency at a fixed accuracy target?                                                          | Matched quantization quality, optimized low-bit binary baselines, memory/latency/energy accounting                                |
| Deterministic, auditable execution            | Which assurance guarantees can be obtained with acceptable overhead?                                                                       | Same workload and guarantees in T81 and a conventional stack; fault injection and independent verification                        |
| Composition effects                           | Does combining identity, policy, execution, and replay produce a measurable benefit beyond individual mechanisms?                          | Verified end-to-end trace, equivalent conventional control, valid component ablations                                             |

**Hardware uncertainty is not hardware evidence.** Binary CPUs are appropriate for studying semantics, conversion overhead, algorithmic behavior, and software integration. They are not sufficient to settle the efficiency of future native ternary devices. Conversely, a hypothetical device cannot be used to claim current speed or energy advantages without a transparent, validated physical cost model.

### A disciplined path from idea to result

For each new investigation, record the hypothesis and possible falsifier; establish a correctness oracle; choose the strongest feasible conventional control; match functional and assurance requirements; collect raw results and uncertainty; attempt independent reproduction; and classify the outcome as supported, refuted within scope, or inconclusive. Preserve negative findings and avoid changing acceptance criteria after seeing results.

### What success would look like

A successful T81 investigation need not demonstrate a universal architecture. It may establish a **narrow, reproducible advantage** for a specific workload, reveal an unexpected limitation, produce a better experimental method, or show that a proposed mechanism adds no value under realistic constraints. Each outcome improves the research program if the evidence is trustworthy.

______________________________________________________________________

## 10. Research Priorities

**Immediate priority: resolve remaining experimental validity limits for Investigations 07 and 08.** PR #597 corrected hardware harness defects and marked unavailable ablations explicitly. Preserve those corrections and the negative findings; establish a genuinely matched end-to-end conventional baseline before drawing integrated performance or assurance conclusions.

With those corrections recorded, the next research choice should be based on evidence rather than an assumed architectural destination. Candidate directions include:

- **Integrated-system equivalence and ablation:** Verify one real path through model execution, policy, artifact identity, and replay, then compare equivalent guarantees and costs.
- **Cross-platform determinism:** Test precisely scoped numerical and execution traces across supported targets, with measured performance overhead.
- **Representation and hardware hypotheses:** Pursue only bounded experiments with appropriate binary controls, explicit cost models, and falsifiable stopping conditions.

Prefer experimental infrastructure that can support competing hypotheses over features that presuppose one architectural destination. Avoid adding production features merely to make an experiment succeed. An **INCONCLUSIVE** result with a documented missing integration seam is preferable to a misleading positive or negative claim.

______________________________________________________________________

## 11. Primary References

- [Research charter](docs/research/RESEARCH_CHARTER.md)
- [Master continuity charter](docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md)
- [Full reassessment, October 2026](docs/research/T81_FULL_REASSESSMENT_2026-10-25.md)
- [Ternary research closure and next-hypothesis decision](docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md) — historical dispositions, subject to later evidence
- [Research consolidation ledger](docs/research/consolidation/README.md)
- [TISC opcode registry](spec/tisc/opcode-registry.md)
- [CanonFS foreign filesystem interchange — RFC-00D1](spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)
- [Accountable decision bundle — RFC-00D4](spec/rfcs/RFC-00D4-accountable-decision-bundle.md)
- [Investigation 07 — PR #595](https://github.com/t81dev/t81-foundation/pull/595)
- [Investigation 08 — PR #596](https://github.com/t81dev/t81-foundation/pull/596)

______________________________________________________________________

## 12. Contributing

Contributions are welcome when they improve correctness, reproducibility, experimental controls, source traceability, or clearly scoped research hypotheses. See [CONTRIBUTING.md](CONTRIBUTING.md) and [AGENTS.md](AGENTS.md).

When reporting an advantage, specify the workload, baseline, equivalent guarantees, environment, raw measurements, uncertainty, and limits. Negative and inconclusive findings are valid contributions.

## License

T81 Foundation is licensed under the [Apache License, Version 2.0](LICENSE).
