# T81 Human–AI Research Initiative

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Human–AI Research Initiative](#t81-human–ai-research-initiative)
  - [Independent Research Collaboration, Scientific Framework Development, and Deliverable Dossier](#independent-research-collaboration-scientific-framework-development-and-deliverable-dossier)
  - [Section 1: Critical Assessment of Initiative Feasibility & Assignment 10 Answers](#section-1-critical-assessment-of-initiative-feasibility-&-assignment-10-answers)
    - [1.1 Feasibility Assessment](#11-feasibility-assessment)
    - [1.2 Assignment Answers (Questions 1–10)](#12-assignment-answers-questions-1–10)
      - [1. What would make this a legitimate and scientifically credible human–AI research initiative?](#1-what-would-make-this-a-legitimate-and-scientifically-credible-human–ai-research-initiative?)
      - [2. What is the most defensible scientific contribution already visible in T81?](#2-what-is-the-most-defensible-scientific-contribution-already-visible-in-t81?)
      - [3. What evidence is missing before the project can make stronger claims?](#3-what-evidence-is-missing-before-the-project-can-make-stronger-claims?)
      - [4. How should independent AI systems collaborate without reinforcing one another's mistakes?](#4-how-should-independent-ai-systems-collaborate-without-reinforcing-one-another's-mistakes?)
      - [5. What research governance is necessary before inviting outside contributors?](#5-what-research-governance-is-necessary-before-inviting-outside-contributors?)
      - [6. Which three research hypotheses deserve the highest priority?](#6-which-three-research-hypotheses-deserve-the-highest-priority?)
      - [7. What would justify continuing, redirecting, or terminating a research direction?](#7-what-would-justify-continuing-redirecting-or-terminating-a-research-direction?)
      - [8. How should the human researcher's original contributions and AI assistance be documented?](#8-how-should-the-human-researcher's-original-contributions-and-ai-assistance-be-documented?)
      - [9. What institutional or academic relationships would be valuable, and what prerequisites should exist before approaching them?](#9-what-institutional-or-academic-relationships-would-be-valuable-and-what-prerequisites-should-exist-before-approaching-them?)
      - [10. What should we do first?](#10-what-should-we-do-first?)
  - [Section 2: Deliverable A — Research Initiative Charter](#section-2-deliverable-a-—-research-initiative-charter)
    - [2.1 Name, Mission, and Scope](#21-name-mission-and-scope)
    - [2.2 Governance Structure & Roles](#22-governance-structure-&-roles)
    - [2.3 Scientific Principles & Rules of Engagement](#23-scientific-principles-&-rules-of-engagement)
    - [2.4 Research Ethics & Transparency](#24-research-ethics-&-transparency)
  - [Section 3: Deliverable B — Human–AI Collaboration Protocol Specification](#section-3-deliverable-b-—-human–ai-collaboration-protocol-specification)
    - [3.1 Role Architecture & Interactions](#31-role-architecture-&-interactions)
    - [3.2 Protocol Safeguards & Anti-Echo-Chamber Controls](#32-protocol-safeguards-&-anti-echo-chamber-controls)
    - [3.3 Disagreement Logging & Provenance Schema](#33-disagreement-logging-&-provenance-schema)
  - [Section 4: Deliverable C — Scientific Research Roadmap](#section-4-deliverable-c-—-scientific-research-roadmap)
    - [4.1 Prospective 12-Month Research Roadmap (Q4 2026 – Q3 2027)](#41-prospective-12-month-research-roadmap-q4-2026-–-q3-2027)
    - [4.2 Canonical Hypothesis Ledger (H01–H08) & Proposed Initiative Hypotheses](#42-canonical-hypothesis-ledger-h01–h08-&-proposed-initiative-hypotheses)
      - [Master Canonical Hypothesis Register (H01–H08)](#master-canonical-hypothesis-register-h01–h08)
      - [Proposed Initiative Hypotheses (H-INIT Namespace)](#proposed-initiative-hypotheses-h-init-namespace)
  - [Section 5: Deliverable D — Independent Research Assessment](#section-5-deliverable-d-—-independent-research-assessment)
    - [5.1 Scientific Originality Evaluation](#51-scientific-originality-evaluation)
    - [5.2 Qualitative Implementation & Verification State](#52-qualitative-implementation-&-verification-state)
    - [5.3 Synthesis of Existing Research Evidence](#53-synthesis-of-existing-research-evidence)
      - [Investigation 07: Ternary Dot-Product Hardware Feasibility](#investigation-07-ternary-dot-product-hardware-feasibility)
      - [Investigation 08: Integrated Architecture Advantage](#investigation-08-integrated-architecture-advantage)
      - [Investigation 09-E1: TISC Executable Size vs. Wasm Baseline](#investigation-09-e1-tisc-executable-size-vs-wasm-baseline)
    - [5.4 Categorization of Evidence Types](#54-categorization-of-evidence-types)
  - [Section 6: Deliverable E — Publication and External Collaboration Strategy](#section-6-deliverable-e-—-publication-and-external-collaboration-strategy)
    - [6.1 Peer-Reviewed Publication Route](#61-peer-reviewed-publication-route)
    - [6.2 Reproducibility Package Standards](#62-reproducibility-package-standards)
    - [6.3 External Academic & Community Engagement](#63-external-academic-&-community-engagement)
  - [Section 7: Deliverable F — Immediate Action Plan (Next 30 Days)](#section-7-deliverable-f-—-immediate-action-plan-next-30-days)
    - [7.1 Top 5 Action Items](#71-top-5-action-items)
      - [Detailed Action Descriptions:](#detailed-action-descriptions)
  - [Section 8: Summary Governance Sign-Off & Administrative Intake Record](#section-8-summary-governance-sign-off-&-administrative-intake-record)

<!-- T81-TOC:END -->

## Independent Research Collaboration, Scientific Framework Development, and Deliverable Dossier

**Date:** October 27, 2026
**Status:** Foundational Research Initiative Charter & Strategic Assessment (Reconciled)
**Target Surface:** `/docs/research/HUMAN_AI_RESEARCH_INITIATIVE.md`
**Primary Repository:** [t81dev/t81-foundation](https://github.com/t81dev/t81-foundation)
**Historical Precursor:** [t81dev/Hanoi](https://github.com/t81dev/Hanoi)

---

## Section 1: Critical Assessment of Initiative Feasibility & Assignment 10 Answers

### 1.1 Feasibility Assessment

Establishing a long-term, human-directed, AI-assisted research initiative centered on T81 is **technically and methodologically feasible**, subject to strictly bounded governance and epistemological constraints.

- **Technical Feasibility:** High. The T81 codebase contains functional execution engines, content-addressed identity stores, and policy evaluation kernels alongside reproducible benchmark and synthesis harnesses (such as Icarus Verilog and Yosys scripts in `experiments/ternary_hardware_feasibility/` and C++ test runners in `experiments/integrated_system_advantage/`).
- **Methodological Feasibility:** Feasible with explicit safeguards. AI models are prone to confirmation bias, hallucinations of non-existent APIs, and circular agreement ("echo-chamber validation"). A credible initiative requires a structured multi-role pipeline with mandatory adversarial falsification, blind evaluation, and automated reproducibility verification.
- **Governance Feasibility:** Essential prerequisite. Without strict human PI authorization gates and automated CI/CD governance scripts (e.g., `check_overclaim_guardrails.py`, `check_root_structure.py`), AI assistance risks generating architectural bloat and unsubstantiated claims.
- **Resource Feasibility:** High. By enforcing cold-start vs. steady-state separation and prioritizing lightweight, isolated execution environments, research can proceed under standard computing hardware and API quotas.

---

### 1.2 Assignment Answers (Questions 1–10)

#### 1. What would make this a legitimate and scientifically credible human–AI research initiative?
Scientific credibility relies on **falsifiability, transparency, blind multi-model verification, and negative result preservation**. Legitimacy is achieved when AI agents operate under adversarial roles (falsifiers, auditors) rather than promotional generators, every claim links to a bit-exact source revision and reproducible experiment, and negative findings (such as Investigation 07's ternary hardware overlay rejection) are preserved as first-class scientific outcomes.

#### 2. What is the most defensible scientific contribution already visible in T81?
The most defensible scientific contribution is T81's **Accountable Computation Substrate**: specifically, the pre-side-effect policy interception mechanism (Axion Policy Engine) coupled with cryptographic content identity (CanonFS SHA-3/256 hashing) and detached evidence verification (RFC-00D4 evidence bundles). This is verified by Investigation 08's H-S1b finding, demonstrating fail-closed policy enforcement prior to state mutation.

#### 3. What evidence is missing before the project can make stronger claims?
- **Subsystem Substitution Ablations:** Individual C++ substitution seams (Configs C, D, E, F in Investigation 08) are currently `UNIMPLEMENTED` without core code modifications.
- **Runtime Performance Parity:** Cold-start CLI process startup ($\sim 5.3 \text{ s}$) dominates T81 execution compared to in-process baselines ($\sim 0.65 \text{ ms}$). Long-running daemon/server measurements are required.
- **Off-the-Shelf Protocol Comparative Baselines:** Head-to-head empirical comparison against Wasmtime + OCI + Cosign (Project Baseline-X) under mid-flight policy mutations.

#### 4. How should independent AI systems collaborate without reinforcing one another's mistakes?
By enforcing **functional role separation and adversarial incentive alignment**:
- The *Implementation Engineer* cannot review or approve its own code or experiments.
- The *Adversarial Reviewer* is explicitly prompted and evaluated on its ability to find flaws, missing baselines, unmeasured variables, or invalid statistical claims.
- The *Reproducibility Auditor* executes scripts independently in clean sandboxes without access to prompt context.
- Cross-model reviews (e.g., Anthropic Claude, OpenAI GPT-4, Google Gemini) must evaluate artifacts blindly without model identification.

#### 5. What research governance is necessary before inviting outside contributors?
1. An explicit **Hypothesis & Evidence Classification Ledger** (SPECIFIED through CONTRADICTED).
2. Automated CI guardrails enforcing claim bounding (e.g., link checks, API semver checks, and determinism claim checks bounded by the [Determinism Surface Registry](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md)).
3. Clear attribution and provenance standards distinguishing human intent/design from AI generation.
4. An open, reproducible benchmark suite with one-command execution scripts (`run_verification.py`, `run_experiments.py`).

#### 6. Which three research hypotheses deserve the highest priority?
1. **Hypothesis H-INIT-A1 (Accountable Substrate Necessity vs Protocol):** *Does pre-side-effect policy gating require a bespoke C++ VM/ISA, or can standard Wasmtime runtimes paired with RFC-00D4 detached evidence protocols achieve identical security guarantees with $10\times$ lower latency and complexity?*
2. **Hypothesis H-INIT-R1 (Adaptive Radix & Representation Efficiency Bounds):** *Under what specific sparse vector and low-bit matrix multiplication workloads does 2-bit packed ternary SWAR arithmetic outperform bit-aligned INT2/INT4 binary SIMD on commodity AVX2/NEON hardware?*
3. **Hypothesis H-INIT-M1 (Multi-AI Adversarial Falsification Drift):** *Does a structured multi-AI adversarial pipeline (Architect $\to$ Engineer $\to$ Falsifier $\to$ Auditor) significantly reduce ungrounded architectural overclaims compared to single-agent iterative generation?*

#### 7. What would justify continuing, redirecting, or terminating a research direction?
- **Continue:** The hypothesis meets preregistered quantitative thresholds (e.g., $\ge 1.25\times$ efficiency or zero policy bypasses) in reproducible, comparative experiments.
- **Redirect:** The core outcome is achieved via a simpler mechanism (e.g., pivoting from a heavy bespoke C++ VM to a lightweight, universal Wasm evidence protocol upon confirming Outcome B in `docs/research/RESEARCH_CHARTER.md`).
- **Terminate:** The hypothesis fails its falsification threshold across matched baselines (e.g., Investigation 07's termination of the H-T1 ternary hardware sign-and-mask overlay after obtaining only a $1.098\times$ gate area reduction vs. $1.25\times$ required).

#### 8. How should the human researcher's original contributions and AI assistance be documented?
Every commit, RFC, and research report must include a **Provenance and Attribution Block** specifying:
- **Human Principal Investigator:** Research vision, hypothesis formulation, authorization, and final approval.
- **AI Collaborators:** Agent roles, model versions, prompt hashes, generated source files, and automated audit results.
- **Verification Hash:** Git commit SHA and CTest/CI execution log signature.

#### 9. What institutional or academic relationships would be valuable, and what prerequisites should exist before approaching them?
- **Valuable Relationships:** Systems, Security, and Programming Language research groups (e.g., ACM SIGPLAN, USENIX Security, IEEE Micro, OSDI/ATC communities).
- **Prerequisites:**
  1. Complete, one-command reproducibility packages running inside standard OCI container images.
  2. Peer-review-ready technical preprints with fully disclosed negative findings.
  3. Zero ungrounded marketing claims or unverified benchmarks in public documentation.

#### 10. What should we do first?
Execute **Immediate Action Plan Step 1**: Implement C++ production subsystem substitution seams (Configs C, D, E, F) to resolve the `INCONCLUSIVE` status of Investigation 08's compositional advantage hypothesis (H-S1c).

---

## Section 2: Deliverable A — Research Initiative Charter

### 2.1 Name, Mission, and Scope

- **Official Name:** T81 Open Research Initiative for Accountable Computation and Alternative Representations (T81-ORI).
- **Mission Statement:** To rigorously investigate computational representation, policy-governed execution, and scientific human–AI research methodologies through empirical, falsifiable, and independently reproducible computer science experiments.
- **Scope:**
  - **Track R (Representation & Arithmetic):** Balanced ternary arithmetic, packed SIMD representation, and adaptive radix efficiency bounds.
  - **Track F (Foundation & Governance Systems):** Deterministic execution (bounded by the [Determinism Surface Registry](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md)), content-addressed evidence infrastructure (CanonFS / RFC-00D1), and pre-side-effect policy enforcement (Axion / RFC-00D4).
  - **Track M (Methodology & Meta-Science):** Multi-AI collaborative workflows, adversarial falsification protocols, and reproducible provenance tracking.

---

### 2.2 Governance Structure & Roles

```
+-------------------------------------------------------------------+
|                  Human Principal Investigator (PI)                |
|    - Research priorities, final authorization, ethical oversight |
+-------------------------------------------------------------------+
                                  |
         +------------------------+------------------------+
         |                                                 |
+-----------------------------------+             +-----------------------------------+
|      Multi-AI Agent Network       |             |    External Reviewers & Community |
| - Architect     - Engineer        |             | - Independent Academic Reviewers  |
| - Falsifier     - Auditor         |             | - Open-Source Contributors        |
| - Synthesizer                     |             | - Verification Auditors           |
+-----------------------------------+             +-----------------------------------+
```

1. **Human Principal Investigator (PI):** Retains exclusive authority over research direction, RFC acceptance, security boundary approvals, and official publication submission.
2. **AI Collaborators:** Specialized, non-autonomous research assistants operating within strictly bounded roles and subjected to mandatory adversarial review.
3. **External Reviewers & Community:** Independent human researchers invited to audit reproducibility packages and challenge findings.

---

### 2.3 Scientific Principles & Rules of Engagement

1. **Falsification-First Design:** Every hypothesis must specify a quantitative, measurable falsification condition prior to experiment execution.
2. **Equivalence of Negative Results:** Rejection of a hypothesis (e.g., `NOT SUPPORTED`) is equal in scientific value to confirmation (`SUPPORTED`).
3. **No Ungrounded Claims:** Source code existence or draft RFC status does not constitute proof of performance or correctness.
4. **Matched Baselines:** All comparative evaluations must compare against fully optimized, semantically identical standard binary or protocol equivalents.
5. **Bit-Exact Reproducibility:** All measurements must be generated via scripted, single-command harnesses producing machine-readable JSON logs and cryptographic evidence bundles.

---

### 2.4 Research Ethics & Transparency

- **No Personhood Assumption:** AI collaborators are tools, not legal entities or co-authors under standard academic guidelines (e.g., IEEE/ACM guidelines).
- **Public Provenance:** All prompts, model outputs, raw logs, and intermediate failure artifacts must be archived in the open repository under `/docs/research/` or `/execution_reports/`.
- **Open-Source Licensing:** All software prototypes are licensed under permissive open-source licenses (MIT / Apache-2.0), and documentation under CC-BY-4.0.

---

## Section 3: Deliverable B — Human–AI Collaboration Protocol Specification

*Note: The collaboration protocol defined in this section represents a documented procedural specification and governance guideline. It specifies how multi-role AI interactions should be structured to prevent confirmation bias, rather than describing an existing, fully automated C++ or Python software system.*

### 3.1 Role Architecture & Interactions

To prevent circular confirmation bias ("echo chambers"), AI agents operate according to strict role constraints:

```
[ Human PI: Research Goal ]
           |
           v
[ AI-Arch: Formal Hypothesis & Protocol ]
           |
           v
[ AI-Impl: Prototype & Experiment Script ]
           |
           +-----------------------+
           |                       |
           v                       v
[ AI-Adv: Adversarial Review ]   [ AI-Aud: Sandbox Execution ]
           |                       |
           +-----------------------+
           |
           v
[ AI-Syn: Reconciled Evidence & Uncertainty Report ]
           |
           v
[ Human PI: Final Evaluation & Decision ]
```

- **Research Architect (AI-Arch):** Formulates formal mathematical models, precise hypothesis statements, and experimental protocol specifications.
- **Implementation Engineer (AI-Impl):** Writes software/hardware source code and automated test scripts strictly conforming to AI-Arch specifications.
- **Adversarial Reviewer (AI-Adv):** Scrutinizes code and methodology specifically to find bugs, unmeasured variables, missing baselines, or statistical flaws.
- **Reproducibility Auditor (AI-Aud):** Executes scripts in clean virtual machines/containers, verifying bit-exact outputs and JSON results without relying on unverified claims.
- **Research Synthesizer (AI-Syn):** Aggregates findings from AI-Adv and AI-Aud, highlighting unresolved discrepancies and uncertainty bounds.
- **Human Principal Investigator (Human PI):** Evaluates the synthesis, approves/rejects pull requests, and updates the official research ledger.

---

### 3.2 Protocol Safeguards & Anti-Echo-Chamber Controls

1. **Prompt Isolation:** AI-Adv receives only the specification, code, and raw output logs; it does *not* receive AI-Impl's self-justifications or prompt context.
2. **Cross-Model Diversification:** Code generated by Model A (e.g., GPT-4) must be reviewed by Model B (e.g., Claude 3.5 Sonnet) and verified in an isolated environment by Model C.
3. **Circuit-Breaker Rule:** If AI-Adv identifies an unmeasured variable or benchmark flaw, the pipeline immediately halts and returns to AI-Arch before any pull request can be merged.
4. **Blind Evaluation:** Experimental results are anonymized (e.g., System Alpha vs. System Beta) before performance evaluation by AI-Adv to prevent brand or architectural bias.

---

### 3.3 Disagreement Logging & Provenance Schema

All collaborative steps are recorded in machine-readable JSON format. Specific AI model names and prompt hashes are recorded only when independently documented in git commit logs or execution provenance receipts:

```json
{
  "experiment_id": "EXP-2026-10-C1",
  "hypothesis_id": "H-S1c",
  "human_pi_authorization": "APPROVED",
  "agent_contributions": [
    {"role": "AI-Arch", "model": "documented_model_id_or_generic", "artifact": "docs/research/H08_spec.md"},
    {"role": "AI-Impl", "model": "documented_model_id_or_generic", "artifact": "experiments/integrated_system_advantage/ablation.cpp"},
    {"role": "AI-Adv", "model": "documented_model_id_or_generic", "finding": "Missing C++ substitution seam in Config C", "verdict": "REJECT"},
    {"role": "AI-Aud", "model": "custom-sandbox", "execution_exit_code": 0, "hash_match": true}
  ],
  "reconciled_disposition": "INCONCLUSIVE",
  "human_pi_signoff": true
}
```

---

## Section 4: Deliverable C — Scientific Research Roadmap

### 4.1 Prospective 12-Month Research Roadmap (Q4 2026 – Q3 2027)

```
2026 Q4                     2027 Q1                     2027 Q2                     2027 Q3
+---------------------------+---------------------------+---------------------------+---------------------------+
| Q4 2026 Milestone:        | Q1 2027 Milestone:        | Q2 2027 Milestone:        | Q3 2027 Milestone:        |
| Subsystem Substitution    | Baseline-X Protocol       | Adaptive Radix SIMD       | Academic Peer-Review      |
| Seams & Ablation Suite    | Evaluation (Wasmtime)     | Bounds & MatVec           | Package & External Audit  |
| - Config C, D, E, F seams | - RFC-00D4 verifier CLI   | - AVX2/NEON benchmarking  | - OCI reproducibility     |
| - Resolve H-S1c status    | - Head-to-head comparison | - Falsify/Verify H-INIT-R1| - Journal submission      |
+---------------------------+---------------------------+---------------------------+---------------------------+
```

- **Q4 2026 — Subsystem Ablation & Seam Infrastructure:**
  - *Objective:* Add production C++ substitution seams (`--substitute-vm`, `--substitute-fs`, `--substitute-policy`) to resolve Investigation 08's `INCONCLUSIVE` compositional status.
  - *Stopping Criteria:* If subsystem isolation shows zero positive interaction effects over isolated components, declare monolithic integration non-essential.
- **Q1 2027 — Accountable Substrate vs. Protocol Evaluation (Project Baseline-X):**
  - *Objective:* Execute comparative benchmarks between T81VM+Axion and Wasmtime+Cosign+RFC-00D4.
  - *Stopping Criteria:* If Baseline-X achieves identical policy guarantees with $>5\times$ lower latency, pivot T81 to a pure protocol/specification standard.
- **Q2 2027 — Adaptive Radix & Packed Ternary SIMD Bounds:**
  - *Objective:* Benchmark 2-bit packed ternary SWAR MatVec against INT2/INT4 binary SIMD on AVX2/NEON.
  - *Stopping Criteria:* If packed ternary yields $<1.10\times$ throughput advantage over INT2, freeze SWAR ternary arithmetic development.
- **Q3 2027 — External Reproducibility Package & Peer-Review Submission:**
  - *Objective:* Publish complete open-source reproducibility containers and submit preprints to USENIX Security / IEEE Micro.

---

### 4.2 Canonical Hypothesis Ledger (H01–H08) & Proposed Initiative Hypotheses

#### Master Canonical Hypothesis Register (H01–H08)
*Governed strictly by the [Master Continuity Charter](continuity/MASTER_CONTINUITY_CHARTER_v1.0.md) and [HYPOTHESIS_REGISTER_v1.json](continuity/HYPOTHESIS_REGISTER_v1.json).*

| Hypothesis ID | Track | Canonical Formal Statement | Reconciled Status & Primary Evidence |
| :--- | :--- | :--- | :--- |
| **H01** | Track R | Equal-information ternary storage can have lower total physical cost than binary storage under stated device and overhead assumptions. | Mathematical density established ($\log_2 3 \approx 1.585$). Software tryte addition on binary ALUs is **falsified for speed and frozen (STOP)** ($10.6\times$ slower than native binary integer addition). Physical hardware claims remain open. |
| **H02** | Track R | Balanced ternary can improve a specified signed arithmetic or rounding task at equal semantics and cost accounting. | Algebraic symmetry established. Software SWAR SIMD emulation on binary vector units is **falsified for speed and frozen** due to digit-unpacking tax. |
| **H03** | Track R | Grouping four trits as base-81 can provide a workload-specific interface or execution benefit beyond equivalent binary packing. | 4-trit grouping reduces software tryte addition latency from $105.3\text{ ns}$ to $11.7\text{ ns}$. 2-bit weight MatVec is **validated for storage reduction ($75\%$ DRAM savings)**, but **disproven for CPU MatVec speed** ($75.3\%$ of INT8 AVX2) in L1/L2 cache. |
| **H04** | Track R | An explicitly defined adaptive ternary representation policy can beat fixed ternary and an equally capable adaptive binary policy on a specified workload. | Refined by **H04-R01** trace discriminator. Yields **conditional model support with explicit counterexamples** (e.g., $M=256$, $C_3/C_2=1.5$ yields higher cost for ternary), not physical hardware proof. |
| **H05** | Track R | A specified three-state primitive can reduce circuit or system cost against an equivalent Boolean implementation. | Exploratory. In-VM pre-dispatch policy evaluation (`eval_axion_call`) prevents unauthorized side effects ($0.0\%$ breach rate) in T81VM, but host capability boundaries in Wasm provide equivalent protection. |
| **H06** | Track R | Ternary weights or activations can improve accuracy-constrained inference versus equally optimized binary low-bit alternatives. | On-disk 2-bit weight format (`.t81w`) validated for storage; CPU speedup disproven on binary SIMD. |
| **H07** | Track F | Representation-independent governed execution can improve a defined auditability property. | **Register evidence tiers: B, C.** Separate engineering track. PRs #565/#566 remain open/draft: both CodeQL pass, required quality gate fails. Existing source/test reports are not a new independent security certification here. **Confidence:** Current PR metadata checked; prior security tests not rerun in this session. **Next action:** Maintain the bounded CI/security investigation separately; Protocol 001 remains NOT READY. |
| **H08** | Track M | AI-assisted iteration improves research trustworthiness or discovery under an independently auditable process. | **Register evidence tiers: C, E.** This session records source claims, independent extraction probes and mathematical derivation separately; it is not a controlled study of AI productivity. **Confidence:** Researcher intent authoritative; comparative benefit untested. **Next action:** Append session handoffs and preserve failed claims rather than rewriting the narrative. |

The H07/H08 dispositions above quote the register's **2026-10-25** snapshot;
its PR/CI metadata is historical, not a current readiness check. Tier **B** means
source inspected, not executed integration; **C** means historical/user report or
AI synthesis not independently verified; **E** means proposal/speculation.
Additional source/test reports retain their own tested scope and do not promote
these hypotheses to established findings.

- **H07 limits:** The register requires a conventional runtime with an explicit
  authorization/evidence boundary under equal threat model and host trust.
  Authorization, evidence integrity, factual validity and consent are different;
  simulated dispatch supplies no inference-security or malicious-host claim.
  Existing reports do not prove runtime independence.
- **H08 limits:** Earlier prompt provenance is not available. Commit authorship
  does not prove prompt provenance; repeated model summaries are not independent
  verification. The baseline is human-only or alternative-assisted work under
  matched tasks and independent verification. Artifact identity and session
  continuity do not establish improved research trustworthiness or discovery.

#### Proposed Initiative Hypotheses (H-INIT Namespace)
*Newly proposed research inquiries under the Human–AI Research Initiative (distinct from canonical H01–H08).*

| Initiative ID | Track | Proposed Hypothesis Statement | Target Surface / Falsifier |
| :--- | :--- | :--- | :--- |
| **H-INIT-A1** | Track F | Pre-side-effect policy gating in off-the-shelf Wasm runtimes (Baseline-X) achieves identical security guarantees to bespoke VMs with $\ge 5\times$ lower latency. | Baseline-X exhibits policy bypasses or higher cold-start/steady-state latency than T81VM. |
| **H-INIT-R1** | Track R | Sparse 2-bit packed ternary SWAR vector arithmetic outperforms bit-aligned INT2/INT4 binary SIMD under severe memory bandwidth constraints ($K \ge 4096$). | Throughput advantage over optimized INT2/INT4 SIMD is $< 1.10\times$ on commodity AVX2/NEON hardware. |
| **H-INIT-M1** | Track M | Multi-AI adversarial review pipelines reduce ungrounded architectural overclaims in software research documentation. | Overclaim regression rate $> 0\%$ in automated governance audits. |

*Note: All deterministic claims are bounded by the [Determinism Surface Registry](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md).*

---

## Section 5: Deliverable D — Independent Research Assessment

### 5.1 Scientific Originality Evaluation

T81 combines several established computer science paradigms with novel architectural integrations:

1. **Established Paradigms (Prior Art):**
   - *Balanced Ternary Arithmetic:* Historical precursor Setun (1958), Knuth (TAOCP Vol 2).
   - *Content-Addressed Storage:* Git, IPFS, Plan 9 Venti.
   - *Policy Authorization:* Capability systems, KeyKOS, SELinux, Open Policy Agent (OPA).
2. **Standard Engineering Techniques:**
   - C++20 interpreter dispatch loop, CMake build configuration, Icarus Verilog testbenches, Python benchmark harnesses.
3. **Genuine Originality & Innovation:**
   - **Unified Accountable Substrate Primitive:** Unifying content identity (CanonFS), pre-side-effect authorization (Axion), and detached cryptographic proof bundles (RFC-00D4) into a single, cohesive execution model.
   - **Multi-AI Falsification Methodology:** A formalized multi-agent research framework engineered specifically to combat AI hallucination and confirmation bias in experimental computer science.

---

### 5.2 Qualitative Implementation & Verification State

*In accordance with T81 research governance, subjective numeric percentage scores are replaced by explicit, reproducible implementation and verification states anchored to CTest targets and codebase artifacts.*

- **TISC ISA / Core Encoding:** `FROZEN SPEC; REGISTRY INTEGRITY CHECK`. Run `python3 scripts/ci/check_tisc_freeze_integrity.py` to check the registry's 209 unique opcode values, reserved-range boundary and bitwise assignments. This checks registry integrity, not executable opcode conformance or toolchain behavior. Executable semantics require separately built and run tests, such as `tisc_opcode_family_semantics_test`; availability of a test is not a passing result.
- **Axion Kernel / Policy Engine:** `IMPLEMENTED & UNIT TESTED`. Functional pre-side-effect policy interception (`eval_axion_call`); verified by `t81_vm_predispatch_policy_deny_logging_test` and `canonfs_policy_boundary_test`.
- **CanonFS / Content Store:** `IMPLEMENTED & INTEGRATION TESTED`. SHA-3/256 content-addressed storage and foreign filesystem interchange (RFC-00D1); verified by `canonfs_identity_contract_test` and CTest #84–85.
- **T81VM Execution Engine:** `IMPLEMENTED WITH COLD-START BOTTLENECK`. Cross-platform deterministic execution profile verified (`vm_determinism_property_test`); CLI cold-start latency ($\sim 5.3\text{ s}$) identified as primary performance tax.
- **Subsystem Substitution Seams:** `UNIMPLEMENTED IN PRODUCTION C++`. Subsystem substitution flags (`--substitute-vm`, `--substitute-fs`, `--substitute-policy`) for Investigation 08 ablation configs C, D, E, F require C++ driver refactoring.
- **Native Ternary Hardware Target:** `EMULATED / UNBUILT PHYSICAL SILICON`. Emulated via Verilog/Yosys logic models (`experiments/ternary_hardware_feasibility/`); H-T1 sign-and-mask hardware overlay rejected ($1.098\times$ gate reduction vs. $1.25\times$ required); native physical ASIC unbuilt.

---

### 5.3 Synthesis of Existing Research Evidence

#### Investigation 07: Ternary Dot-Product Hardware Feasibility
- **Finding:** Design C (H-T1 Ternary Overlay) yielded a $1.098\times$ gate area reduction over Design B (Optimized Binary Control) at $N=64$ (4,948 gates vs. 5,924 gates).
- **Disposition:** `NOT SUPPORTED` (bounded strictly to the H-T1 sign-and-mask overlay design). It failed the pre-registered $1.25\times$ area reduction threshold.

#### Investigation 08: Integrated Architecture Advantage
- **H-S1a (Computational Advantage):** `NOT SUPPORTED`. T81 cold-start CLI execution latency ($\sim 5,362 \text{ ms}$) is dominated by binary load overhead compared to in-process baselines ($\sim 0.65 \text{ ms}$).
- **H-S1b (Assurance Advantage):** `SUPPORTED`. Proved pre-execution policy enforcement and SHA-3/256 content-addressed state verification before state transitions.
- **H-S1c (Compositional Advantage):** `INCONCLUSIVE`. Individual subsystem substitution seams (Configs C, D, E, F) do not exist in the production binary without code modification.
- **H-S1d (Ternary-Specific Advantage):** `NOT SUPPORTED / UNIMPLEMENTED`. Unimplemented due to lack of C++ VM arithmetic substitution seams.

#### Investigation 09-E1: TISC Executable Size vs. Wasm Baseline
- **Finding:** Compiler-generated TISC executable size was $1,240$ bytes versus $157$ bytes for an optimized WebAssembly comparator.
- **Ratio:** $7.898\times$ size penalty against a required maximum threshold ratio of $0.95\times$.

---

### 5.4 Categorization of Evidence Types

- **Mathematical Derivations & Model Calculations:** Exact rational conversion properties and finite-capacity storage thresholds (`docs/research/EXACT_RATIONAL_CONVERSION_FEASIBILITY.md` and `docs/research/continuity/H04_RADIX_ADAPTATION_DISCRIMINATOR.md`).
- **Executable Demonstrations:** End-to-end CLI policy interception tests (`tests/cpp/axion_policy_allow_deny_determinism_test.cpp`).
- **Simulation Results:** Icarus Verilog and Yosys synthesis gate count models (`experiments/ternary_hardware_feasibility/`).
- **Physical Measurements:** Measured execution latencies and executable binary byte sizes (`experiments/integrated_system_advantage/results.json`).
- **Speculative Architectural Models:** Unbuilt native ternary silicon hardware claims.

---

## Section 6: Deliverable E — Publication and External Collaboration Strategy

### 6.1 Peer-Reviewed Publication Route

1. **Target Venue 1: USENIX Security / OSDI (Systems & Security Track)**
   - *Paper Title:* "Pre-Side-Effect Policy Authorization and Detached Evidence Protocols for Accountable Runtimes."
   - *Core Focus:* Axion policy engine, CanonFS content identity, and RFC-00D4 evidence bundles.
2. **Target Venue 2: IEEE Micro / ASPLOS (Architecture & Systems Track)**
   - *Paper Title:* "Empirical Limits of Balanced Ternary Overlays and Packed SWAR Vector Arithmetic."
   - *Core Focus:* Quantitative findings from Investigations 07 & 08, presenting rigorous negative hardware results and SWAR bounds.

---

### 6.2 Reproducibility Package Standards

Every paper submission will be accompanied by an open-access **Artifact Evaluation Package**:
- **Docker/OCI Container:** Fully self-contained image containing GCC 13, Yosys, Icarus Verilog, CMake, and Python dependencies.
- **One-Command Execution:** `./scripts/reproduce_all_experiments.sh` executing the complete benchmark and audit suite.
- **Raw Data & Verification Logs:** Machine-readable JSON outputs with SHA-256 signatures.

---

### 6.3 External Academic & Community Engagement

- **Prerequisites for Outreach:**
  1. Completion of production subsystem substitution seams.
  2. Baseline-X empirical comparative evaluation.
  3. Clean CI governance audit with zero active overclaim flags.
- **Engagement Playbook:**
  - Share preprints on arXiv / Cryptology ePrint Archive.
  - Invite independent academic security labs to conduct adversarial audit reviews of RFC-00D4 evidence bundles.

---

## Section 7: Deliverable F — Immediate Action Plan (Next 30 Days)

### 7.1 Top 5 Action Items

```
+----------------------------------------------------------------------------------------------------------------------+
| Action Item                                 | Target Deliverable              | Lead Role | Success Criterion                    |
+---------------------------------------------+---------------------------------+-----------+--------------------------------------+
| 1. Implement C++ Substitution Seams         | `src/vm/subsystem_seams.cpp`    | AI-Impl   | Configs C, D, E, F runnable via CLI  |
| 2. Build Wasmtime Baseline-X Benchmark      | `experiments/baseline_x/`       | AI-Arch   | Parity & latency benchmark report    |
| 3. Develop RFC-00D4 Verifier CLI Tool       | `tools/rfc00d4_verifier`        | AI-Impl   | Standalone evidence bundle verifier  |
| 4. Deploy Multi-AI Adversarial CI Pipeline  | `scripts/ci/multi_ai_review.py` | AI-Aud    | Automated cross-model review check   |
| 5. Publish OCI Reproducibility Container    | `Dockerfile.reproducibility`    | AI-Aud    | Single-command bit-exact execution   |
+----------------------------------------------------------------------------------------------------------------------+
```

#### Detailed Action Descriptions:

1. **Action 1: Implement C++ Production Subsystem Substitution Seams**
   - *Goal:* Refactor `t81` CLI driver to accept flags `--substitute-vm`, `--substitute-fs`, and `--substitute-policy`.
   - *Success Criteria:* Configs C, D, E, and F in Investigation 08 execute successfully and log valid latency metrics, resolving the `INCONCLUSIVE` status of H-S1c.
2. **Action 2: Build Wasmtime Baseline-X Comparative Suite**
   - *Goal:* Create a matched Wasmtime + SQLite + Cosign comparator for inventory replenishment under constraints.
   - *Success Criteria:* Machine-readable JSON comparison of cold-start latency, steady-state execution, and evidence generation overhead.
3. **Action 3: Develop Standalone RFC-00D4 Verifier CLI**
   - *Goal:* Build a lightweight, zero-dependency C++ CLI tool (`t81-evidence-verify`) to validate RFC-00D4 evidence bundles without running the full T81 VM stack.
   - *Success Criteria:* Verification of evidence bundles produced by both T81VM and Wasmtime baselines.
4. **Action 4: Design Multi-AI Adversarial Review Procedural Harness**
   - *Goal:* Specify procedural guidelines for multi-role cross-review across independent AI agents for new RFCs and research documentation.
   - *Success Criteria:* Written review protocol verified against PR submission workflows.
5. **Action 5: Package OCI Reproducibility Container**
   - *Goal:* Create `Dockerfile.reproducibility` pin-specifying all compilers, Yosys, Icarus Verilog, and CMake versions.
   - *Success Criteria:* Clean execution of `run_verification.py` and `run_experiments.py` inside container with 100% test pass rate.

---

## Section 8: Summary Governance Sign-Off & Administrative Intake Record

- **Human Principal Investigator Approval Status:** `DOCUMENTATION INTAKE RECORDED (PR #607 MERGE)`
  - *Note:* Merge of PR #607 constitutes administrative intake and charter registration into the repository documentation structure. It does not imply prior human PI approval or scientific certification of unverified or speculative claims. Specific scientific assertions remain subject to empirical falsification and explicit PI sign-off.
- **Lead AI Research Assistant Role:** `AI-Architect`
- **Determinism Boundary Verification:** Confirmed bounded by the [Determinism Surface Registry](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md).
- **Root Structure Integrity:** Compliant with `scripts/governance/check_root_structure.py`.
