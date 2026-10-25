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
