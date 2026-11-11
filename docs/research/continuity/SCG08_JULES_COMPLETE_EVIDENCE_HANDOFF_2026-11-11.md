# SCG-08 — Jules Evidence Handoff and Reconciliation Assignment

**Purpose:** Supply the complete source reports that were uploaded to a separate ChatGPT conversation and are not otherwise visible to Jules. This handoff includes three unedited report bodies below. Treat them as historical investigator outputs, not automatically as authoritative specifications.

**Repository:** https://github.com/t81dev/t81-foundation

## Task for Jules

1. Verify each appendix's assertions against the actual repository, charter, commits, PRs and source code. Record verified, contradicted, and not-verifiable findings separately.
2. Preserve the original reports verbatim as evidence. Do not silently revise their historical text.
3. Reconcile the newer provenance finding that `T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06` has **no independently verified frozen text** and that historical dated Gate 1 implementation authorization was **not verified**. In particular, Appendix B calls `GATE1_IMPLEMENTATION_REPORT.md` an authoritative specification; do not propagate that authority claim without independent evidence.
4. Trace every purported preregistered threshold, budget, operand domain, accounting rule and stopping criterion to an actual frozen source, or label it implementation-derived, retrospective, proposed, or unsupported.
5. Independently rederive the complete three-state carry transition model, finite-sequence boundary effects and the cluster-robust ratio/delta variance estimator. The transient and variance formulas in Appendix C require verification; do not assume they are correct.
6. Distinguish the 179 Gate 1 bounded smoke trials, implementation accounting checks, and schema validation from independent scientific acceptance. Do not treat successful merges as governance authorization.
7. Reconcile the 10,000,000 contiguous digit-step charter design with the 136,500 clustered sequences / 48,128,000 digit-step proposal. They are different experimental designs; do not declare them equivalent.
8. Prepare a **prospective**, documentation-only Phase 1 specification and maintainer decision package, clearly identifying historical gaps. Do not backdate or fabricate the missing Addendum 06 or historical approvals.
9. **Gate 2 remains NO-GO** until a separately documented, dated approval and all technical/statistical prerequisites are met.

**Boundaries:** No new experimental execution or benchmarks, no production source edits, no retroactive normative amendments, no automatic merge. Report the exact base/head SHAs, changed files, test/validation results, unresolved findings and PR URL if one is opened. Ask for approval before normative changes.

## Source reports (unaltered)


---

## Appendix A — Provenance Recovery Report

SCG-08 Contract and Authorization Provenance Recovery Report

Investigation Date: 2026-11-11
Investigator: Jules
Target Program: SCG-08 (Balanced-Ternary Carry Propagation and Arithmetic Work)
Investigation Mode: Read-Only Audit (Zero repository modifications, zero experimental executions, zero automatic merges, zero retroactive amendments)

1. Executive Summary & Core Dispositions

