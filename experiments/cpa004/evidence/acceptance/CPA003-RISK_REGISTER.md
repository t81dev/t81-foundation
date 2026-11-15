# CPA-003 risks and unresolved discrepancies

All records here are proposed experimental material. No accepted research record
or production source was changed. Numerical agreement is scoped to this artifact.

| ID | Finding | State and effect | Required boundary/action |
|---|---|---|---|
| R01 | Capability document / registry / header inventories are 174 / 209 / 244; four registry assignments conflict with the header. | Open historical discrepancy. All 17 opcodes in this program agree across the header and registry and are below 174; it does not affect these bytes. | Preserve the discrepancy; repair authority only through a separate authorized task. See `inventory-assessment.json`. |
| R02 | PR #707 is open and unmerged at head `55223cbd3ee29b5391e151197fe6eee1b8889452`; main remains `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4`. | Operational prerequisite for accepting the broader restored conformance gate. The PR is test/validation only; the hosted semantics audited here are unchanged. | Do not describe the repaired matrix as merged. Await human review/merge decisions; this task does not merge. |
| R03 | This agent authored CPA-001 and CPA-002 and retains their conversational context. | Reviewer relationship disclosed. Separate Python mathematical computation and C++ observation do not establish independent-person scientific review. | Permit a new reviewer to reproduce the packet; make no reviewer-independence, H08 or scientific-acceptance claim. |
| R04 | Only one positive program and selected scalar trit inputs are exercised. JZ's condition agrees with the zero flag in this fixture; JNZ does distinguish the register from zero-flag interpretation. | Open coverage limit, not an observed defect. No full ISA, truth-table or branch-family conformance follows. | Admit exact bytes only; no arbitrary-program or general interpreter claim. |
| R05 | Trap instructions are skipped; division-by-zero, bounds, invalid operands, policy denial, step exhaustion and timeout do not occur in the positive run. | Source-derived negative behavior only. No executed negative-trap evidence is fabricated. | Retain precise fault classes and source PC/next-PC rules in the contract; add separately reviewed negative fixtures if the later scope needs them. |
| R06 | System signatures use an exact nonstandard FNV offset and 64-bit sign extension; integer JSON consumers can lose precision. | Verified for all observed boundaries; portability risk remains for future implementations. | Use the pinned word/byte formula and decimal strings in normalized records. Do not substitute standard FNV offset or SHA-256 for R76/R77. |
| R07 | Hosted `load_program` synchronizes system registers before resetting its instruction counter; fresh-VM loading is the admitted path. | Source-order hazard outside the tested reload path; no reload defect is claimed from this experiment. | Create a fresh VM/engine in every run. Do not infer reused-instance initialization equivalence. |
| R08 | Compiler `.tisc` serialization and native T81X bytes are different formats. | Resolved for this artifact: independent struct packing/manual reading and existing C++ encode/decode agree on raw13 bytes. | Preserve 351-byte identity and the digest; do not substitute a container or recompile in either future profile. |
| R09 | No guest evaluator exists or ran in this task; no post-ExitBootServices evidence was produced. | Deliberately untested future profile. Agreement applies to oracle/artifact/hosted observations only; overall disposition has explicit prerequisites. | Obtain separate implementation and execution authorization. Require live post-boot records and input-buffer identity, not a generated readiness banner. |
| R10 | Build reused an existing baseline CMake directory; Ninja recovered a build-log warning before a successful 27-action rebuild. Only macOS ARM64 hosted execution was performed. | Recorded limitation. No independent clean-build, cross-platform or CI pass is claimed. | Retain commands, library hashes and source pins; a reviewer can rebuild from the pinned revision. |
| R11 | Full Axion audit-log/hash equivalence and CanonFS storage/authenticity are excluded; default policy mask is zero despite explicit allow-all engine. | Open assurance boundary; numeric equality does not certify a promoted backend or governed OS. | Preserve policy-before-dispatch in the existing host; do not claim dynamic guest policy or storage conformance. |
| R12 | JSON Schema validity and artifact hashes cannot establish true provenance, genuine consent or correct execution against a malicious host. | Permanent evidence boundary. H07's comparative auditability claim remains untested. | Keep `status: proposed`, future freestanding authorization ungranted, and canonical H01–H08 history unchanged. |
| R13 | 32 KiB evaluator workspace is a proposal, not a measurement; guest serial transport/firmware timing is untested. | Open implementation/resource prerequisite. | Measure actual map/stack footprint later, stream observations, enforce 64 instructions/30 seconds; timeout is inconclusive failure, never Halt. |

No numerical, artifact-construction, supported-observation or hosted-state
disagreement was found in the three admitted runs. `hosted-comparison.json`
retains an empty differences list and the five required classification categories.
No oracle correction was made after observing the host.

## Material contract clarification prerequisite

**R14 (open, affects this program):** normative memory/control-flow prose in
`spec/tisc-spec.md:1291–1334` conflicts with the direct-address, immediate-target,
register-tested JZ/JNZ behavior in source and the opcode reference. The TXor
“XOR / Sum” label is also ambiguous beside the actual wrapped-difference rule.
See `discrepancy-register.json` for exact paths, line ranges and impact. These
are unresolved source-contract discrepancies. The numerical oracle remains
verified for the pinned hosted profile, but disposition is **READY WITH
PREREQUISITES** until a human explicitly clarifies/accepts that experimental
profile before implementation. No frozen document or production behavior was
changed and no oracle output was adjusted to force agreement.
