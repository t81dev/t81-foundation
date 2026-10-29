# Append-only research session handoff

## 2026-10-25 — Charter v1.0 continuation / register 1.0.0

**DATE / SESSION:** 2026-10-25, America/New_York. First source-grounded continuation of the supplied master charter. This file is separate so the original charter remains byte-for-byte intact; append future entries rather than rewriting this result.

**REPOSITORIES AND PINNED COMMITS:** Hanoi `59205d76db4aca3da7c0bdc521924b8f9bc52cc3`; Foundation `f6d4d680709c5c6c98b305628e04346f5ef61620`; t81-lang `bcce8e729595df8fdcf92ad81c0c751fe0d928d6`; t81-vm `4e2bba639d05249722db847563ae37991289ea52`; t81lib `995ea70796259a8f9c5f265a6cb3084143e2b949`; t81-hardware `834521d4e6b37cf8c871b1ac64af449ff6493b7d`. Historical commits and recorded author/committer dates are in the pin/lineage records.

**QUESTION OR HYPOTHESIS TESTED:** What source continuity can be established, and can the legacy arithmetic support a balanced positional-integer performance comparison? Which minimal comparison separates radix from adaptive precision?

**EVIDENCE EXAMINED:** Selected source files and targeted queries over the available histories in the six repositories; original user charter; Foundation legacy CWEB objects; t81-lang migration script/parser bytes; standalone VM migration plan; current scheduler/promotion/VM context definitions; digital RTL contract; live GitHub PR metadata. The referenced historical PDF/transcripts and original paper were unavailable and were not treated as inspected evidence.

**WHAT WAS REPRODUCED:** 41 shared source blobs in the pinned Foundation ancestor history and Hanoi import; exact recursion and initial split-parser identities; isolated CWEB function compilation and exhaustive representable balanced-domain probes; exact finite-state storage calculations. Root Hanoi make dry-run fails with missing hvm_loader.c rule (exit 2). Probe collection succeeds but candidate arithmetic fails; no full Hanoi build or integrated scheduler test passed.

**WHAT WAS ONLY REPORTED OR MODELED:** Knuth/intellectual history and missing historical artifacts; previous security/experimental reports; native three-state physical benefit; speedups and modern cross-repo semantic equivalence. H01 is a conditional mathematical cost derivation, not device data. The four-arm next experiment is specified, not implemented.

**RESULT / NEGATIVE RESULT:** Addition fails 422/547 exact representable balanced cases, multiplication 140/201 under positional-integer interpretation, negation zero of 27. Componentwise multiplication may suit another algebra; it is unsuitable evidence for the scalar integer hypothesis. Tier meanings and mappings are unresolved. Public Git chronology reveals a legacy archive and later extraction/adaptation, not a simple Hanoi-repo -> language -> VM -> Foundation timeline. At M=256 and element ratio 1.5, finite ternary storage costs 9 versus binary 8 despite satisfying the asymptotic threshold.

**KNOWN DEFECTS / CONFOUNDERS:** Full CWEB build/integration unverified; expected headers/path rules incomplete; different mode fields/opcode constants/triggers; ambiguous scalar/vector semantics; physical costs, confidence of historical claims and optimized implementation equivalence absent. Prior broad CPU-negative statements do not establish future-device impossibility. A benchmark cannot convert incorrect semantics into scientific advantage.

**OPEN PRS AND CI STATUS:** #565 and #566 remain open/draft at `736e4fd9754936b4851564881cfb244e359d916e` and `5bfe915531f0629b3dabb15cda6c374fe86945a0`. Current checked timestamp and exact check URLs/results are in evidence/maintenance-status.json. Both CodeQL checks are successful; both required quality gates fail. No merge, closure, protection change or CI repair in this session. Protocol 001 remains NOT READY.

**NEXT SINGLE ACTION:** Produce H04-R01's four-arm exact trace ledger and independently check the domain/range and packing formulas before an implementation or timing experiment. Keep adaptive binary as a required control. Continue the bounded CI baseline investigation separately, without substituting it for the primary scientific question.

**STOPPING CRITERION:** Stop if domains/units, independent oracle, matched optimized baseline or conversion/selection costs cannot be specified fairly; report under-specification or a negative result. No new VM, scheduler, ISA, large framework or Protocol 001 harness is authorized by this packet.


## 2026-10-25 — H04-R02 independent audit and integration

**DATE / SESSION:** America/New_York; remote evidence read around 20:05 EDT
(2026-10-26 00:05 UTC). Original charter and H01–H08 definitions unchanged.

**REPOSITORIES AND PINNED COMMITS:** Foundation main `01c85019319fd020922402ba356878e392cb2231`;
#570 `8a49ba1d89f1d2f43ea1bdd654cc15e5f2f79ecd`;
#571 `b2918551a676762c401f492d2cc591bab45c2488`;
local H04-R01 `def47c680f477bb1a7bb10ebb57e11a0c7b24a6f`.

**QUESTION / EVIDENCE:** Independent H04-R02 equations, DP optimality, oracle
independence, comparator domains and actual Base-243 packing. Full inventory,
prior reports, live PR reviews/status, recovered R01 archive and verification
logs are in [the integration packet](../../../experiments/h04_r02_adaptive_representation/EVIDENCE_INVENTORY.md).

**WHAT WAS REPRODUCED:** C++ and Python ledgers, exact finite exhaustive schedule
checks, 10-bit control and reversible codec byte counts; clean CMake/CTest,
sanitisers and bounded research regressions. Recovered R01 source produced a
packet accepted by its separate verifier (384 arm evaluations, 192 disabled
equalities, 1596 sweep points), and its 19-test suite passed. Exact counts,
commands and limitations are in the packet's VERIFICATION.md.

**WHAT WAS ONLY MODELED:** Symbol cost ratios and transition/conversion charges.
No new CPU execution, energy or native-device performance measurement. R01
reexecution is not an independent proof of all its assumptions or a second lab.

**RESULT / NEGATIVE RESULT:** Original H04-R02 inference invalid; corrected model
is conditional. Six-trit full domain needs 10 covering signed bits, unlike 8 bits.
Base-243 has a real stream-byte saving against packed 10-bit fields, but binary
block coding can use the same code language. Equal metadata cannot reverse
11.6<12. A universal no-software-ternary conclusion is withdrawn; H04 is open.

**KNOWN DEFECTS / CONFOUNDERS:** Original header scaling, wrong published ratios,
greedy C++ versus declared DP, unequal mode capacity, overgeneralized attribution,
false absent-history statements, unsupported performance/closure inferences.
Integration also fixes block modes, unsafe cost sentinel and reoptimized roots.
See the consolidated discrepancy ledger; invalid prior claims are preserved.

**OPEN PRS / CI:** #568/#569 merged; #570/#571 open at inspected heads. Their
checks were not all successful; snapshots preserve individual states. Recommend
reviewing and merging the integration PR after required checks, then closing
#570/#571 as superseded. No merge or closure performed. Main-wide CI failures
are not reclassified as passes by successful local research checks.

**NEXT SINGLE ACTION:** Predeclare the matched-domain codec/block/alignment/access
comparison in the integration RESULTS.md, then measure bytes separately from
end-to-end latency/bandwidth. Continue Track R with Track F evidence support and
Track M correction provenance; no new subsystem or ISA work.

**STOPPING CRITERION:** Reject a radix-exclusive claim if the same-domain binary
block code matches the codec; reject a practical performance claim if measured
access/conversion overhead removes its predeclared benefit. Preserve negative
results and open device/arithmetic questions.