Governance Question	Authoritative Finding / Disposition
Authoritative Frozen Specification Text (T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06)	NOT VERIFIED — The physical, frozen contract text does not exist as an independent specification document in Git history, historical branches, or merged spec directories.
Historical Dated Gate 1 Implementation Authorization	NOT VERIFIED — No maintainer directive, approval record, or recorded decision granting Gate 1 implementation authorization exists. Parent Charter explicitly records Gate 1 as NOT GRANTED.
Existing Gate 1 Implementation Code (PR #760, #763, #765)	IMPLEMENTATION DESCRIPTION ONLY — Subject to separate independent technical and evidentiary acceptance; merging code does not establish specification authority or maintainer approval.
Gate 2 Experimental Execution Status	STRICTLY WITHHELD — No experimental campaign, benchmarking, or execution authorized or performed.
2. Repository Baseline & Search Scope

Repository: https://github.com/t81dev/t81-foundation
Reference Mainline Baseline Commit: 36b3849aa1e6297e88de2ec1655777793ab0b59e (Merge PR #765)
Search Scope Audited:
Full Git commit history (git log --all, full unshallowed repository object database).
All local and remote branches (origin/*), tags, and reflogs.
Pull Requests #760, #763, and #765, including PR metadata, description bodies, automated Codex review comments, and inline discussion threads.
Related SCG-08 PRs: #745, #753, and #761.
Historical research continuity documentation (docs/research/continuity/, docs/ai-context/, and docs/architecture/visualization/).
Pre-commit and validator integrity execution (python3 scripts/ci/validate_scg08_gate1_evidence.py && python3 scripts/ci/validate_scg08_gate1_evidence.py --test-negative).
3. Evidence Inventory & Candidate Source Audit

Every candidate source referencing T81-SCG-08-PHASE1-PROP-001, ADDENDUM-06, or Gate 1 authorization was inspected and categorized:

Source Reference	Commit SHA	Date	Document Identity / Path	Category	Establishes Authorization?
Parent Charter (PR #753)	0bd0817e4b (head 09c666e36a)	2026-11-10 17:02:14Z	T81-SCG-08-PHASE0-CHARTER-001-R2 / docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md	Frozen Specification Text (Phase 0)	NO — Explicitly specifies Gate 1: NOT GRANTED, Gate 2: NOT GRANTED.
PR #760 Initial Gate 1 Harness	dab59a63e4 / 525554a193	2026-11-10 20:07:36Z	T81-SCG-08-GATE1-REPORT-001 / experiments/scg08_arithmetic_work/	Implementation Description	NO — Open, unmerged PR citing Addendum 06 as an external spec reference string; contains no authorization grant.
PR #761 Independent Acceptance Review R2	5c69f11579	2026-11-10 21:06:23Z	SCG08_PHASE0_INDEPENDENT_ACCEPTANCE_REPORT.md	Retrospective Reporting	NO — Open draft PR. Retrospectively asserts Phase 1 was authorized under separate directives, but cites no artifact SHA or maintainer record.
PR #763 Gate 1 Evidence Repair	dbf107fda8 (head 93a3d1bc36, commit afa0714c4e)	2026-11-10 22:18:11Z	PR #763 Merge / experiments/scg08_arithmetic_work/	Implementation Description	NO — Merged corrective PR resolving table lookups and conditions; merging code does not infer governance authorization.
PR #765 Codex Findings Resolution	36b3849aa1 (head 2dd3cc62e5)	2026-11-11 00:56:56Z	PR #765 Merge / experiments/scg08_arithmetic_work/	Implementation Description	NO — Merged corrective PR resolving Codex findings; PR body explicitly requires: READY FOR INDEPENDENT REVIEW — ACCEPTANCE WITHHELD.
Continuity Follow-up R1	1a0873de42	2026-11-10 19:32:55 EST	docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R1.md	Retrospective Reporting	NO — Explicitly documents: Gate 1 authorization provenance: MISSING EVIDENCE; Addendum 06 / original authoritative decision not located.
Continuity Follow-up R2	cd93840338	2026-11-10 20:07:41 EST	docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R2.md	Retrospective Reporting	NO — Explicitly documents: Addendum 06 / original authoritative decision not located. No approval inferred from merge.
4. Categorical Distinctions

To prevent governance confusion, the four evidential classes in the SCG-08 record are delineated:

Frozen Specification Text:
The only frozen specification document present in the repository for SCG-08 is SCG08_PHASE0_RESEARCH_CHARTER.md (T81-SCG-08-PHASE0-CHARTER-001-R2, merged at 0bd0817e4b). It defines the 3-state carry Markov chain, the three-tier experimental architecture, and non-negotiable two-gate sequential authorization.
T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06 does not exist as frozen specification text anywhere in the repository.
Implementation Description:
experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md, tier1_markov_harness.hpp, tier2_work_harness.hpp, test_exp_scg08_01.cpp, gate1_evidence_record.json, and evidence_schema.json.
These files implement arithmetic algorithms and report local trial execution receipts (179 trials). They describe how the harness was constructed, but cannot serve as their own authorization or normative specification.
Maintainer Authorization:
Formal, dated approval by a repository maintainer granting permission to transition between gates.
Finding: Zero maintainer authorizations exist for Gate 1 or Gate 2. PR merges were mechanical bot integrations or task-driven commits without an explicit maintainer approval record.
Retrospective Reporting:
Reports in docs/ai-context/ (T81_RESEARCH_STATUS_2026-11-10_R1.md, R2.md) and docs/research/continuity/ (SCG08_PHASE0_INDEPENDENT_ACCEPTANCE_REPORT.md).
These documents record observations after the fact. While the Phase 0 report claims proposals R1–R3 and Addenda 01–05 were authorized under separate directives, it supplies no receipt, commit SHA, or review link. The AI continuity status reports specifically confirm that the authoritative record could not be found.
5. Specification Gap Analysis

Because T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06 is missing, the following normative requirements cited by the Gate 1 harness cannot be authenticated against a frozen source:

Accumulator-Width Sizing & Limb Allocation:
The table sizing operand trits ($T_{\text{signed}} \in {41, 81, 162, 324, 647}$) and allocation of ternary limbs (e.g., Band 128: 3 Op / 4 Add / 6 Mul) vs. binary limbs (2 Op / 3 Add / 4 Mul) are codified only inside C++ headers and the implementation report. Whether this was the approved normative standard or an author choice cannot be verified.
Weighted Cost Metric ($W_{\text{ops}}$) Composition:
The formula $W_{\text{ops}} = \text{OP}{\text{ALU_ADD_SUB}} + 3 \cdot \text{OP}{\text{ALU_MUL}} + \text{OP}{\text{TABLE_LOOKUP}} + \text{OP}{\text{BRANCH_COND}} + \text{OP}_{\text{NORM_PASS}}$ and the decision to treat table lookups as unit weight and multiplication as weight 3 cannot be traced to an approved addendum.
Detailed Primitive Accounting Invariants:
The breakdown of 38 table lookups per T81Limb27::addc() (9 ADD_TABLE, 20 COMPOSITION_TABLE, 9 CARRY_FROM_ZERO) and symmetric condition accounting rules are nowhere specified outside PR review responses.
Execution Ceiling Budget:
The hard ceiling of $\le 1,000$ cumulative trials enforced by TrialBudgetEnforcer is not grounded in an authoritative document outside the C++ implementation.
6. Minimum Prospective Governance Remediation

To resolve the provenance gap without rewriting history or fabricating retroactive approvals:

Do Not Reconstruct or Backdate Historical Approval:
Historical records must remain immutable. Do not forge an approval commit or retroactively modify SCG08_PHASE0_RESEARCH_CHARTER.md.
Draft and Publish Prospective Phase 1 Specification (RFC-00E1 or T81-SCG-08-SPEC-001):
Author a standalone, reviewable specification document under docs/research/continuity/ (or spec/rfcs/) codifying:
The signed $B$-bit operand domain $[-2^{B-1}, 2^{B-1}-1]$ across $B \in {64, 128, 256, 512, 1024}$.
The exact accumulator and limb allocation table.
The normative definition of $W_{\text{ops}}$ and primitive operation accounting rules (including the 38-lookup limb composition model).
The execution trial budget constraints ($\le 1,000$ trials for validation).
Formal Maintainer Decision Record:
Propose the prospective specification via a dedicated pull request.
Require an explicit, dated review and signed approval by a project maintainer (e.g., recorded in PROJECT_CONTROL_CENTER.md or a formal maintainer acceptance comment).
Prospective Gate 1 Acceptance:
After specification approval, perform an independent technical code review of the merged Gate 1 harness against the newly ratified specification.
If technical parity, oracle validation, and accounting are validated, grant Gate 1 Acceptance prospectively.
Gate 2 Pre-requisite:
Maintain Gate 2 experimental execution as STRICTLY WITHHELD until Gate 1 acceptance and statistical protocol reconciliation (136,500 sequences vs. $10^7$ steps) are formally approved.
7. Integrity & Execution Verification

Repository Cleanliness: Verified via git status and git diff that zero working tree changes or untracked files were introduced.
Validator Integrity: Executed existing schema and negative test suite:
Command: python3 scripts/ci/validate_scg08_gate1_evidence.py && python3 scripts/ci/validate_scg08_gate1_evidence.py --test-negative
Result: Positive schema validation passed; all 12 negative fail-closed test cases passed.
Pre-commit / Review Safeguards: Executed pre-commit review probe, confirmed no pending changes, recorded governance memory. No experimental benchmarking or execution took place.
Delivered the final SCG-08 Contract and Authorization Provenance Report with complete evidence inventory, distinctions, gap analysis, and prospective remediation actions.

---

## Appendix B — Gate 1 Accounting Reconciliation

SCG-08 Gate 1 Arithmetic Accounting and Execution Provenance Reconciliation Report

Authoritative Baseline: 36b3849aa1e6297e88de2ec1655777793ab0b59e (origin/main)
Evaluation Scope: SCG-08 Gate 1 Arithmetic Work Harness (experiments/scg08_arithmetic_work/)
Authoritative Specifications:

Parent Research Charter: docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md (T81-SCG-08-PHASE0-CHARTER-001-R2, PR #753 at commit 0bd0817e4b9dc879fffce18c59ea9ff781f7acb3)
Gate 1 Implementation Specification: experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md (T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06)
Evidence Schema: experiments/scg08_arithmetic_work/evidence_schema.json (t81.scg08.carry-experiment.v1)
Validation Script: scripts/ci/validate_scg08_gate1_evidence.py
1. Specification Audit & Conflicting Definitions

1.1 Specification Lineage & Scope

Charter Scope (SCG08_PHASE0_RESEARCH_CHARTER.md): Formally preregistered experiment EXP-SCG08-01 to test Hypothesis H02-A (balanced-ternary carry-chain dynamics) under a 3-tier decoupled model (Tier 1: mathematical scalar carry Markov chain; Tier 2: algorithmic multi-precision operation work; Tier 3: exploratory host CPU timing). Enforces a 2-gate sequential authorization model: Gate 1 (harness implementation) and Gate 2 (experimental campaign execution).
Harness Scope (GATE1_IMPLEMENTATION_REPORT.md / test_exp_scg08_01.cpp): Implements isolated Gate 1 harness under Addendum 06. Caps cumulative trials to $\le 1,000$ via TrialBudgetEnforcer and strictly withholds Gate 2 benchmarking.
1.2 Reconciliation of Divergent / Follow-Up Definitions

During the reconciliation audit across branches (main vs origin/fix/scg08-gate1-provenance-and-scalar-validation-36b3849-5951848895340305516), two specific definition discrepancies were identified:

Git Provenance Hash Definition (git_context):
Committed on main (commit 36b3849): git_context in evidence_schema.json and gate1_evidence_record.json specified baseline_sha: "dbf107fda83a3755fa21d6b95caa3eb8ac450dd3" and head_sha: "dbf107fda83a3755fa21d6b95caa3eb8ac450dd3".
Follow-up Branch (a763ab6): Replaces head_sha with evaluated_implementation_sha: "2dd3cc62e526942cdcbed6d100770c08c26ab2fe", documenting that commit 2dd3cc62 contains the evaluated harness code while commit dbf107fd was the base branch of PR #765, resolving circular self-referencing.
Reconciliation: Both definitions are preserved without silently discarding historical records. The evidence record on main is bound to the exact merge point 36b3849 whose parents are dbf107fd and 2dd3cc62.
Scalar Validation Rigor:
On main, scripts/ci/validate_scg08_gate1_evidence.py enforces integer checks and weighted formula consistency. The pending branch a763ab6 adds strict scalar type rejection for boolean/null/float properties. Both implementations pass schema validation cleanly against the canonical gate1_evidence_record.json.
2. Source-Level Counter Audit

The frozen weighted cost metric defined by the contract is: $$W_{\text{ops}} = \text{OP}_{\text{ALU\_ADD\_SUB}} + 3 \cdot \text{OP}_{\text{ALU\_MUL}} + \text{OP}_{\text{TABLE\_LOOKUP}} + \text{OP}_{\text{BRANCH\_COND}} + \text{OP}_{\text{NORM\_PASS}}$$

2.1 Audit of Ternary Arithmetic Operations (tier2_work_harness.hpp)

add_ternary<InLimbs, OutLimbs>():
op_alu_add_sub: Incremented by 1 per limb pair in the primary loop (InLimbs times) and by 1 per carry limb addition (carry_in != 0).
op_table_lookup: Incremented by 38 lookups per T81Limb27::addc() call. Tracing T81Limb27::addc() in include/t81/types/T81Limb.hpp:
9 in detail::ADD_TABLE (1 per tryte)
$8 + 7 + 5 = 20$ in detail::COMPOSITION_TABLE
$8 + 1 = 9$ in detail::CARRY_FROM_ZERO
Exact sum: $9 + 20 + 9 = 38$ lookups.
op_branch_cond: Incremented on each iteration checking carry_in != 0.
mul_ternary<InLimbs, OutLimbs>():
op_alu_mul and op_alu_add_sub: Incremented once per active trit convolution product (accum[i+j] += ai * bj).
op_branch_cond: Incremented for outer ai == 0, inner bj == 0, and boundary check i + j < ACCUM_TRITS.
op_norm_pass: Incremented once per multiplication pass.
Normalization internal primitives (norm_alu_add_sub, norm_branch_cond) are tracked separately and verified not to aggregate into $W_{\text{ops}}$.
2.2 Audit of Binary Arithmetic Operations (tier2_work_harness.hpp)

mul64() Primitive Multiplier:
Performs 64x64-bit unsigned multiply via 32-bit halves: 4 32x32-bit muls (u0*v0, u1*v0, u0*v1, u1*v1), each incrementing op_alu_mul.
Performs 3 intermediate 64-bit additions, incrementing op_alu_add_sub by 3.
mul_binary<InLimbs, OutLimbs>():
For each limb product $(i, j)$: calls mul64() (+4 op_alu_mul, +3 op_alu_add_sub), then executes 4 primitive accumulator additions (sum1 = cur + lo, sum2 = sum1 + carry, carry = hi + c1, carry += c2), incrementing op_alu_add_sub by 4 (total 7 adds per inner step).
Tracks all conditional branches: outer ai == 0, inner i + j >= OutLimbs, carry predicates (sum1 < cur, sum2 < sum1), carry propagation checks (carry == 0, sum < cur), zero detection (res.limbs[i] != 0), and final sign normalization (all_zero).
add_binary<InLimbs, OutLimbs>():
Audits same-sign addition (InLimbs adds, carry predicates) and opposite-sign subtraction via cmp_binary_limbs() and limb borrow propagation.
3. Independent Analytical Derivation vs. Observed Counts

All counts independently derived below are verified against test_deterministic_exact_counts() in experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp:

3.1 Zero Operands ($0 + 0, 0 \times 0$ on Band $B=64$)

Ternary Zero Add (InLimbs=2, OutLimbs=2):
Loops: 2 iterations. Neither has carry-in (carry_in == 0).
op_alu_add_sub: $2 \times 1 = 2$.
op_table_lookup: $2 \times 38 = 76$.
op_branch_cond: $2$ (carry_in != 0 checks).
Derived $W_{\text{ops}} = 2 + 3(0) + 76 + 2 + 0 = \mathbf{80}$. (Observed: 80, Match).
Binary Zero Add (InLimbs=1, OutLimbs=2):
Outer branch: a.is_negative == b.is_negative (1 branch).
Inner loop: 1 limb addition + 1 accumulator zeroing loop ($1 + 1 = 2$ ALU adds, 1 carry check).
Derived $W_{\text{ops}} = 2 + 3(0) + 0 + 2 + 0 = \mathbf{4}$. (Observed: 4, Match).
Ternary Zero Mul (InLimbs=2, OutLimbs=3):
Trit length: 54 trits. All trits 0. Outer loop tests ai == 0 for all 54 trits $\to 54$ branches, 0 inner multiplies.
Derived $W_{\text{ops}} = 0 + 3(0) + 0 + 54 + 1 = \mathbf{55}$. (Observed: 55, Match).
Binary Zero Mul (InLimbs=1, OutLimbs=2):
Limb 0 is 0: ai == 0 (1 branch, breaks).
Zero-detection loop: 2 limb checks res.limbs[i] != 0 (2 branches).
Sign normalization: if (all_zero) (1 branch).
Derived $W_{\text{ops}} = 0 + 3(0) + 0 + 4 + 0 = \mathbf{4}$. (Observed: 4, Match).
3.2 Sparse Operands ($1 + 0, 1 \times 1$ on Band $B=64$)

Ternary Sparse Add: Derived $W_{\text{ops}} = \mathbf{80}$. (Observed: 80, Match).
Binary Sparse Add: Derived $W_{\text{ops}} = \mathbf{4}$. (Observed: 4, Match).
Ternary Sparse Mul ($1 \times 1$):
Exactly 1 non-zero trit in $a$ (index 0) and 1 non-zero trit in $b$ (index 0).
Multiplications: 1 ($1 \times 1$). Additions: 1.
Branches: 54 outer checks (1 false, 53 true) + 54 inner checks (1 false, 53 true) + 1 range check = 109 branches.
Derived $W_{\text{ops}} = 1 + 3(1) + 0 + 109 + 1 = \mathbf{114}$. (Observed: 114, Match).
Binary Sparse Mul ($1 \times 1$):
1 outer limb non-zero ($ai \neq 0$). 1 inner limb non-zero ($bj \neq 0$).
mul64(): 4 muls, 3 adds. Caller: 4 adds. Carry loop: 1 add. Total ALU adds = 8.
Branches: 1 outer ai==0 + 1 inner i+j>=OutLimbs + 2 carry predicates + 1 carry loop check + 1 zero detect + 1 all_zero = 7 branches.
Derived $W_{\text{ops}} = 8 + 3(4) + 0 + 7 + 0 = \mathbf{27}$. (Observed: 27, Match).
3.3 Dense Operands ($V_{\text{max}} + V_{\text{max}}, V_{\text{max}} \times V_{\text{max}}$ on Band $B=64$)

Ternary Dense Add: Derived $W_{\text{ops}} = \mathbf{80}$. (Observed: 80, Match).
Binary Dense Add: Derived $W_{\text{ops}} = \mathbf{4}$. (Observed: 4, Match).
Ternary Dense Mul ($V_{\text{max}} = 2^{63}-1$, 41 non-zero trits):
Convolutions: $41 \times 41 = 841$ inner products.
op_alu_mul = 841, op_alu_add_sub = 841.
Branches: 54 outer checks + $41 \times (54 \text{ inner checks} + 4 \text{ range checks}) = 2461$ branches.
Derived $W_{\text{ops}} = 841 + 3(841) + 0 + 2461 + 1 = \mathbf{5826}$. (Observed: 5826, Match).
Binary Dense Mul ($V_{\text{max}} = 2^{63}-1$):
mul64(): 4 muls, 3 adds. Caller: 4 adds. Carry loop: 2 adds. Total ALU adds = 9.
Branches: 8 branches.
Derived $W_{\text{ops}} = 9 + 3(4) + 0 + 8 + 0 = \mathbf{29}$. (Observed: 29, Match).
3.4 Carry-Propagating Operands

Ternary Carry-Propagation ($[1^{27}, 0^{27}] + 1$):
Limb 0 is filled with 27 positive trits ($1^{27} = (3^{27}-1)/2$). Adding 1 produces a carry-out $c_1 = +1$.
Limb 1 executes a second nested addc(c_limb) to absorb the carry, triggering a 3rd addc() call.
Total addc() calls: 3. Lookups: $3 \times 38 = 114$. ALU adds: 3. Branches: 2.
Derived $W_{\text{ops}} = 3 + 3(0) + 114 + 2 + 0 = \mathbf{119}$. (Observed: 119, Match).
Binary Carry-Propagation ($[0\text{xFFFF...FFFF}, 0] + 1$):
Limb 0 addition overflows $2^{64}-1 + 1 \implies$ carry 1 propagates into limb 1.
ALU adds: 3. Branches: 3.
Derived $W_{\text{ops}} = 3 + 3(0) + 0 + 3 + 0 = \mathbf{6}$. (Observed: 6, Match).
4. Accounting Reconciliation Matrix

Reconciliation of all 5 evaluated capacity bands recorded in gate1_evidence_record.json:

Capacity Band ($B$ bits)	Operation	op_alu_add_sub	op_alu_mul	op_table_lookup	op_branch_cond	op_norm_pass	Derived $W_{\text{ops}}$	Emitted $W_{\text{ops}}$	Reconciled Status
64	Add Ternary	2	0	76	2	0	80	80	EXACT MATCH
64	Mul Ternary	841	841	0	2,461	1	5,826	5,826	EXACT MATCH
64	Add Binary	2	0	0	2	0	4	4	EXACT MATCH
64	Mul Binary	9	4	0	8	0	29	29	EXACT MATCH
128	Add Ternary	5	0	152	4	0	161	161	EXACT MATCH
128	Mul Ternary	2,601	2,601	0	6,813	1	17,218	17,218	EXACT MATCH
128	Add Binary	3	0	0	3	0	6	6	EXACT MATCH
128	Mul Binary	34	16	0	21	0	103	103	EXACT MATCH
256	Add Ternary	10	0	342	7	0	359	359	EXACT MATCH
256	Mul Ternary	12,321	12,321	0	30,465	1	79,750	79,750	EXACT MATCH
256	Add Binary	5	0	0	5	0	10	10	EXACT MATCH
256	Mul Binary	132	64	0	65	0	389	389	EXACT MATCH
512	Add Ternary	18	0	684	12	0	714	714	EXACT MATCH
512	Mul Ternary	45,369	45,369	0	114,705	1	296,182	296,182	EXACT MATCH
512	Add Binary	9	0	0	9	0	18	18	EXACT MATCH
512	Mul Binary	520	256	0	225	0	1,513	1,513	EXACT MATCH
1024	Add Ternary	34	0	1,292	24	0	1,350	1,350	EXACT MATCH
1024	Mul Ternary	185,761	185,761	0	465,697	1	1,208,742	1,208,742	EXACT MATCH
1024	Add Binary	17	0	0	17	0	34	34	EXACT MATCH
1024	Mul Binary	2,064	1,024	0	833	0	5,969	5,969	EXACT MATCH
5. Discrepancy Inventory

Auditing by source file, function, and operation identified zero implementation arithmetic defects:

Source File	Function / Scope	Operation	Expected Count	Observed Count	Discrepancy Status
tier2_work_harness.hpp	ArithmeticWorkHarness::add_ternary	Table lookups per addc()	38	38	Reconciled (PR #763 / PR #765)
tier2_work_harness.hpp	mul64	32x32 primitive multiplies	4	4	Reconciled (PR #765)
tier2_work_harness.hpp	mul64 + caller	64-bit primitive additions	$3 + 4 = 7$	7	Reconciled (PR #765)
tier2_work_harness.hpp	ArithmeticWorkHarness::mul_binary	Condition evaluations	Tracked per limb	Tracked per limb	Reconciled (PR #765)
gate1_evidence_record.json	git_context	Commit hash provenance	Non-circular binding	Bound to merge parent	Documented (PR #765 / PR #766 follow-up)
6. Deterministic Reproduction Procedure

Exact Source Baseline SHA: 36b3849aa1e6297e88de2ec1655777793ab0b59e (origin/main)
Evaluated Implementation SHA: 2dd3cc62e526942cdcbed6d100770c08c26ab2fe
Compiler: GNU C++20 (g++ 13.3.0), flags -O3 -DNDEBUG
Environment: Ubuntu Linux x86_64, Python 3.12.13
Reproducible Commands:
# 1. Configure and build isolated harness
cmake -B build -S . -DT81_BUILD_EXPERIMENTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target t81_exp_scg08_01_test

# 2. Run bounded smoke checks and deterministic verifications (179 trials)
./build/experiments/scg08_arithmetic_work/t81_exp_scg08_01_test

# 3. Execute CTest suite
ctest --test-dir build -R scg08 --output-on-failure

# 4. Validate evidence schema and negative test suite
python3 scripts/ci/validate_scg08_gate1_evidence.py
python3 scripts/ci/validate_scg08_gate1_evidence.py --test-negative

# 5. Verify root structure governance
python3 scripts/governance/check_root_structure.py
Verification Results Summary:

t81_exp_scg08_01_test: PASSED (0.40 sec, 179 cumulative trials executed, budget ceiling respected)
t81_exp_scg08_01_schema_validation: PASSED (0.33 sec)
t81_exp_scg08_01_schema_negative_validation: PASSED (0.33 sec, all 12 negative test cases rejected fail-closed)
check_root_structure.py: PASSED (82 entries checked)
7. Baseline & Changed-File Inventory

The current working tree is completely clean on branch jules-12290889561148666585-b677ece2 at commit 36b3849aa1e6297e88de2ec1655777793ab0b59e. Zero source code modifications were required for reconciliation, as the mainline code and committed artifacts already incorporate the resolved arithmetic findings.

Files audited in the baseline:

experiments/scg08_arithmetic_work/tier2_work_harness.hpp
experiments/scg08_arithmetic_work/tier1_markov_harness.hpp
experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp
experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp
experiments/scg08_arithmetic_work/gate1_evidence_record.json
experiments/scg08_arithmetic_work/evidence_schema.json
experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md
scripts/ci/validate_scg08_gate1_evidence.py
docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md
8. Remaining Scientific Acceptance Blockers

Gate 2 Benchmarking Withheld: Gate 2 execution (large-scale statistical trials, empirical cycle latency on physical CPUs, and cluster-robust delta variance estimation) remains strictly unauthorized and withheld.
Emulated Substrate Boundary: All arithmetic operations are executed on binary x86_64 hardware. Operation counts reflect software limb emulation and software trit loops, not native 3-state semiconductor physics.
Software Arithmetic Operation Parity Deficit: As demonstrated in the accounting reconciliation matrix, balanced-ternary software arithmetic incurs significantly higher $W_{\text{ops}}$ than binary (e.g., at $B=64$, ternary add requires $80$ $W_{\text{ops}}$ vs binary $4$; ternary mul requires $5,826$ $W_{\text{ops}}$ vs binary $29$). Software multi-precision ternary arithmetic does not exhibit an operation-count advantage on conventional binary hosts.
9. Scientific Disposition & Recommended Next Decision

Dispositions:

IMPLEMENTATION VERIFIED: YES. The C++ arithmetic work harness (t81_exp_scg08_01_test), decoupled base-$10^9$ exact integer oracle, trial budget enforcer ($\le 1,000$ ceiling), and primitive counters count exactly the primitive operations defined by the contract.
EVIDENCE VERIFIED: YES. The committed Gate 1 evidence artifact (gate1_evidence_record.json) strictly conforms to schema t81.scg08.carry-experiment.v1, correctly records the 179 cumulative trials executed in the smoke suite, and validates with 100% negative test suite coverage.
SCIENTIFIC ACCEPTANCE: WITHHELD. Arithmetic correctness and operation accounting reconciliation validate harness integrity only. They do not demonstrate or establish ternary architectural superiority. Progression to Gate 2 requires separate maintainer authorization.
Recommended Next Decision:

Maintain Gate 1 as IMPLEMENTATION VERIFIED and EVIDENCE VERIFIED. Withhold Gate 2 experimental campaigns pending explicit maintainer determination of whether to proceed with statistical characterization or conclude the study under the documented software overhead findings.

---

## Appendix C — Gate 2 Entry Decision Package

# &#x20;T81 — SCG-08 Gate 2 Entry Decision Package (Documentation Draft)

**Program Identification:** SCG-08 (Balanced-Ternary Carry Propagation & Arithmetic Work)\
**Evaluated Repository Baseline:** `origin/main` at commit `36b3849aa1e6297e88de2ec1655777793ab0b59e`\
**Classification:** Documentation-Only Formal Governance & Preregistration Decision Package\
**Target Gate:** Gate 2 (Experimental Execution Authorization)\
**Governing Authorities:**

1. `CHARTER`: `docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md` (PR #753, commit `0bd0817e4b9dc879fffce18c59ea9ff781f7acb3`)
2. `REV-R2`: `docs/research/continuity/SCG08_PHASE0_INDEPENDENT_ACCEPTANCE_REPORT.md` (PR #761, commit `5c69f11579b96686de1520aa45062b418c7c3bdd`)
3. `G1-REP`: `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` (PR #763 / PR #765, commit `36b3849aa1e6297e88de2ec1655777793ab0b59e`)
4. `SCHEMA`: `experiments/scg08_arithmetic_work/evidence_schema.json`
5. `STATUS`: `docs/ai-context/T81_RESEARCH_STATUS_2026-11-10.md` and `docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R2.md` (PR #764)

---

## Executive Determination & Recommendation

### **Recommendation: STRICT NO-GO FOR GATE 2**

Gate 2 experimental campaign execution **MUST NOT BE AUTHORIZED** at this time. Entry criteria fail across three non-fungible governance and methodological boundaries:

1. **Gate 1 Acceptance is Not Granted:** Code merged in PR #765 on `main` contains open Codex review findings regarding evidence provenance self-referentiality (`head_sha: dbf107fda...`) and scalar integer type validation holes (`operand_trits: null` accepted).
2. **Statistical Allocation Unreconciled:** The preregistered $10^7$ contiguous digit steps specified in `CHARTER` §5.3 have not been reconciled with the proposed 136,500 clustered sequences across 6 length bands. Compliance with the registered power cannot be mathematically or statistically inferred.
3. **Execution Authorization Remains Withheld:** Per `CHARTER` §13, Gate 2 execution requires explicit, dated maintainer approval, which has not been granted.

---

## 1. Gate 1 Acceptance Decision Template

A lawful Gate 1 acceptance decision must cleanly decouple five independent technical dimensions before any downstream gate can be considered:

```
# T81 SCG-08 Phase 1 — Gate 1 Formal Acceptance Decision Record

**Document ID:** T81-SCG08-GATE1-DECISION-<YYYYMMDD>
**Authoritative Evaluated SHA:** <40-char commit SHA of evaluated implementation>
**Baseline Repository SHA:** <40-char commit SHA of underlying main baseline>
**Review Profile:** Independent Governance & Correctness Verification

### 1. Implementation Correctness
- [ ] Multi-precision arithmetic algorithms (addition and convolution multiplication) link decoupled arbitrary-precision `ExactBigIntOracle` (base 10^9).
- [ ] Oracle verification passes across all five defined capacity bands: 64, 128, 256, 512, and 1024 bits.
- [ ] Arithmetic handles asymmetric negative boundary -2^(B-1), positive, zero, and mixed signs without truncation, non-canonical digit corruption, or overflow defects.
- [ ] Failure injection tests verify fail-closed termination (exit code 1) on oracle mismatch.
- *Finding Disposition:* [ACCEPTED / REJECTED]

### 2. Operation Accounting Correctness
- [ ] Frozen cost metric W_ops is strictly computed as:
      W_ops = OP_ALU_ADD_SUB + 3 * OP_ALU_MUL + OP_TABLE_LOOKUP + OP_BRANCH_COND + OP_NORM_PASS.
- [ ] In T81Limb27::addc(), table lookups are reconciled exactly to 38 per invocation (9 ADD_TABLE + 20 COMPOSITION_TABLE + 9 CARRY_FROM_ZERO).
- [ ] In binary mul64(), all 4 32x32 multiplications and 3 64-bit primitive additions are accounted without distortion.
- [ ] Diagnostic counters (diagnostic_convolution_ops, diagnostic_reduction_ops, norm primitive counters) remain decoupled and non-aggregating.
- *Finding Disposition:* [ACCEPTED / REJECTED]

### 3. Evidence-Content Validity
- [ ] JSON artifact validates strictly against schema t81.scg08.carry-experiment.v1.
- [ ] Validator strictly enforces non-negative integer scalar typing on all band fields (rejecting booleans, nulls, floats, negative values, and strings).
- [ ] Timestamp is valid ISO-8601 UTC with offset zero; specification version exactly matches approved reference.
- *Finding Disposition:* [ACCEPTED / REJECTED]

### 4. Generating-Run Provenance
- [ ] Evidence artifact git_context cleanly decouples baseline_sha from evaluated_implementation_sha.
- [ ] The artifact avoids circular self-referential publication hashes (evidence_publication_sha omitted or explicitly bound to publication PR commit).
- [ ] Historical recorded output (179 trials) is explicitly distinguished from fresh campaign runs.
- [ ] Execution environment (CPU, OS, compiler, flags, PRNG seed 0x5C608001ULL) is recorded for byte-exact reproducibility.
- *Finding Disposition:* [ACCEPTED / REJECTED]

### 5. Independent Review & Closure
- [ ] All open Codex/peer review findings (P1 provenance, P2 scalar schema) are verified as closed.
- [ ] Automated CI workflows (scg08 CTests, schema positive/negative validation) pass 100%.
- [ ] Maintainer Gate 1 Acceptance: [GRANTED / WITHHELD]
- [ ] Maintainer Gate 2 Execution Authorization: [STRICTLY WITHHELD]

```

---

## 2. Statistical Protocol Reconciliation: $10^7$ Steps vs. 136,500 Clustered Sequences

### 2.1 The Two Competing Specifications

| Specification Source                          | Defined Entity                           | Scope & Boundary Condition                            | Sample Specification            |
| --------------------------------------------- | ---------------------------------------- | ----------------------------------------------------- | ------------------------------- |
| **Charter §5.3 / §10.3** (`PREREGISTERED`)    | Elementary carry Markov chain steps      | Single contiguous Markov sequence                     | $M = 10,000,000$ steps ($10^7$) |
| **Phase 1 Addendum 03 / Report** (`PROPOSED`) | Clustered sequence trials across 6 bands | Independent sequences with boundary reset ($c_0 = 0$) | $M = 136,500$ sequence clusters |

### 2.2 Exact Step Totals by Band under Proposed 136,500 Allocation

The proposed implementation design structures trials into 6 sequence length bands $L \in {64, 128, 256, 512, 1024, 2048}$ trits/bits:

| Band Index | Sequence Length ($L$ digits) | Proposed Cluster Allocation ($K_b$ sequences) | Weight ($%$) | Total Digit Steps ($K_b \times L$) |
| ---------- | ---------------------------- | --------------------------------------------- | ------------ | ---------------------------------- |
| **Band 1** | $64$                         | $32,000$                                      | $23.44%$     | $2,048,000$                        |
| **Band 2** | $128$                        | $32,000$                                      | $23.44%$     | $4,096,000$                        |
| **Band 3** | $256$                        | $32,000$                                      | $23.44%$     | $8,192,000$                        |
| **Band 4** | $512$                        | $24,000$                                      | $17.58%$     | $12,288,000$                       |
| **Band 5** | $1024$                       | $12,000$                                      | $8.79%$      | $12,288,000$                       |
| **Band 6** | $2048$                       | $4,500$                                       | $3.30%$      | $9,216,000$                        |
| **Total**  | —                            | **$136,500$ clusters**                        | **$100.0%$** | **$48,128,000$ digit steps**       |

*(Note: In earlier draft summaries, an allocation of $20,000$ sequences at $L=64..512$, $10,000$ at $L=1024$, and $6,500$ at $L=2048$ yielded $37,708,000$ steps; in both formulations, total digit steps exceed $10^7$ by $3.7\times$ to $4.8\times$).*

### 2.3 Mathematical vs. Statistical Inconsistency Analysis

1. **Total Step Count Mismatch:**
   - The proposed clustered design executes $\approx 4.81 \times 10^7$ steps, while the preregistered protocol specifies $10^7$ steps. This is a $4.8\times$ sample size difference.
2. **Boundary Transient Distortion (Mathematical Non-Equivalence):**
   - In a single contiguous $10^7$-step sequence, initial boundary effects ($c_0 = 0$) affect only the first few trits; stationary mixing is achieved rapidly because the subdominant eigenvalue is $\lambda_3 = 1/9$, giving transient decay $(1/9)^k$.
   - In $136,500$ independent sequences, the boundary condition $c_0 = 0$ is re-initialized $136,500$ times. At digit $k=1$, $P(c_1 \neq 0 \mid c_0=0) = 2/9 \approx 0.2222$, whereas stationary occurrence is $\pi(c \neq 0) = 1/4 = 0.2500$. Across $136,500$ sequences, boundary resets introduce an aggregate negative bias: $$\Delta N\_{\text{active}} = 136,500 \times \sum\_{k=1}^L \left(-\frac{(1/9)^k}{4}\right) \approx -136,500 \times \frac{1/9}{4(1 - 1/9)} = -\frac{136,500}{32} = -4,265.6\text{ carries}$$
   - Therefore, a clustered sequence average is **not mathematically equivalent** to an uninterrupted stationary Markov stream.
3. **Serial Correlation & Variance Estimation (Statistical Non-Equivalence):**
   - As established in `REV-R2` §2, the effective sample size for the stationary indicator under asymptotic mixing is $M\_{\text{eff}} \approx 0.8M$.
   - In clustered sequences, intra-sequence observations are serially correlated, while inter-sequence observations are i.i.d. Standard sample variance formulas underestimate parameter uncertainty.
   - Variance must be calculated using a cluster-robust delta-method estimator: $$\hat{\sigma}^2\_{\hat{R}} = \frac{1}{\bar{V}^2} \frac{K}{K - 1} \sum\_{k=1}^K (U_k - \hat{R}V_k)^2$$
   - **Conclusion:** Clustered sequence sampling is **statistically superior** for multi-precision software testing (reflecting actual multi-precision register resets), but it is **not numerically or mathematically equivalent** to the charter's registered $10^7$ scalar Markov step criterion. A formal protocol amendment must authorize this reconciliation.

---

## 3. Thresholds & Stopping Rules: Authoritative Provenance Register

| Proposed Metric / Threshold                                                          | Categorization      | Authoritative Source Citation                   | Current Documented Status                                               |
| ------------------------------------------------------------------------------------ | ------------------- | ----------------------------------------------- | ----------------------------------------------------------------------- |
| **$f\_{\text{gen}\mid 0} \in [0.217, 0.227]$**                                       | **`PREREGISTERED`** | `CHARTER` §10.5 (line 499)                      | Theoretical $2/9 \approx 0.2222$; bounds frozen in Phase 0.             |
| **$f\_{\text{trans}} \in [0.161, 0.172]$**                                           | **`PREREGISTERED`** | `CHARTER` §10.5 (line 500); `REV-R2` §7         | Theoretical $1/6 \approx 0.1667$; verified in R2.                       |
| **$f\_{\text{occ}} \in [0.245, 0.255]$**                                             | **`PREREGISTERED`** | `CHARTER` §10.5 (line 501)                      | Theoretical $1/4 = 0.2500$; primary stationary frequency.               |
| **$f\_{\text{cont}\mid\text{act}} \in [0.328, 0.338]$**                              | **`PREREGISTERED`** | `CHARTER` §10.5 (line 502)                      | Theoretical $1/3 \approx 0.3333$; continuation rate.                    |
| **$\bar{L}\_{\text{run}} \in [1.450, 1.550]$ trits**                                 | **`PREREGISTERED`** | `CHARTER` §10.5 (line 503)                      | Theoretical $E[L] = 1.500$ trits; geometric expectation.                |
| **$W\_{\text{ops}}$ parity $\le 1.10\times$ at $\ge 512$ bits**                      | **`PREREGISTERED`** | `CHARTER` §10.5 (line 504)                      | Algorithmic operation work parity target for Tier 2.                    |
| **Falsification: $W\_{\text{ops}} > 2.0\times$ binary across all bands**             | **`PREREGISTERED`** | `CHARTER` §10.5 (line 506), §9 Table (line 434) | Universal stopping rule: immediate H02 falsification.                   |
| **Falsification: Ternary $f\_{\text{occ}} > 0.35$ or $\bar{L}\_{\text{run}} > 2.0$** | **`PROPOSED`**      | None in `CHARTER`; derived in Assessment        | **NOT PREREGISTERED**. Must be ratified in Gate 2 authorization.        |
| **Hard Ceiling $\le 1,000$ cumulative trials**                                       | **`PREREGISTERED`** | `G1-REP` §6; `test_exp_scg08_01.cpp`            | Enforced fail-closed in Gate 1 harness; requires relaxation for Gate 2. |
| **$M\_{\text{eff}} \approx 0.8M$ effective sample variance**                         | **`PROPOSED`**      | `REV-R2` §2 (lines 53–62)                       | Asymptotic qualification; not formally part of charter threshold table. |
| **Signed Domain $[-2^{B-1}, 2^{B-1}-1]$**                                            | **`PROPOSED`**      | Addendum 03; `STATUS` line 12                   | Requires formal preregistration amendment recording.                    |

---

## 4. Separation of Research Hypotheses & Tiers

To prevent conflation of mathematical theory with software overhead or CPU microarchitecture, EXP-SCG08-01 enforces strict separation across three tiers:

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ TIER 1: Primary Mathematical Carry-Propagation Dynamics                     │
│ ∙ Hypothesis: H02-A / H02-B (Pure Carry Markov Chain Parity)                 │
│ ∙ Question: Does balanced-ternary digit addition produce theoretical stationary│
│   carry occurrence π(carry ≠ 0) = 0.2500 and mean run length L = 1.500 trits? │
│ ∙ Substrate: Pure scalar digit stream; decoupled from limb arrays and OS.   │
│ ∙ Consequence: Success validates adder math; does NOT prove runtime speedup. │
└──────────────────────────────────────┬───────────────────────────────────────┘
                                       │
┌──────────────────────────────────────▼───────────────────────────────────────┐
│ TIER 2: Secondary Algorithmic Weighted Operation Work (W_ops)                │
│ ∙ Hypothesis: H02-C / H03-A (Algorithmic Operation Work Parity)              │
│ ∙ Question: Does the 50% reduction in carry frequency overcome tryte         │
│   packing and 38-lookup limb addition overhead to achieve W_ops ≤ 1.10x?     │
│ ∙ Substrate: Multi-limb structures (T81Limb27/54 vs. uint64_t limbs).        │
│ ∙ Metrics: Pure operation counts: ALU add/sub, mul, lookups, branches.       │
│ ∙ Falsification: If W_ops > 2.0x binary across all bands, H02 is FALSIFIED.  │
└──────────────────────────────────────┬───────────────────────────────────────┘
                                       │
┌──────────────────────────────────────▼───────────────────────────────────────┐
│ TIER 3: Exploratory Host Microarchitectural Timing                           │
│ ∙ Hypothesis: None (Explicitly Exploratory / Non-Normative)                  │
│ ∙ Question: What is the raw wall-clock latency on binary x86_64/AArch64 host?│
│ ∙ Substrate: Host CPU execution (Intel Xeon, AMD EPYC, Apple Silicon).       │
│ ∙ Strict Caveat: Measures host instruction emulation tax, NOT native ternary│
│   silicon performance. Cannot be cited as proof of ternary hardware speed.   │
└──────────────────────────────────────────────────────────────────────────────┘

```

---

## 5. Minimum Gate 2 Authorization Template

```
# T81 SCG-08 — Gate 2 Experimental Execution Authorization

**Authorization Identifier:** T81-SCG08-GATE2-AUTH-<YYYYMMDD>
**Preregistered Experiment ID:** EXP-SCG08-01 (Tiers 1, 2, and 3)
**Authoritative Evaluated Baseline SHA:** <40-char commit SHA from clean main>
**Parent Research Charter:** `docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md` (PR #753)
**Implementation Baseline:** `experiments/scg08_arithmetic_work/` (PR #763, PR #765, PR #766)

### 1. Workload & Experimental Scope
- **Tier 1 (Markov Validation):** Uniformly distributed digit sequences across lengths L ∈ {64, 128, 256, 512, 1024, 2048}.
- **Tier 2 (Arithmetic Work):** Exact multi-precision addition and multiplication across bands B ∈ {64, 128, 256, 512, 1024} bits.
- **Verification Oracle:** Decoupled ExactBigIntOracle active on 100% of trials; 0 discrepancies tolerated.

### 2. Execution Parameters & Determinism
- **Deterministic PRNG Seed:** `0x5C608001ULL` (std::mt19937_64).
- **Sample Allocation Model:**
  - [ ] Option A (Charter Protocol): 10,000,000 continuous digit steps.
  - [ ] Option B (Reconciled Clustered): 136,500 sequences (48,128,000 digit steps) across 6 length bands.
- **Statistical Estimator:** Cluster-robust linearized delta-method variance estimation.

### 3. Execution Budget
- **Trial Ceiling:** Relaxed from Gate 1 smoke limit (1,000) to authorized campaign ceiling: <EXACT CEILING, e.g., 140,000 trials>.
- **Compute Time Ceiling:** Maximum 120 minutes wall-clock execution time. Fail-closed on timeout.

### 4. Stopping Rules & Falsification
- **Correctness Fault:** Any ExactBigIntOracle mismatch halts execution immediately with exit code 1.
- **Carry Dynamic Divergence:** If Tier 1 measured f_occ > 0.350 or mean run length > 2.000 trits, halt campaign; model is rejected.
- **Algorithmic Falsification:** If Tier 2 balanced-ternary W_ops exceeds binary W_ops by > 2.0x across all bands B ∈ {64..1024}, immediately terminate and declare H02 FALSIFIED.

### 5. Responsible Approver Sign-Off
I hereby certify that Gate 1 implementation and accounting correctness have been independently verified, PR #766 has been merged, and statistical allocation reconciliation is complete. Experimental execution under the specified parameters is authorized.

**Maintainer Signature:** _______________________  
**Date (UTC):** _______________________  

```

---

## 6. Identification of Evidence-Provenance Gaps

Certain governance and provenance deficiencies **cannot** be resolved by retroactively editing documentation:

1. **Self-Referential Hash Cycles:** An evidence JSON file committed to Git cannot record its own merge commit hash without altering the file and generating a new commit hash.\
   *Resolution:* Requires the structural schema change implemented in **PR #766**, which explicitly separates `baseline_sha` from `evaluated_implementation_sha` and designates `evidence_publication_sha` as optional.
2. **Historical Run Re-attribution:** The 179-trial output committed in `gate1_evidence_record.json` was generated under commit `2dd3cc62e526942cdcbed6d100770c08c26ab2fe`. It cannot be claimed as a newly executed test of current `main` (`36b3849`) without re-running the harness.\
   *Resolution:* Documenting the 179 trials as a historical reference snapshot (as PR #766 does) resolves the attribution gap.
3. **Statistical Sample Non-Equivalence:** The discrepancy between the charter's $10^7$ contiguous steps and the proposed $136,500$ clustered sequences is a mathematical fact. It cannot be resolved by documentation reinterpretation.\
   *Resolution:* Requires a formal recorded maintainer addendum amending the preregistration protocol to authorize clustered sequence sampling with cluster-robust delta variance estimation.

---

## 7. Final Actionable Route to Gate 2

To proceed lawfully toward Gate 2 execution, the following four sequential milestones must be reached:

```
[CURRENT BASELINE: 36b3849] (Gate 2 NO-GO)
       │
       ▼
[STEP 1: Merge PR #766]
       ├── Decouples baseline_sha from evaluated_implementation_sha.
       └── Fixes scalar integer validation in validate_scg08_gate1_evidence.py.
       │
       ▼
[STEP 2: Issue Gate 1 Formal Acceptance Decision]
       └── Maintainer signs Section 1 template confirming implementation & accounting correctness.
       │
       ▼
[STEP 3: Issue Preregistration Amendment for Statistical Allocation]
       └── Formalize 136,500 clustered sequences and cluster-robust delta variance estimator.
       │
       ▼
[STEP 4: Execute Gate 2 Authorization Record]
       └── Maintainer signs Section 5 authorization template. Gate 2 transitions to GO.

```

*This package is delivered as a read-only governance draft. No code modifications, PR merges, or unauthorized executions have been performed.*