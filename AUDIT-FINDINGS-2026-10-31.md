# External audit findings for t81-foundation

External audit findings for t81-foundation, at the maintainer's request
(ai-context handoff test: an AI reading only `docs/ai-context` should fully
understand T81 and be able to PR real issues back).
Audited commit: `e108fda` (2026-10-31).
Submitted for the maintainer's review — no code changes proposed in this PR,
findings only.

## Method and limits

- Read-only audit of a local clone at HEAD
  `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`. No repo files were modified,
  no branches created, no code changes are proposed here.
- Shallow clone (phase 1 window: 117 commits; phase 2: depth ~100). A SHA or
  GitHub PR state outside that window is reported as UNVERIFIABLE, never as
  missing. PR open/closed/merged states could not be verified from the clone
  and are neither confirmed nor denied.
- Phase 1 covered the seven files in `docs/ai-context/` only and did not run
  builds. Phase 2 covered the full repo beyond phase 1 in five areas
  (core code, security surfaces, top-level claims, dead references, build/test
  health) and did not re-audit `docs/ai-context/`.
- Phase 2 build work was bounded: the documented Ninja preset configure
  failed only because the environment has no Ninja binary (not a repo
  defect); the Unix Makefiles fallback configured successfully. Two small
  test targets were built and passed 2/2. No full build and no full `ctest`
  run was attempted (1,626 make targets, 410 registered tests inventoried,
  not executed). Test-suite pass rates the repo claims are therefore
  assessed as unconfirmed by this pass, not disproven.
- Severity key: HIGH = a maintainer would want to know before trusting or
  shipping this; MEDIUM = genuine defect with bounded blast radius;
  LOW = nit, latent issue, or hygiene. "Needs check" items are unconfirmed
  leads, not confirmed bugs.

## Verdicts

### Phase 1 — docs/ai-context pack

This pack is **not yet safe for an AI session to trust as-is** — but it is
close, and its failures are fixable documentation bugs, not rot. Its
discipline is real: the pins that can be checked mostly verify, the
prohibited-claims list is honored throughout, the 10-31 corrections genuinely
demote stale sections, and the mission/hypothesis content is complete enough
that a fresh reader does come away understanding what T81 is and what it is
trying to prove. What makes it unsafe as-is is concentrated in three places.
First, the single most-cited number in the pack — the 10.6× Tier A radix
result — is pinned to a file that does not exist and is flagged by the repo's
own claims audit as untraceable, so a session following the pack will launder
an unverified figure into a FROZEN verdict. Second, the pack contradicts
itself on the exact head of PR #637 and on CMP3 open/closed state, which is
precisely the kind of pin the pack demands sessions get right. Third, judged
as the maintainer's AI-to-AI handoff test, the pack teaches T81 but does not
close the loop: it never tells the newcomer how to PR a finding back, never
consolidates current state and next actions on one page, and leans on
undefined vocabulary (CMP3, ADM-2, T1–T8, DAIOS, Baseline-X) and undeclared
GitHub access. Fix the Tier A radix pin (H1), reconcile the #637 head (H2),
and add the contribution path plus a single current-state box (HND-1, HND-2),
and this pack becomes what it aims to be: a handoff a fresh AI can trust and
build on.

### Phase 2 — full repo

The repo is a large, genuinely researched codebase whose ternary arithmetic
core, interpreter integer/float paths, and SHA3-512 implementation are largely
sound — but its headline guarantees do not compose. The single most damaging
pattern is that determinism and policy enforcement, the two properties the
project sells hardest, each have a hole at the layer where they must hold:
the JIT produces different results from the interpreter on ordinary code (so
"bit-exact" depends on whether a loop got hot), and the Axion policy boundary
can be displaced by the program it is supposed to constrain while loop
requirements are satisfied by evidence manufactured from the policy itself.
Around that core, the top-level documents overstate what is delivered (a
frozen ISA version the spec does not define, opcodes the contract says to
trap that the VM dispatches, "production-ready" labels the governance
register contradicts, universal bit-exactness the README itself disclaims),
the test suite silently omits 26 real tests from `ctest` including the ones
CI's epoch-determinism gate runs, and the docs below the clean entry points
carry one systematic stale-layout defect in their paths. None of this means
the research is hollow — the experiments, the self-correcting claims audit,
and the honest README disclaimers are real strengths — but a reader should
trust the README's hedged sections and the determinism/surface registers,
verify any specific capability claim at source level, and treat the
historical reports as dated descriptions until the HIGH items below are
fixed.

## Phase 1 findings — docs/ai-context pack

### What verifies (strengths)

- Header pins `f8ac771` (Oct 28) and `f775680` (Oct 29) both exist and are
  ancestors of HEAD; BOOTSTRAP's `5fcc717` also exists and is an ancestor.
- The #635 original-protocol blob claim verifies byte-identically
  (`0fbe0b4da1614867bf51b1c3eaf200f82d789a65`).
- Short/full PR heads are consistent across files for #649–#653.
- H01–H08 register complete in
  `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`; bounded
  compositions `assess-fixed`, `route-fixed`, `classify-fixed` present in
  `docs/reference/STABLE_BASELINE_CONTRACT.md`.
- RFC statuses verify: RFC-0030 Accepted; RFC-0042–0049 and RFC-0051
  Accepted; RFC-00D1, RFC-00D2 Draft.
- H04-R02 corrected numbers verify against
  `experiments/h04_r02_adaptive_representation/CORRECTED_RESULTS.md:19`
  ("At r=1.60 AT=1210 < AB=1250", crossover 5/3).
- B-arm figure verifies:
  `examples/governance/accountable-computation/REPORT.md:146`
  ("0/150 A, 90/150 B, 0/150 C").
- CLM-03/04 are `UNDER_ACTIVE_TEST` in
  `experiments/research_001/CLAIM_REGISTER.md:34–35`.
- MatVec numbers corroborated outside the pack by
  `TERNARY_COMPARATIVE_CLAIMS_AUDIT.md` (independent re-execution reported).
- Self-correction practice is real: HANDOFF §7's erroneous "PR #634 merged"
  line is preserved and explicitly corrected by the 2026-10-29 note, and the
  2026-10-31 sections demote older sections to history.
- No prohibited overclaim was found asserted anywhere in the pack: no
  ternary speedup on binary CPUs, no commercial-OS claim, no physical
  ternary silicon claim, no custom-VM-necessity or Wasmtime-equivalence
  claim — the pack repeatedly falsifies all of them.

### HIGH

**H1 — The flagship Tier A falsification number (10.6×) has no valid pin,
and the repo's own claims audit flags that exact figure as unsupported.**
Evidence: `docs/ai-context/EVIDENCE_AND_FINDINGS.md:20` (table, Tier A:
"Software tryte addition (11.7 ns) is 10.6× slower than native 64-bit binary
ALU addition (1.1 ns)"); `:61` ("Source Pin:
`experiments/radix_comparison/benchmark.cpp`" — no commit SHA, and that file
does not exist at HEAD or at any checked pin; the experiment lives in
`experiments/radix_comparison/main.cpp`). The figure is repeated as
established fact in `RESEARCH_IDENTITY.md:55` (H01 result),
`DECISIONS_AND_GOVERNANCE.md:30` (RD-001, "proved"), `START_HERE.md:81`
(prohibited-claims list), and `BOOTSTRAP.md:20`. Contradicting in-repo
evidence: `TERNARY_COMPARATIVE_CLAIMS_AUDIT.md:41` classifies "Scalar Tryte
ALU takes 11.70 ns" as Unsupported / Unverified ("No benchmark script or
source code in the repository generates these exact nanosecond numbers"),
and line 132 lists 11.70 ns as "N/A (Not Found)". The checked-in
`experiments/radix_comparison/results.json` contains neither 1.1 ns nor
11.7 ns (it reports binary64 1.99995 ns, radix81 105.337 ns). The qualitative
conclusion (software ternary slower than binary ALU) is supported by that
results file; the precise Tier A numbers are not. Why HIGH: the pack's own
Tier A standard (`START_HERE.md:136`) demands "exact source revision,
executed command/configuration, environment, input identities, output/exit
receipt" — this entry meets none of them, yet is labeled Tier A and used to
justify a FROZEN verdict.
Proposed fix: downgrade the row and rewrite the pin — Tier B/C, citing
checked-in `results.json` (binary64 1.99995 ns vs radix81 105.337 ns), noting
the earlier research note's 11.7 ns vs 1.1 ns (10.6×) is flagged untraceable
by the repo's own claims audit, with the verdict "FALSIFIED & FROZEN for
software CPU ternary arithmetic speedup as a qualitative verdict; the
precise 10.6× ratio is NOT Tier A". In §2.4 replace the pin with
"`experiments/radix_comparison/main.cpp` and `results.json` at HEAD; no
commit pin establishes the 11.7 ns / 1.1 ns figures".

**H2 — Two different full SHAs are cited as the head of PR #637, with no
reconciliation.**
Evidence: `docs/ai-context/DECISIONS_AND_GOVERNANCE.md:60` and
`ACTIVE_RESEARCH_HANDOFF.md:21` both give #637 head
`1a71733cdd266a34a174d13185c78e5b6a07c68e`. `BOOTSTRAP.md:28` says "#637
head was `42b9f6240b136b19c52476727f9f78d2c2c01c08`" and links the
authority record, activation record, original protocol, and r1 validation
report to that commit. Neither file acknowledges the other SHA. (Both SHAs
are PR heads and therefore UNVERIFIABLE from this clone; the contradiction
itself is visible in-pack regardless of which head is right.) Why HIGH:
PR-head exactness is the pack's core discipline ("Pin fresh `main` and PR
heads"). A session acting on either SHA will review a different protocol
version, and nothing in the pack tells it which one is authoritative.
Proposed fix: append to `BOOTSTRAP.md:28`: "Note:
DECISIONS_AND_GOVERNANCE.md §3 and ACTIVE_RESEARCH_HANDOFF.md §1 cite an
earlier #637 head `1a71733cdd266a34a174d13185c78e5b6a07c68e`. The head above
is the later checked head; recheck the live PR head before acting and do not
treat either SHA as current without that check." Add the matching one-line
pointer in DECISIONS §3 / HANDOFF §1 pointing to the BOOTSTRAP head as the
later check.

### MEDIUM

**M1 — H04-R02 summary tables still lead with the superseded "conditional
negative outcome" framing; only the §6 correction carries the corrected
result.**
Evidence: `EVIDENCE_AND_FINDINGS.md:22` (table: "modeled advantage
disappears"), `:137` ("conditional negative result"),
`RESEARCH_IDENTITY.md:82` ("reports a conditional negative outcome"),
`BOOTSTRAP.md:22` ("conditional negative outcome"),
`ACTIVE_RESEARCH_HANDOFF.md:20` ("conditional negative result"). The
correction at `EVIDENCE_AND_FINDINGS.md:146` says the corrected
`boundary_oscillation` result is AT=1210 < AB=1250 — adaptive ternary cheaper
in that modeled case — and that "earlier descriptions of a *conditional
negative outcome* remain dated source characterizations and must not
suppress this corrected conditional counterexample." That correction is
verified against `CORRECTED_RESULTS.md:19`, but none of the five summary
locations points to it.
Proposed fix: append to each summary location (e.g. EVIDENCE :22 verdict
cell): "See §6 correction: corrected accounting gives AT=1210 < AB=1250 at
r=1.60 for `boundary_oscillation` — a conditional counterexample, not a
universal negative. Do not cite this row's 'negative outcome' wording
without §6."

**M2 — SYSTEM_ARCHITECTURE §2 subsystem paths are wrong in five places; a
session following them cannot find the code.**
Evidence (all in `SYSTEM_ARCHITECTURE.md`): `:56` cites `src/vm/vm.cpp` —
actual implementation is `vm/vm.cpp` (`src/vm/` contains only
`CMakeLists.txt`, which itself references
`${PROJECT_SOURCE_DIR}/vm/vm.cpp`). `:61` cites
`include/t81/kernel/axion/` — actual header dir is `include/t81/axion/`
(`src/axion/` is correct). `:66` cites `src/core/fs/` and
`include/t81/fs/` — `src/core/` contains only a `CMakeLists.txt`; actual
CanonFS headers are `include/t81/canonfs/` and tooling is
`tools/cli/canonfs_tools/`. `:76` cites `src/core/isa/math/` — actual
soft-math implementation is `isa/math/t81_soft_math/t81_soft_math.cpp` with
headers in `include/t81/math/`. `:81` cites `src/tools/model/` — actual dir
is `tools/model/` (gguf bridge, weights emitter). §4 `:107` repeats the
`src/vm/` + `spec/vmspec.md` errors (`spec/vmspec.md` exists nowhere in the
repo); `:109` repeats the `src/core/fs/` error.
Proposed fix: replace each Path line with the verified path:
`vm/vm.cpp`, `include/t81/vm/vm.hpp`; `src/axion/`,
`include/t81/axion/`; `include/t81/canonfs/`, `tools/cli/canonfs_tools/`
(CanonFS core implementation location to be confirmed by maintainer if a
separate core library dir exists); `isa/math/t81_soft_math/`,
`include/t81/math/`, `vm/determinism/`; `tools/model/`, `tools/cli/ai/`.
In §4: replace `src/vm/` with `vm/`, and replace the `spec/vmspec.md` cell
with "no `spec/vmspec.md` in repo — VM spec status to be confirmed".

**M3 — The RTL evidence path does not exist at HEAD or at either doc
snapshot.**
Evidence: `SYSTEM_ARCHITECTURE.md:122` ("RTL in `rtl/core/t81_top.sv` uses
2-bit binary representations per trit") and `EVIDENCE_AND_FINDINGS.md` §2.5
("Verilog RTL (`rtl/core/t81_top.sv` at `834521d4…`)"). There is no `rtl/`
directory at HEAD, at `f8ac771` (the docs' own 10-29 snapshot), or at
`f775680` (the 10-31 reference); the `834521d4` pin is UNVERIFIABLE from
this shallow clone. `SYSTEM_ARCHITECTURE.md:112` is internally inconsistent
on top of that: its Path column says `rtl/core/` while its Spec column says
"`t81-hardware` repo" (external).
Proposed fix (SYSTEM_ARCHITECTURE.md:122): "4. **Hardware Substrate
Disconnect:** Historical RTL (formerly cited as `rtl/core/t81_top.sv`, not
present in this repo at HEAD; hardware work lives in the separate
`t81-hardware` repo) used 2-bit binary representations per trit; no physical
3-state semiconductor characterization data exists in this repository." In
EVIDENCE §2.5, qualify the RTL sentence with "historical pin `834521d4…`,
not present at HEAD — verify against the `t81-hardware` repo before citing."

**M4 — Experiment citations name a file that never existed under that name.**
Evidence: `EVIDENCE_AND_FINDINGS.md:52` ("Source Pin: Commit
`f6d4d680…`, file `experiments/packed_ternary_matvec/benchmark.cpp`").
Commit `f6d4d680` verifies, but its tree for that directory contains
`CMakeLists.txt`, `main.cpp`, `matvec_experiment.hpp` — no `benchmark.cpp`.
Same error at `:61` for `experiments/radix_comparison/benchmark.cpp`
(actual: `main.cpp`, `radix_comparison.hpp`, `results.json`). The MatVec
numbers themselves are corroborated by the claims audit, so this is a
citation error, not a numbers error — except for the radix numbers, which
are covered by H1.
Proposed fix: replace `benchmark.cpp` with `main.cpp` in both pins, and in
§2.3 add "Numbers corroborated by `TERNARY_COMPARATIVE_CLAIMS_AUDIT.md`
(independent re-execution reported there)."

**M5 — The evidence table prints bare "Tier A" for five rows while the
pack's own latest sections downgrade historical Tier A to source-reported.**
Evidence: `EVIDENCE_AND_FINDINGS.md:17,19,20,25` (plus the Inv 08 row at
`:25`, which cites no source pin at all — violating the pack's own rule in
`DECISIONS_AND_GOVERNANCE.md` §5.2 that every assertion cite path, SHA, and
test command). The downgrade lives only in `START_HERE.md:136` ("Historical
Tier A entries in this package are source-reported until their receipts are
independently checked") and `EVIDENCE_AND_FINDINGS.md:148` ("not Tier A by
location, test-name, or successful historical prose"). A session reading
the table — the most likely path — never sees the qualifier.
Proposed fix (table header, EVIDENCE_AND_FINDINGS.md §1): add directly
above the table: "Tier labels in this table are the originating packets'
labels, NOT independent reproduction by this pack. Treat every Tier A row as
source-reported until its receipt is independently checked (see §6 and
START_HERE §11.5). Rows without a commit pin (notably Inv 08) must not be
cited as Tier A." Add a source pin to the Inv 08 row or demote it to Tier
B/C.

**M6 — Every file's header is stale: "Version 1.0.1 / Last Reviewed
2026-10-29" while all seven files contain 2026-10-31 sections.**
Evidence: `START_HERE.md:4-5`, and the identical 1.0.1/2026-10-29 headers on
RESEARCH_IDENTITY, SYSTEM_ARCHITECTURE, EVIDENCE_AND_FINDINGS,
DECISIONS_AND_GOVERNANCE, ACTIVE_RESEARCH_HANDOFF; `BOOTSTRAP.md` header
says "Snapshot: 2026-10-29" while carrying a 2026-10-31 overlay and an
incremental 10-31 note. Related: `START_HERE.md` §10 (2026-10-29) directs
sessions to HANDOFF §8 and DECISIONS §7 as "the current" status, but the
newer 10-31 sections (HANDOFF §9, DECISIONS §8) exist and §11 only partially
repairs the pointer.
Proposed fix: bump all seven headers to "Version: 1.0.2 · Last Reviewed:
2026-10-31" with the note "10-31 sections are overlays; older dated sections
are history, not current status." Append to START_HERE §10: "Superseded by
§11 (2026-10-31) for current status; the sections named here are the
2026-10-29 state."

**M7 — "The six files in docs/ai-context/" is now literally false.**
Evidence: `START_HERE.md:109`. The directory holds seven files; BOOTSTRAP.md
was added after the original six (the original commit `50693d7` message
lists files 00–05, six documents). The "six-document package + BOOTSTRAP
overlay" framing used in BOOTSTRAP itself and in START_HERE §11 is coherent,
but §9's flat "six files" sentence contradicts the directory a session can
see.
Proposed fix (START_HERE.md:109): "- The six core files in
`docs/ai-context/` plus `BOOTSTRAP.md` (fast orientation overlay) are a
living, non-normative research index."

### LOW

**L1 — CTest numbers in the maturity matrix are unverifiable and one test
name is wrong.**
Evidence: `SYSTEM_ARCHITECTURE.md:106-110` cites "CTest #391–396", "#154",
"#84–85", "#355". CTest numbers shift with test registration and cannot be
verified without configuring a build. The named targets do exist
(`t81_vm_determinism_property_test` in root `CMakeLists.txt:695`,
`t81_radix_comparison_test`, `t81_packed_ternary_matvec_oracle_test`), but
§4 writes `vm_determinism_property_test` without the `t81_` prefix.
Proposed fix: replace the numbers column with test names, e.g. "CTest:
`t81_vm_determinism_property_test` (number varies by configuration — use
the name, not the number)."

**L2 — Two referenced RFCs have no files.**
Evidence: `BOOTSTRAP.md` 10-31 overlay and `DECISIONS_AND_GOVERNANCE.md:8`
reference "proposed SDK compatibility transition RFC-00E9" and (§7) "draft
RFC-0057", but no `RFC-00E9*` or `RFC-0057*` file exists in `spec/rfcs/`.
These are proposals, so this is a dead-end lookup, not a false claim.
Proposed fix: append "(not yet drafted — no file in `spec/rfcs/` at HEAD)"
to each reference.

**L3 — BOOTSTRAP's snapshot provenance ("after PR #638 merged") is not
corroborated by local history.**
Evidence: BOOTSTRAP header: "`main` observed at `5fcc717e…` after PR #638
merged." Commit `5fcc717` verifies (a docs correction, single parent
`50693d7`), but no commit message in the 117-commit window mentions #638,
and neither nearby commit is a merge commit. A squash merge could explain
it, but the pack gives a session no way to confirm.
Proposed fix: change to "`main` observed at `5fcc717e…` (docs-correction
commit; PR #638 merge association not verifiable from local history —
recheck on GitHub)."

**L4 — Reproduction commands' binary paths are unverified.**
Evidence: EVIDENCE §3 steps 2–3 run
`./build/experiments/radix_comparison/t81_radix_comparison` and
`./build/experiments/packed_ternary_matvec/t81_packed_ternary_matvec`. Target
names verify in CMakeLists, but output paths were not verified (no build
was run in this audit).
Proposed fix: replace steps 2–3 with
`ctest -R 't81_radix_comparison_test|t81_packed_ternary_matvec_oracle_test'`
after the build step, with a note that binary paths vary by configuration.

**L5 — RFC-0030 impact line overstates what AD-002 itself qualifies.**
Evidence: `DECISIONS_AND_GOVERNANCE.md` §2: "Enables bit-exact
cross-platform float determinism across x86_64, ARM64, and WASM." AD-002
(same file) says acceptance "does not independently certify all CORDIC
functions or universal bit-exact replay across hosts."
Proposed fix: change the impact cell to "Accepted spec for bit-exact
cross-platform float determinism; does not independently certify all CORDIC
functions or universal replay (see AD-002)."

### Handoff effectiveness (the AI-to-AI test) — HIGH

The maintainer's purpose for this pack is a handoff test: a fresh AI reading
only `docs/ai-context/` should come away fully understanding T81 and then be
able to find real issues and PR them back. Judged against that purpose, the
pack teaches mission and hypotheses well but fails the second half of the
loop: it never tells the newcomer how to contribute back, never consolidates
current state in one place, and assumes vocabulary and tooling it never
defines.

**HND-1 — HIGH — The self-iteration loop has no final step: the pack never
documents how to open a PR.**
Evidence: across all seven files there is no branch procedure, no
commit-message convention, no PR template, no review process, and no approval
channel. The only adjacent lines are `DECISIONS_AND_GOVERNANCE.md:85` ("Run
`clang-format-18 -i` … before submitting changes") and §4.2's list of things
requiring human authorization — i.e. what NOT to do without approval, never
how to get approval or submit. The repo itself uses conventional-commit
titles (e.g. "docs(ai-context): …"), but the pack never states that
convention. HANDOFF §6's template is a session log, not a PR body. Why HIGH
for this purpose: the stated goal is "find real issues and PR them back." A
fresh AI that follows the pack perfectly still cannot answer: what branch do
I cut, what do I title the commit, what goes in the PR body, which CI must
be green, who approves? The loop dead-ends at the handoff's own goal.
Proposed fix (new section in START_HERE.md, after §8, in the pack's voice):
"## 8A. Contributing a finding back — 1. Cut a branch from fresh `main`;
keep the change to one finding or one subsystem. Title commits in the repo's
existing style (`<area>: <imperative summary>`, e.g. `docs(ai-context):
…`). 2. Every claim in the PR body cites an exact source path, commit SHA,
and command or test name — the same Tier standard as §5. A finding without
a pin is a draft, not a PR. 3. Run `clang-format-18 -i` on any modified
C/C++ file and the narrowest affected test target (see EVIDENCE §3) before
submitting. 4. State the maintainer gate explicitly: what needs a human
decision (DECISIONS §4.2) vs. what is a mechanical correction. Do not merge,
change branch protection, accept an RFC, or activate policy from a PR alone
— those require separate explicit maintainer approval."

**HND-2 — HIGH — There is no single current-state page; "the next action"
is stated in five places across three files.**
Evidence: `ACTIVE_RESEARCH_HANDOFF.md` §4 (10-29: reconcile H04-R02 + CMP3),
§8 "Next single authorized action" (10-29: validate the #577 head with the
27-command inventory), §9 "Bounded next actions" (10-31: three actions —
Jules archaeology, Codex ADM-2, maintainer decision); `BOOTSTRAP.md` "One
next action" (10-29: read-only H04/CMP3 reconciliation) plus its 10-31
overlay "Next bounded decision" (reconcile #653 + review ADM-2);
`START_HERE.md` §10 (10-29 pointers) vs §11 (10-31 pointers). The 10-31
sections do say older ones are history, but a newcomer must reconstruct
precedence across all seven files to answer "what do I do first?" — and the
two 10-31 statements themselves differ (HANDOFF §9's three actions vs
BOOTSTRAP overlay's one decision).
Proposed fix: rewrite START_HERE.md §11 as the single current-state box:
"## 11. Current state and next actions — 2026-10-31 (supersedes all earlier
'next action' sections). 1. **Archaeology (bounded):** independent #653
acceptance review — fix CLM-003/HISTORY inconsistency, eliminate path-based
Tier A, regenerate deterministically, add provenance regression tests. Stop
before merge. 2. **ADM-2 (bounded):** audit exact A/B diffs and test
receipts; run approved Linux/Windows checks. No publication without
maintainer authorization; worker-lifecycle repair is a separate task.
3. **Maintainer decision:** independent PR disposition for #651/#653 and
ADM-2 publication. No automatic RFC acceptance, activation, measurement, or
protection change. Older 'next action' sections (HANDOFF §§4, 8; BOOTSTRAP
'One next action'; §10 above) are dated history, not instructions." Add a
one-line supersession pointer at the top of each older section.

### Handoff effectiveness — MEDIUM

**HND-3 — MEDIUM — Load-bearing vocabulary is never defined in-pack.**
Evidence: terms used across the pack with no expansion or definition
anywhere in the seven files: CMP3 (workstream name; DECISIONS §3 gives
"EXP-T81-COMP3-01" but never expands it), ADM-2 (workstream; never
expanded), T1–T8 (BOOTSTRAP only: "production T1–T8 runner/attestation" —
stages never listed), PMU (never expanded; used as "PMU counters",
"PMU_UNAVAILABLE"), CLM- (claim IDs; the register exists but the prefix is
never defined), DAIOS (never expanded — the target architecture's name is
used dozens of times without its expansion), Baseline-X (the comparator
protocol's name; never defined as such), Protocol 001 (referenced, never
summarized), M1–M4/r1 (CMP3 milestones; never listed), CanonHash-81 (named
once in SYSTEM_ARCHITECTURE, never explained), FB/AB/FT/AT (four-arm labels;
a reader must infer fixed/adaptive × binary/ternary), Investigation 10A
(numbered, but the numbering scheme is never explained). DCP is the
exception — SYSTEM_ARCH §2.2 does expand "Deterministic Core Profile
(DCP)". Why MEDIUM, not LOW: a newcomer can follow the argument's shape but
cannot define what it is working on, and cannot distinguish (e.g.) "CMP3"
the workstream from "CMP3" the protocol version. For an AI-to-AI handoff,
every undefined acronym is a hallucination risk in the next session's
output.
Proposed fix: new glossary appendix in START_HERE.md, entries grounded only
in what the pack itself establishes; unknowns marked for the maintainer:
CMP3 = the EXP-T81-COMP3-01 decision-advantage protocol/harness workstream
(PRs #632–#637 and successors); ADM-2 = the engineering-recovery workstream
around PR #652 and local candidates A (baseline maintenance) and B (M1
header isolation); PMU = hardware performance counters; trusted measurement
requires them (see HANDOFF §8: "MEASUREMENT BLOCKED — PMU_UNAVAILABLE");
CLM- = claim identifiers in `experiments/research_001/CLAIM_REGISTER.md`;
FB / AB / FT / AT = the H04-R02 four arms: fixed binary, adaptive binary,
fixed ternary, adaptive ternary (in corrected-cost statements, AT/AB are
the adaptive-ternary / adaptive-binary modeled costs); Baseline-X = the
planned conventional-comparator protocol (Wasmtime/OPA arms) against which
governed execution is compared; not yet fully run; Protocol 001 = the
research protocol specifying that comparator; see
`experiments/research_001/RESEARCH_PROPOSAL.md`; DAIOS, T1–T8, M1–M4/r1,
CanonHash-81, Investigation 10A numbering = maintainer to define in one
line each — currently used but undefined in this pack.

**HND-4 — MEDIUM — Verification instructions assume GitHub access the pack
never declares as a prerequisite.**
Evidence: `START_HERE.md` §6 tells sessions to "Verify git branch heads
using `git log` or `git status`," and the pack's PR-head discipline requires
checking live PR heads. But PR heads are not in a local clone — verified:
every PR head cited in the pack (#634, #635, #637, #649–#653, #577) is
absent from this clone — and the pack nowhere states that GitHub access
(web or `gh` CLI) is required, nor what to do without it. The only mention
of how a past update got PR state is a passing note in HANDOFF §8's
evidence-tier line ("inspected through the GitHub connector"). A fresh AI
with only a local clone will silently fail the pack's central verification
instruction, or worse, treat the absence of a PR head locally as evidence
the PR doesn't exist.
Proposed fix (append to START_HERE.md §6): "- **PR heads are not in a local
clone.** Checking a PR head requires GitHub access (web or `gh` CLI). If
you have only a local clone, mark every PR state UNVERIFIED — never report
a PR as open, closed, or merged from local absence alone, and never treat a
cited PR SHA as current without a live check."

**HND-5 — MEDIUM — No general "verify your change" path: only two
experiment commands exist, and no test invocation is given anywhere.**
Evidence: `EVIDENCE_AND_FINDINGS.md` §3 is the pack's only command list
(build two experiment targets, run two experiment binaries, one oracle
script, one regression script). No `ctest` invocation appears anywhere in
the seven files; the only test guidance is START_HERE §6's generic "run the
corresponding CTest target" and SYSTEM_ARCH §4's unstable CTest numbers
(L1). A newcomer who fixes a doc path or a subsystem bug cannot answer "how
do I prove my change is good?" from the pack alone.
Proposed fix (append to EVIDENCE_AND_FINDINGS.md §3): "## 3A. Verifying a
change (general). Build with the command in step 1, then run the narrowest
affected test by name, e.g. `ctest -R
't81_radix_comparison_test|t81_packed_ternary_matvec_oracle_test|t81_vm_determinism_property_test'`
from the build directory. CTest numbers vary by configuration — always cite
test names, never numbers. A full-suite run is `ctest` from the build
directory; record failures verbatim rather than rerunning until green."

**HND-6 — MEDIUM — Authorization for reproduction runs is ambiguous.**
Evidence: `EVIDENCE_AND_FINDINGS.md` §3 presents benchmark runs as
"suggested reproduction commands," inviting a session to run them. But
`BOOTSTRAP.md` "Stop:" says "Do not run performance experiments," and
HANDOFF §4 lists "New benchmark execution" under "Not authorized by this
index." A newcomer cannot tell whether running the already-existing
`t81_radix_comparison` binary is a permitted reproduction or a prohibited
new measurement.
Proposed fix (prepend to EVIDENCE_AND_FINDINGS.md §3): "Reproducing an
already-pinned benchmark binary on its documented workload is permitted
verification. Collecting a NEW performance measurement, or using a run to
support a new performance claim, requires separate maintainer authorization
and PMU/trusted-runner availability (see BOOTSTRAP Stop conditions and
HANDOFF §4). Never present a reproduction run as a new finding."

### Handoff effectiveness — LOW

**HND-7 — LOW — "Do not do" rules are scattered across four files with
different scopes.**
Evidence: START_HERE §7 (claims), DECISIONS §4.2 (approvals), BOOTSTRAP
"Stop:" (operations), HANDOFF §4 stop criteria (task scope). A newcomer can
easily honor one list while violating another (e.g. obeying the claims list
while missing that performance measurement needs PMU authorization).
Proposed fix: add a "Hard stops (summary — details govern)" box to
START_HERE §7 listing all four, each pointing to its source section. LOW
because the rules themselves exist and are consistent; only their
discoverability is weak.

### Handoff strengths (for balance)

- Mission and hypothesis coverage is genuinely complete from the pack alone:
  START_HERE §§1–2 and RESEARCH_IDENTITY §§1, 3 give the central question,
  the three tracks, and all eight hypotheses with formal claims, baselines,
  falsifiers, results, and next actions — a newcomer can answer "what is T81
  and what does it believe?" without leaving the pack.
- The evidence-tier system (START_HERE §5) and the prohibited-claims list
  (§7) are the pack's best handoff features: explicit, checkable, and
  consistently applied in the 10-31 corrections.
- The append-only self-correction practice (HANDOFF §7 correction note,
  EVIDENCE §6, START_HERE §11) models exactly the behavior the maintainer
  wants from the next AI: preserve history, date corrections, never silently
  rewrite.

## Phase 2 findings — full repo

### Top 5 findings (all HIGH)

**1. JIT arithmetic diverges from the interpreter (§A-H1).**
`vm/jit/jit_compiler.cpp:165–235` uses raw `int64_t` arithmetic with no tag
guard and no overflow check, while the interpreter routes through
arbitrary-precision `T81BigInt`. A loop accumulating past `INT64_MAX` wraps
(signed-overflow UB) under JIT but is correct under the interpreter;
`INT64_MIN / -1` is UB under JIT; a `BigIntHandle` operand gets its
*handle* added and tagged `Int`. Results depend on execution history (hot
vs cold), directly breaking bit-exact determinism.
Fix: guard traces to `Int`-only operands with deopt on any other tag, use
checked arithmetic with deopt on overflow and `MIN/-1`, or disable JIT for
arithmetic ops until then.

**2. A program-embedded policy silently displaces a host-supplied Axion
engine (§B-H1).**
`vm/vm.cpp:382–386`: on `load_program`, if the program carries
`axion_policy_text`, the VM parses it and *replaces* the host's engine. Any
embedder relying on `make_interpreter_vm(host_engine)` as a security
boundary loses it the moment a program carries its own policy text — no
warning, intersection, or failure.
Fix: host engine must win — reject programs carrying policy text when a
host engine was supplied, or intersect (deny if either denies).

**3. Loop-hint "evidence" is manufactured from the policy being checked
(§B-H2).**
`vm/vm.cpp:387–404` seeds `state_.axion_log` at load time with `Allow`
events built from the policy's own loop hints, in exactly the format
`PolicyEngine` expects; `vm/policy_trace_bridge.cpp` copies them verbatim
into trace reasons, and the engine accepts substring matches. A loop
requirement is satisfied from load time onward whether or not any loop
executed.
Fix: never seed evidence from the policy under evaluation; emit loop-hint
events only from actual loop execution, matched on structured fields, not
substrings.

**4. Guest program can raise its own FFI resource quotas (§A-H3).**
`vm/vm.cpp:6978` (`FFIPolicySet`) → `vm/ffi_dispatcher.cpp:319–343` sets
time/memory quotas from guest-supplied values with no Axion evaluation, no
privilege check, and no host ceiling — a guest can raise quotas to
`UINT64_MAX`. (`FFICall` itself is properly gated; this is specifically
the policy-set path.)
Fix: remove guest reachability of quota-setting, or gate it through
`eval_axion_call` with Deny → `Trap::SecurityFault` and clamp to a
host-set ceiling.

**5. The advertised frozen ISA version does not exist, and the contract
contradicts the registry and the VM on opcodes 174–255 (§C-H1/H2).**
`README.md:6,89` advertises "ISA v1.9.0 Frozen" linking to
`spec/tisc/opcode-registry.md`, which says v1.1.0; no v1.9.0 ISA document
exists. Worse, `spec/tisc/TISC_CAPABILITY_CONTRACT_v1.1.0.md:20,54–55`
says opcodes 174–255 MUST trap, while `spec/tisc/opcode-registry.md:13`
assigns 174–199 and `vm/vm.cpp:5691,5743,5818` dispatches ATTN/QMATMUL/EMBED
in exactly that range.
Fix: correct the README to v1.1.0 (or disambiguate runtime vs ISA version
with a citation), and amend the contract to carve out 174–199 (resolving
`FFICall`/200) or downgrade the registry entries to draft — one document
must be the tie-breaker.

### A. Core code inspection (ternary arithmetic, VM determinism, execution/policy)

`kernel/axion/` is real code (`engine.cpp`, `policy_engine.cpp`,
`policy_validator.cpp`, `ethics.cpp`, `ai_hooks.cpp`, `canonfs_hook.cpp`,
`shell/`, `userenv/`); `.md` files and `policies/*.axp` there are docs/data,
not gates.

**A-H1 — HIGH — JIT arithmetic diverges from the interpreter.** See Top
finding 1. Related JIT gaps in the same file: `Load`/`Store`
(`vm/jit/jit_compiler.cpp:424–446`) use a weaker bounds check
(`addr < memory.size()`) than the interpreter's segment-membership check
(`vm/memory_segments.cpp:mem_ok`), and JIT `Push` ignores the policy
`max_stack` check the interpreter applies — align those or deopt on them
too. (Sound parts: the JIT policy hook is correctly placed *before* each
instruction at `jit_compiler.cpp:157`, and Axion opcodes correctly exit via
`AxionBoundary`.)

**A-H2 — HIGH — Binary encoding silently truncates `Insn.b` from 64 to 32
bits.**
Evidence: `isa/encoding.cpp:12–19`: the `append` lambda takes
`std::int32_t`, so `append(insn.b)` (`Insn.b` is `std::int64_t`,
`include/t81/isa/program.hpp`) truncates; `decode` (`:37–44`) reads back
only 32 bits. Any `b` outside `int32` range round-trips to a different
instruction with no error. Live path: `tools/tisc_base81.cpp:30–52`
encode/decode plus determinism tests; existing tests only use small `b`, so
they pass.
Fix: serialize `b` as 8 bytes (format version bump or new encoding) and
make `decode` reject the old layout or handle both explicitly. Note the
encoder also drops pools and `literal_kind` entirely — if full-`Program`
round-trip is intended, that is a second loss channel; if insns-only is
intended, document it on `encoding.hpp`.

**A-H3 — HIGH — Guest program can raise its own FFI resource quotas.** See
Top finding 4.

**A-M1 — MEDIUM — Axion policy bytecode interpreter reads past the buffer
on truncated input.**
Evidence: `kernel/axion/policy_engine.cpp:41–64`: `read_u32`/`read_u64` and
single-byte reads in `RequireLoop` (`:124–131`) / `RequireSegmentEvent`
(`:181+`) dereference `pc` with no end check; the loop condition only
checks `pc < end` before reading the opcode. A truncated `.axp`/bytecode
blob reads out-of-bounds memory (UB; can crash and can steer a limit/symbol
check). Unknown opcodes correctly fail closed (`:211` Deny), so this is the
one fail-open hole in that interpreter. *Same defect as §B-M7; counted
once.*
Fix: bounds-check every read (`remaining() >= n`, else return
`Deny "truncated policy bytecode"`).

**A-M2 — MEDIUM — `T81Int::to_string()` returns base-3 digits, not
decimal, and has an `INT64_MIN` UB path.**
Evidence: `include/t81/types/T81Int.hpp:476–490` converts via repeated `% 3`
and emits `'0'/'1'/'2'`, while `operator<<` on the same type prints decimal
`to_int64()`. So `T81Int(5).to_string() == "12"` but `<<` gives `"5"`.
Observable in the VM: `vm/vm.cpp:1368` formats fractions as
`num.to_string() + "/" + den.to_string()` (`T81Int<81>`,
`include/t81/types/T81Fraction.hpp:27–33`), so 10/3 renders as `101/10t81`.
Also `:479` does `v = -v` after `to_int64()`, UB at exactly `INT64_MIN`
(reachable for `N ≥ 41`).
Fix: make `to_string()` decimal (handling `INT64_MIN` via unsigned
magnitude), or rename it `to_base3_string()` and fix the VM fraction
formatter to use decimal.

**A-M3 — MEDIUM — Base81-view complex-pool round-trip is broken.**
Evidence: `isa/base81_view.cpp`: render writes complexes via
`T81Float::to_canonical_string()` (`"+<mantissa trits>E<exp>"`,
`include/t81/types/T81Float.hpp:478–504`), but parse
(`base81_view.cpp:215–226`) feeds that string to `std::stod` as a fallback.
`stod` stops at the first `-` inside the mantissa and misreads the rest, so
a mantissa containing `-` truncates silently to a wrong double; `NaE`/`±Inf`
are special-cased but ordinary values are not recovered.
Fix: implement a real `from_canonical_string` (sign, trit mantissa, decimal
exponent) and return a parse error for unparseable values instead of
silently substituting.

**A-M4 — MEDIUM — Base81-view parse throws instead of returning `Result`,
and silently corrupts other fields.**
Evidence: `isa/base81_view.cpp`: `std::stod` (`:195`) and `std::stoi`
(`:233,244`) throw out of a function whose contract is `Result<Program>`;
symbol hex decoding (`:197–208`) ignores `from_chars` errors and drops a
trailing odd nibble; tensor pool restores only `exact_int`/`exact_trit`
classes and silently drops others, with no shape/data-length validation.
Fix: validate each field and return `T81Error` on failure (hex: require
even length and checked `from_chars`; floats/shapes/tensors: catch and
convert to errors; reject unknown tensor classes or preserve them).

**A-M5 — MEDIUM — BigInt bitwise ops let an overflow exception escape
`step()` instead of becoming a Trap.**
Evidence: `vm/vm.cpp:1465–1466,1514,1548–1549` (`handle_bitwise_binary` /
`handle_bitwise_not` / `handle_bitwise_shift`): a `BigIntHandle` operand
converts with `to_int64()`, which throws (`trap_overflow`,
`include/t81/axion/api.hpp:144`) for out-of-range values. No `try/catch`
around these handlers or at the top of `step()`/`run_to_halt()`
(`vm/vm.cpp:435,7248`), so a guest `BitAnd` on a >int64 BigInt aborts the
run with an exception rather than a `Trap` (contrast `FracAdd`, which
catches into `Trap::DecodeFault` at `:3940–3970`).
Fix: check `maybe_int64()` first and return `Trap::DecodeFault` (or a
dedicated overflow trap) when absent, or wrap the handlers in `try/catch`.

**A-M6 — MEDIUM — Determinism detector uses implementation-defined hashing,
and exists in two divergent definitions.**
Evidence: `vm/vm.cpp:226–235` (`DeterminismDetector::hash_event`) mixes
`std::hash<std::string>` / `std::hash<std::string_view>`, not guaranteed
stable across standard libraries/platforms — so a cross-platform
"deterministic" verdict rests on a non-portable hash. Worse,
`kernel/axion/nondeterminism_detector.cpp:10–22` defines the *same* member
again, hashing only opcode/tag/verdict-kind/reason — omitting `value`,
`pc`, and all structured fields. Which definition wins depends on
archive/link selection (no built artifacts were present to check with
`nm`); a binary using only the kernel version would report "no divergence"
for runs differing in values or PCs.
Fix: delete one definition, and hash with the fixed-key `canon_hash81`
(`vm/determinism/canon_hash81.cpp`, correctly deterministic) over a
canonical serialization of all event fields.

**A-M7 — MEDIUM — Step/JIT policy handling fails open for non-Deny
verdicts.**
Evidence: `vm/vm.cpp:616–625`: the per-instruction step check blocks only
`Deny`; `Warn` is logged and execution continues, and `Defer`/`Quarantine`
fall through to execution. The JIT callback (`vm/vm.cpp:509–517`) likewise
returns false only on `Deny`. `VerdictKind::Quarantine` is documented as
"thread is suspended, RD not committed (SecurityFault)"
(`include/t81/axion/verdict.hpp:10`), and the one place Quarantine is
produced (TACT ceiling, `vm/vm.cpp:6860–6872`) correctly withholds the
result and traps — so the step path is inconsistent with that semantics.
Caveat: shipped engines currently only return Allow/Deny/Warn, so this is
latent for custom engines, not triggerable today via stock policy.
Fix: treat any verdict that is not explicit `Allow`/`Warn` as blocking at
the step/JIT boundary, or document that only Deny is blocking and remove
Quarantine from the step verdict vocabulary.

**A-L1 — LOW — `value_ops` helpers are unchecked signed arithmetic
(currently dead code).**
Evidence: `vm/value_ops.cpp:5–20`: `add_int`/`sub_int`/`mul_int`/`neg_int`
are plain signed ops (overflow UB); `div_int`/`mod_int` check only
`rhs == 0`, missing `INT64_MIN / -1`. No callers were found in `vm/`,
`src/`, or `include/` — latent, not live.
Fix: use `__builtin_*_overflow` + the `MIN/-1` check, or delete the file if
it stays unused.

**A-L2 — LOW — Ternary `decode_i64` clamps instead of erroring,
inconsistently with `T81Int`.**
Evidence: `include/t81/ternary/arith.hpp:48–70`: on overflow it returns
`INT64_MAX`/`INT64_MIN` silently, and the `power > INT64_MAX/3` branch keys
the clamp direction on the *current* trit's sign rather than the accumulated
result's sign. `T81Int::to_int64()` throws on the same condition.
Fix: return `std::optional`/`expected` or throw, matching `T81Int`.

**A-L3 — LOW — FFI audit trail is non-reproducible across runs.**
Evidence: `ffi/ffi_dispatcher.cpp:240–241,429`: audit events embed wall-clock
`execution_time_ns`; `vm/ffi_dispatcher.cpp:265–271` uses a process-global
`static` call-id counter that also feeds `provenance_hash`
(`ffi/ffi_dispatcher.cpp:394`) and cache keys (`:448`). Two identical VM
runs in one host process therefore produce different provenance/audit output.
Fix: reset the counter per VM run (or derive call identity from program
state), and exclude wall-clock duration from any hash/audit artifact
claimed reproducible.

**A-L4 — LOW — Second base-81 packing is silently incompatible and
unvalidated.**
Evidence: `isa/math/quantization/ternary_codec.cpp:74–97`
(`unpack_base81_to_ternary`) accepts bytes > 80 with no error, and its
packing is MSB-first (`trit[0]*27+…`), whereas
`codec/trit_packing.cpp:119–165` (`pack_base81`) is LSB-first
(`trit[0]*1+…`). Both call themselves base-81 packing of 4 trits; any path
mixing them corrupts data. (No cross-calls were found — each is
self-consistent within its own tests — so this is a consistency trap, not a
live corruption.) Similarly `codec/base81_packed.cpp:22–44`
(`pack5`/`unpack5`) validates neither digit ≤ 80 nor packed ≤ max
(3,486,784,400).
Fix: reject bytes > 80, and document/canonicalize a single base-81 trit
packing (or rename one explicitly as MSB-first).

**A-NC1 — NEEDS CHECK — TLoadHash empty-payload allowlist skip.**
Evidence: `kernel/axion/policy_engine.cpp:257–275`: when
`allowed_tensor_hashes` is non-empty but `ctx.payload` is empty, the
membership check is skipped and evaluation falls through to Allow. The VM
handler (`vm/vm.cpp:1850–1900`) passes the symbol text and validates hash
format / CanonFS lookup, so VM exploitability is unclear. *Same issue as
§B-M1; the §B-M1 verdict governs (confirmed library-API bypass; VM path
not affected).*
Next check: confirm `load_canon_tensor_by_hash("")` can never return a
tensor (no wildcard/default entry); if it can, this becomes a policy
bypass — fix by denying empty payload when an allowlist is configured.

**A-NC2 — NEEDS CHECK — `AxVerify` Defer semantics.**
Evidence: `vm/vm.cpp:7499`: `registers[a] = (verdict == Defer) ? 1 : 0`, so
Allow and Warn both yield 0.
Next check: confirm against the AxVerify spec what 0/1 mean — if 1 means
"verified", Defer is being treated as success.

**A-NC3 — NEEDS CHECK — Duplicate `DeterminismDetector` link behavior.**
See A-M6.
Next check: build a binary linking both `t81_vm` and `t81_axion` and
inspect with `nm` which `hash_event`/`record_run` definition is used; do not
assume a link error.

**A-NC4 — NEEDS CHECK — Final-instruction-only policy requirements vs
earlier side effects.**
Evidence: `kernel/axion/policy_engine.cpp:330–353` enforces
match-guard/segment/axion/alignment requirements only when
`final_authorization || next_opcode == Halt`. A program that performs
memory/FFI effects and then halts without satisfying them gets a
`SecurityFault` at Halt (`vm/vm.cpp:582–585`), but earlier effects are not
rolled back.
Next check: decide whether those requirements are meant as end-of-run
audit (acceptable) or as gates (then they need per-effect checks or
rollback); per-opcode gates that do exist (TLoadHash, FFI call, TACT
ceiling) are correctly placed before the effect.

**A-NC5 — NEEDS CHECK — Base-81/Base-243 leading-zero loss.**
Evidence: `codec/base81.cpp:encode_bytes` /
`codec/base243.cpp:encode_bytes_be` treat input as a big integer, so leading
zero bytes are lost on round-trip. The base-81 test explicitly documents
this as canonical-integer behavior
(`tests/cpp/codec_base81_test.cpp:19`), so it is only a bug if any caller
treats these as byte-string codecs.
Next check: grep production callers for byte-string (vs integer) use; if
any, add length prefixing.

**Checked and found sound (core code):** `T81Int` core arithmetic
(balanced-ternary add carry logic, `assign_from_int64` including the
`INT64_MIN` magnitude path, negation symmetry, `to_int64()` trit-40 special
cases, `div_mod` base-3 long division); `T81Fraction::canonicalize`
(normalizes zero denominators to 0/1 rather than dividing by zero);
`codec/base81_balanced.cpp` pack/unpack and `codec/base81.cpp`/`base243.cpp`
digit conversions; VM integer path (non-JIT) uses `T81BigInt` with explicit
truncation-toward-zero Div/Mod and zero-division traps; float
transcendentals use the deterministic soft-math path when
`T81_DETERMINISTIC` is defined (set globally for CMake builds by
`cmake/Compiler.cmake:8` — non-CMake builds should verify the define);
decoder `unordered_map`s are lookup-only (no iteration-order
nondeterminism); `hot_spots_`/`compiled_traces_` are `find`-only;
`vm/determinism/canon_hash81.cpp` uses a fixed SipHash key. Policy gates
that do work: per-step Deny → `SecurityFault`, TLoadHash pre-load check,
FFI-call capability/policy check, TACT activation-ceiling Quarantine
withholding the result, GC-cycle veto before the cycle.

### B. Security-sensitive surfaces (obvious issues only, no exploit development)

**B-H1 — HIGH — A program-embedded policy silently displaces a
host-supplied Axion engine.** See Top finding 2. Additional evidence:
`vm/vm.cpp:283–286` (the engine passed to `make_interpreter_vm(engine)` is
the host's control point); `include/t81/vm/vm.hpp:75–76` (factory
argument). In-tree CLI callers pass no engine, so this bites host
embeddings, not the current CLI flow — but the API invites exactly that
composition.
Fix: host engine must win — reject programs carrying policy text when a
host engine was supplied, or intersect program policy with host policy
(deny if either denies); at minimum, log/fail loudly when both are present.

**B-H2 — HIGH — Loop-hint "evidence" is manufactured from the policy being
checked.** See Top finding 3. Additional evidence:
`vm/policy_trace_bridge.cpp` (copies every `axion_log` reason verbatim
into `SyscallContext.trace_reasons`); `PolicyEngine::evaluate_internal` /
`loop_hint_satisfied` (accept any substring match). A program claiming
`bound=10` for an unbounded loop passes the loop check.
Fix: never seed the evidence log from the policy under evaluation; emit
loop-hint events only from actual loop execution (opcode/pc-anchored),
matched on structured fields (event type + file/line/bound), not substrings.
General fix for all final-evidence checks: evidence must be structured,
authenticated events, not strings any program-influenced log line can
contain.

**B-M1 — MEDIUM — Empty-payload TLOADHASH skips the tensor whitelist
entirely (library API).**
Evidence: `kernel/axion/policy_engine.cpp`, `evaluate_internal`: the
whitelist loop runs only `if (!ctx.payload.empty())`, with the comment
"Allow logic continues below…" — an empty payload falls through to
limits/bytecode and can return `Allow`. The bytecode opcode set has no
hash-check op, so a bytecode policy cannot close this either. Consequence:
any direct `PolicyEngine` caller presenting an empty hash (e.g.
`load_model_via_tloadhash` in `kernel/axion/ai_model_loader.cpp:20–31`
with an empty `hash` argument) loads without any whitelist check. The VM's
own TLOADHASH handler is *not* affected — it re-evaluates with the real
`hash_str` before loading (`vm/vm.cpp:1863–1883`). *Same issue as §A-NC1;
confirmed here as a library-API bypass, and this verdict governs.*
Fix: deny TLOADHASH whenever `ctx.payload` is empty, regardless of the
allowed list.

**B-M2 — MEDIUM — Fail-open defaults: no policy ⇒ allow, and MetaWrite
without policy is only a warning.**
Evidence: `kernel/axion/policy_engine.cpp`, `evaluate_internal`: with no
policy, everything except `final_authorization` and TLOADHASH returns
`Allow`. `kernel/axion/ethics.cpp` Theta-9: MetaWrite without a policy
returns `Warn`, and `PolicyEngine::evaluate` returns a warning only when
the verdict is otherwise `Allow` — so the write proceeds.
`kernel/axion/axion_api.cpp:8,17` and `vm/vm.cpp:285–286`: the default
`AxionContext`/interpreter engine is allow-all. Consequence: a VM or
backend constructed without an engine, or a program carrying no policy
text, runs unrestricted — including meta-memory writes the code itself
calls "highly risky." This is a design default, not a bug in one call
site, but it means "no policy configured" and "policy allows" are
indistinguishable downstream.
Fix: make the default deny (or require an explicit opt-in `AllowAll`
construction with a distinct marker), and promote Theta-9
MetaWrite-without-policy from `Warn` to `Deny`. Positive counterexample:
`vm/ai_backend/controlled_ai_backend.cpp:171–186` is correctly fail-closed
(missing engine ⇒ false; authorization requires an active policy *and*
`Allow`).

**B-M3 — MEDIUM — BlockBackedDriver skips the Axion hook for capability
operations and enforces no capabilities at all.**
Evidence: `fs/block_backed_driver.cpp`: `write_object`/`read_object_bytes`
call `axion_allow`, but `publish_capability` and `revoke_capability` never
call it, and neither read nor write consults the `caps_` map (only
bookkeeping for publish). Contrast `fs/in_memory_driver.cpp` and
`fs/persistent_driver.cpp`, which gate all four operations. Consequence: on
the block-backed driver, anyone who can call the driver can grant
themselves any capability or revoke anyone else's, and can read/write any
object regardless of grants — the capability system is simply absent on
this driver.
Fix: gate publish/revoke through `axion_allow` like the other drivers, and
check `caps_` (including expiry) on read/write, or document that this
driver is intentionally capability-free and remove the misleading
capability API from it.

**B-M4 — MEDIUM — Capability bootstrap + unenforced grant semantics across
drivers.**
Evidence: `fs/in_memory_driver.cpp` (`has_capability`: `if
(capabilities_.empty()) return true`) and `fs/persistent_driver.cpp`
(`if (!has_capabilities_) return true`): before the first capability
exists, all access is allowed. `publish_capability` in both drivers
overwrites any existing grant without checking that the caller holds grant
rights. `fs/persistent_driver.cpp` `write_capability` persists only the
perms integer — `expires_at` (and `granted_by`/`revocable_by`) are dropped,
so expiry is never enforced there; in the block driver expiry is serialized
(`fs/block_backed_driver.cpp`, 106-byte layout) but never checked.
Consequence: a fresh store is fully open by default, and once grants exist,
any caller able to invoke publish can self-grant, with no expiry —
capabilities are advisory labels, not enforcement, unless an Axion hook
independently restricts the operation.
Fix: remove the empty-store bootstrap allow (or gate it behind an explicit
bootstrap flag cleared after first grant); require a granter capability on
publish; persist and enforce `expires_at` on every read/write.

**B-M5 — MEDIUM — CanonFS read verification can be switched off by
environment, and VM tensor loads trust the driver.**
Evidence: `fs/in_memory_driver.cpp` and `fs/persistent_driver.cpp` define
`read_verify_enabled()` from `T81_CANONFS_READ_VERIFY` (`0/false/off` ⇒ no
verification). `vm/tensor_helpers.cpp:1348+`
(`load_canon_tensor_by_hash`) trusts `driver.read_object_bytes` and only
decodes — it does not re-hash. The TLOADHASH policy check validates the
*requested hash string* against the whitelist, not the bytes actually
returned. Consequence: with the env var set (operator-controlled, but a
single ambient variable with no warning), a tampered object file on the
persistent driver loads as a whitelisted tensor: the whitelist attests to
the name, the bytes are unchecked. Mitigations that bound this: the
interchange loader re-hashes after read
(`fs/canonfs_interchange_ops.cpp:137–143`) and `CanonStore::get` re-verifies
(`ternaryos/dev/canon_store.cpp:116`) — but neither covers the VM
persistent-driver tensor path.
Fix: remove the env kill-switch (or require an explicit, logged,
per-process opt-out that cannot be set silently), and re-hash inside
`load_canon_tensor_by_hash` before decoding. Also fix
`fs/in_memory_driver.cpp` `parity_repair_subtree`: it stores RS-recovered
bytes under `ref.hash` without verifying the recovered hash matches — a
corrupt repair reports success; verify before storing.

**B-M6 — MEDIUM — `verify_deterministic_replay` can never pass for a
generated decision, and its hash doesn't cover the decision.**
Evidence: `fs/canonical_optimization_decision.cpp:349–350` computes
`deterministic_hash` over `to_canonical_json()` while
`deterministic_hash` is still empty; lines 352–368 then add
`preconditions`, `expected_outcomes`, `rollback_strategy`, and
`feature_importance` *after* hashing. `verify_deterministic_replay` (lines
489–500) recomputes over the full JSON including the stored hash and the
later fields, so recomputed ≠ stored for every genuinely generated
decision. Separately, line 368 sets `training_data_hash =
compute_content_hash("training_data_v1.0")` — a constant, not a fingerprint
of any training data. Consequence: the replay-integrity check is dead in
the fail-closed direction (it rejects valid decisions, so anything relying
on it either never passes or gets ignored), and tampering with
preconditions/outcomes/rollback would not have been caught by the stored
hash even if verification worked. Provenance claims about training data
are false as written.
Fix: hash a canonical form that excludes the hash field itself and is
computed *after* all fields are final (hash-then-set pattern with a fixed
field ordering); compute `training_data_hash` from actual training-data
bytes or drop the claim.

**B-M7 — MEDIUM — Policy bytecode interpreter reads operands without
bounds checks (heap over-read).**
Evidence: `kernel/axion/policy_engine.cpp`, `execute_bytecode`:
`read_u32`/`read_u64` dereference `pc` 4–8 bytes at a time and `*pc++` is
used for flag bytes, with the loop condition only `pc < end`. A truncated
bytecode ending mid-operand reads past the vector's buffer (UB; adjacent
heap bytes become limits, symbol indices, or bound values). Unknown opcodes
do deny, and truncated operands are the hole, not unknown opcodes.
`get_sym` itself is bounds-safe (returns `""`). *Same defect as §A-M1;
counted once.*
Fix: bounds-check every read (return `Deny` on truncation) and reject
trailing partial instructions instead of falling through to `Allow` at
end-of-bytecode.

**B-M8 — MEDIUM — CanonFS export follows symlinks on materialization.**
Evidence: `fs/canonfs_interchange_ops.cpp`, `export_ref` manifest branch:
per-entry policy check and `is_safe_relative_export_path` (rejects absolute
paths and `..` components) run before writing, but the write itself is
`std::ofstream(target_file, trunc)` with no symlink check — import, by
contrast, explicitly rejects symlinks (`collect_import_entries`). A
pre-existing symlink at `output_path/<entry>` or in an intermediate
directory is followed. Consequence: exporting a manifest into a
pre-existing directory tree containing a planted symlink writes
attacker-chosen CanonFS content outside the output directory (file
overwrite via truncation). The manifest's content is hash-bound, so this
needs a victim exporting into a prepared directory — realistic for
interchange workflows.
Fix: refuse to write through symlinks (check `is_symlink` on the target
and each created parent component, or open with `O_NOFOLLOW`/`O_EXCL`
semantics) and fail the entry instead of following.

**B-L1 — LOW — Object type is excluded from CanonFS identity.**
Evidence: all three drivers ignore the `ObjectType` argument to
`write_object` (unnamed parameter); `CanonRef` carries only the hash
(`include/t81/canonfs/canon_types.hpp`), and the hash is over raw bytes
only. Consequence: identical bytes written as `RawBlock`, `CanonTensor`,
`CanonExec`, etc. share one identity. A ref obtained for a benign object
type is indistinguishable from the same bytes interpreted as another type;
the only gate is each decoder's format validation.
Fix: domain-separate the hash (prefix the object type into the hashed
bytes) or carry the type in the ref and enforce it at decode/load time.

**B-L2 — LOW — The "sha3-256:" label is truncated SHA3-512, not SHA3-256.**
Evidence: `vm/tracing/canonhash81.cpp`: `CanonHash81` = first 32 bytes of
`sha3_512`. `fs/canonfs_interchange_ops.cpp:57` and
`tools/cli/ai/ai_cli_shared.cpp:325` label this `sha3-256:`. Internally
consistent (all in-tree consumers use the same construction), but it is
not FIPS 202 SHA3-256 (different rate/capacity), so any external verifier
computing real SHA3-256 will mismatch, and the label invites exactly that
cross-check.
Fix: rename the label to something accurate (e.g.
`t81-sha3-512-trunc256:`) or implement true SHA3-256 (rate 108) and
migrate refs explicitly.

**B-L3 — LOW — SHA3-512 itself looks correct; two latent issues only.**
Evidence: `crypto/sha3.cpp` uses rate 72 (correct for SHA3-512), suffix
`0x06` + final `0x80` (correct SHA3 padding, distinct from Keccak `0x01`),
the standard 24 round constants, and standard rotation/permutation tables;
squeeze produces the full 64 bytes. The KAT vectors in
`tests/cpp/test_sha3.cpp` (empty, `abc`, fox, multi-block) are the correct
FIPS 202 values. The only other crypto in-tree is OpenSSL SHA-256 in
`fs/canonical_optimization_decision.cpp` (standard library call, no custom
construction). Two latent notes: (a) `load64`/`store64` use `memcpy` with
no little-endian conversion, so a big-endian host would silently produce
nonstandard digests (all currently supported targets are little-endian —
low); (b) `vm/jit/ai_jit_optimizer.cpp:442` calls
`t81::crypto::sha3_256_hex`, which is not declared in
`include/t81/crypto/sha3.hpp` — but that file is not in
`src/vm/CMakeLists.txt` (only `jit_compiler.cpp`/`jit_trace_cache.cpp`
are), so this is dead code that would fail to build if wired in, not a
live defect.
Fix (b) by deleting the call or implementing the function before ever
adding the file to the build.

**B-L4 — LOW — Manifest parsing gaps (export).**
Evidence: `fs/canonfs_interchange.cpp:465+` (`parse_manifest`): no
duplicate-path rejection (a later entry silently overwrites an earlier one
at the same path), and `entry.size_bytes` is parsed but never compared
against the actually loaded bytes in `export_ref`. Hand-rolled JSON
scanning (`find('}')`/`find(']')`) is fragile for paths containing those
characters, though observed failure modes trend fail-closed (missing-field
rejection).
Fix: reject duplicate paths, verify `size_bytes` against loaded size, and
prefer a real JSON parser for manifests.

**B-L5 — LOW — Simulated shell gate is deny-list-only and builtins bypass
it.**
Evidence: `kernel/axion/userenv/user_env.cpp`: `fire_shell_exec_gate`
allows any command not on `denied_commands_`, and the dispatch path runs
builtins before the gate check. This is a simulated shell (non-builtins
"succeed" without executing), so the impact is confined to the simulation's
fidelity, not host execution — noted for completeness, not as a host
bypass.

**Positive controls observed (security):** CanonFS Axion hook itself is
fail-closed: missing/invalid policy or an evaluation exception denies
(`kernel/axion/canonfs_hook.cpp:36–79`). VM evaluates policy *before* every
instruction dispatch, with `Deny` ⇒ `SecurityFault` (`vm/vm.cpp:616–622`);
TLOADHASH is re-checked with the real hash before the driver load
(`vm/vm.cpp:1863–1883`); unparseable program policy installs a deny engine
(`vm/vm.cpp:407–408`). Interchange load re-hashes and compares to the
requested ref (`fs/canonfs_interchange_ops.cpp:137–143`);
`CanonStore::get` verifies block hash (`ternaryos/dev/canon_store.cpp:116`);
export checks per-entry policy before materialization and rejects
absolute/`..` paths.

**B-NC1 — NEEDS CHECK — Policy lexer integer overflow crash.**
Evidence: `include/t81/axion/policy.hpp` lexer calls `std::stoll`/`std::stod`
with no try/catch; `vm/vm.cpp:382–383` calls `parse_policy` with no
try/catch either (the CanonFS hook does catch).
Next step: feed a VM a policy containing `(tier 9999999999999999999999)`
and an unterminated number like `(tier -)` and observe whether
`load_program` throws out of the process. If it crashes, severity is low
(local DoS via crafted program) and the fix is to catch in the lexer and
return a parse error.

**B-NC2 — NEEDS CHECK — QMATMUL `std::stoi` throw.**
Evidence: `kernel/axion/ai_hooks.cpp:89` parses `scale=` with unguarded
`stoi`.
Next step: determine whether a program-controlled payload string can reach
this path (VM QMATMUL handler payload construction) or whether payloads are
VM-constructed only; if reachable, same low DoS class as NC1.

**B-NC3 — NEEDS CHECK — Is any production path installing a CanonFS Axion
hook?**
Evidence: grep found hook installation only in examples/experiments; the
VM driver (`vm/vm.cpp:288–296`) and interchange drivers
(`fs/canonfs_interchange_ops.cpp`) are created without one, so their
`axion_allow` defaults to true.
Next step: confirm with the owning team whether CanonFS authorization is
intentionally Axion-free in production paths (interchange profile/evaluator
only) — if Axion is supposed to guard CanonFS, this is a
missing-enforcement gap, not just a default.

**B-NC4 — NEEDS CHECK — Cross-type decode strictness (for B-L1).**
Next step: check `decode_canon_tensor_object` and the CanonExec/JIT-trace
decoders for strict format validation; if any accepts arbitrary bytes, B-L1
upgrades from low to medium.

**B-NC5 — NEEDS CHECK — Binary policy deserialization reachability.**
Evidence: `kernel/axion/policy_serialization.cpp:225+`
(`Policy::deserialize`) performs unbounded `resize` from
attacker-influenced counts and reads past EOF silently (`is.get()` →
`0xFF`), but no in-tree caller was found.
Next step: confirm it is dead code (then delete or harden) vs. reachable
from a file/network input (then bound counts/lengths and check stream
state — medium).

**B-NC6 — NEEDS CHECK — Base81 short-string aliasing.**
Evidence: `CanonHash81::from_string` left-pads short decodes to 32 bytes.
Next step: confirm `t81::codec::base81::decode_bytes` drops leading zero
digits only (in which case left-padding is exactly correct for fixed
32-byte hashes) vs. any case where two distinct textual refs decode to the
same hash.

### C. Top-level claims vs implementation

Method note: the test suite was not built or run in this area, so pass-rate
claims are assessed as *unconfirmed by this pass*, not disproven. GitHub PR
merge states (e.g. PR #595/#596/#597) are unverifiable from this clone.
Phase 1's finding on the tryte-addition figure inside the claims-audit doc
itself is deliberately not re-reported.

**C-H1 — HIGH — README badge/table claim "ISA v1.9.0 Frozen" contradicts
the linked normative spec, which says v1.1.0.** See Top finding 5.
Additional evidence: `README.md:6` (badge `ISA: v1.9.0 Frozen`, linking to
`spec/tisc/opcode-registry.md`); repeated at `README.md:89` (citing
`spec/tisc/opcode-registry.md` and `include/t81/isa/opcodes.hpp`). Spec:
`spec/tisc/opcode-registry.md:2–3` ("Status: Canonical / Frozen (v1.1.0),
Version: 1.1.0"), corroborated by
`spec/tisc/TISC_CAPABILITY_CONTRACT_v1.1.0.md:1–3` and
`spec/tisc/TISC_FREEZE_VERIFICATION_REPORT_v1.1.0.md:1`. No `v1.9.0` ISA
document exists under `spec/tisc/`; the only `v1.9.0` hits are the README
badge/table, translated READMEs, and an RFC-00D4 example string
(`spec/rfcs/RFC-00D4-accountable-decision-bundle.md:128`, `t81vm-v1.9.0` —
a runtime version string, not an ISA version). Likely cause: conflation of
a runtime/product version with the ISA version.
Fix: change the badge and `README.md:89` to `v1.1.0`, or add one sentence
disambiguating "ISA v1.1.0 (frozen) vs runtime vX" if `v1.9.0` is a real
runtime version — and then cite where that runtime version is defined.
Apply the same fix to `README.es.md:16,45,55,57`, `README.pt-BR.md`,
`README.ru.md`, which additionally claim `369/369` tests passed (see C-M4).

**C-H2 — HIGH — Normative contradiction on opcodes 174–255: the capability
contract says "trap", the registry assigns them, the VM dispatches them.**
See Top finding 5. Additional evidence: contract
`spec/tisc/TISC_CAPABILITY_CONTRACT_v1.1.0.md:20` ("Opcodes `0xAE` (174)
through `0xFF` (255) are reserved. Execution of these opcodes MUST trap or
be rejected"); `:54–55` ("You MAY target opcodes 0–173. You MUST NOT emit
opcodes 174–255… You MUST treat 174–255 as invalid/trap."). Contradicting
spec: `spec/tisc/opcode-registry.md:13` ("Opcodes 174–199 are assigned:
Map/Set (174–185), TShape (186), AI-Native Inference / RFC-0026 (187–193),
Ternary-Native Inference / RFC-0034 (194–199)"); `:324` lists `FFICall`
at 200 even though `:13` also says 200–255 are "reserved for future
expansion." Contradicting implementation: `vm/vm.cpp:5691` dispatches
`Opcode::ATTN`, `:5743` dispatches `QMATMUL`, `:5818` dispatches `EMBED` —
the VM implements exactly the range the contract says to trap.
`include/t81/isa/opcodes.hpp:209–213` assigns the RFC-0034 ops to 194–199.
A compiler writer following the contract and a VM following the registry
cannot both be right; the AI-opcode surface (a headline capability claim)
is normative-ambiguous.
Fix: amend the contract to carve out the assigned 174–199 range (and
resolve `FFICall`/200), or downgrade the registry entries to draft. One
document must be declared the tie-breaker.

**C-H3 — HIGH — "Bit-exact everywhere / no undefined behavior, ever" in
the two historical reports contradicts the current README and the
determinism registry.**
Evidence: `T81_CAPABILITIES_REPORT.md:35` ("Bit-exact reproducibility
across all platforms"), `:178` ("Same model + same data = identical results
everywhere"), `:32` ("No undefined behavior, ever");
`SYSTEMS_AND_SUBSYSTEMS_REPORT.md:418` ("Bit-exact reproducibility
guaranteed"), `:445` (same, in the layer diagram). Contradicting current
sources: `README.md:196` explicitly lists "Universally bit-exact across
every operation, host architecture, compiler, and deployment mode" under
*What T81 Does Not Yet Establish*;
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md:37,42` — guarantees apply
only to "explicitly verified surfaces", "Surfaces without verification are
not guaranteed"; `:91–98` excludes real-time scheduling, native hardware FP
(outside `T81Float` wrappers), and performance determinism;
`spec/tisc/TISC_CAPABILITY_CONTRACT_v1.1.0.md:28` — "Strict Determinism is
only guaranteed if the VM is compiled with `T81_DETERMINISTIC`."
Mitigation noted: both historical reports now carry an Oct 27, 2026
qualification header (`T81_CAPABILITIES_REPORT.md:3–4`,
`SYSTEMS_AND_SUBSYSTEMS_REPORT.md:3–4`) telling readers to prefer the
README/surface register. That mitigates but does not fix the body text — a
reader landing mid-document still reads absolutes.
Fix: qualify the body claims in place (e.g. "bit-exact on verified DCP
surfaces when built with `T81_DETERMINISTIC`") or add a banner at each
absolute claim pointing to the registry. Do not leave "ever/everywhere/
guaranteed" standing unscoped.

**C-M1 — MEDIUM — Capabilities report conflates "RFC accepted" with
"implemented", and double-lists RFC-0038.**
Evidence: `T81_CAPABILITIES_REPORT.md:150–156` — "Fully Accepted Core RFCs
(23)… All foundational stack components are implemented and accepted",
including "RFC-0037 through RFC-0038". Contradiction 1 (same file):
`:158–159` lists RFC-0038 (lattice crypto) and RFC-0039 (NTRU) again under
"Proposed/In-Progress", and `:265` puts "Complete RFC-0038 lattice
cryptography implementation" in the near-term roadmap — i.e.
accepted-but-not-complete by the report's own account. Contradiction 2
(index): `spec/rfcs/index.md` legend defines `accepted` as "design
accepted, pending/ongoing integration" — not "implemented". The RFC files
themselves confirm accepted status (`spec/rfcs/RFC-0031-*.md`,
`RFC-0033-*.md`, `RFC-0037-*.md`, `RFC-0039-*.md` all carry
`Status: accepted`), which is a design state, not an implementation proof.
Fix: rename the section "Accepted (design) RFCs" and split it into
"accepted + implemented (with test evidence)" vs "accepted, implementation
pending". Remove RFC-0038 from one of the two lists.

**C-M2 — MEDIUM — "51 RFCs" scope is stale; the repo now has 113 RFC
files.**
Evidence: `T81_CAPABILITIES_REPORT.md:7,13,294` — "Total RFCs Analyzed:
51", "With 51 RFCs covering the full stack…". Actual: `ls
spec/rfcs/RFC-*.md | wc -l` = **113** (129 files total in `spec/rfcs/`
including README/template/index). `SYSTEMS_AND_SUBSYSTEMS_REPORT.md:307,413`
instead claims "120+ RFC documents" — also not matching 113.
Fix: either re-date/scope the capabilities report ("51 RFCs as of
2026-03-19") in the executive summary and conclusion, not just the header,
or refresh the count. Reconcile the systems report's "120+" to the actual
113 RFC files (or state what the 120+ counts).

**C-M3 — MEDIUM — "4.3× information density" and "2-bit vs 8-bit" memory
claims invert the audit's own packing arithmetic.**
Evidence: `T81_CAPABILITIES_REPORT.md:173` — "Base-81 encoding provides
~4.3× more information density than binary"; `:200–202` — "Memory
Efficiency: 2-bit trit encoding vs 8-bit bytes". Contradicting analysis
in-repo: `TERNARY_COMPARATIVE_CLAIMS_AUDIT.md` §4.2 — 41 trits at 2
bits/trit = 82 bits = 11 bytes vs 8 bytes for 64-bit binary, i.e. **37.5%
more** DRAM for equivalent capacity (~64.98 vs 64.00 bits). A byte holds 8
bits of information; a trit holds log₂3 ≈ 1.585 bits, so "2-bit vs 8-bit"
compares a per-digit code width against a per-byte capacity — not a
density win. No derivation of "4.3×" was found in `spec/rfcs/RFC-0000-*.md`
or the comparative research doc (grep for "4.3" returns only unrelated
section numbers).
Fix: delete the 4.3× claim or show its derivation with matched capacity
and packing overhead included; reword the memory line to the audit's
framing (packed 2-bit weights save vs FP32 *weights* — which the README
states correctly at `README.md:156` as a 93.75% *raw weight-storage*
reduction vs FP32, before metadata — but cost more than binary at matched
integer capacity).

**C-M4 — MEDIUM — Systems report inventory numbers are stale in several
places at once (counts, paths, sizes).**
Evidence: `SYSTEMS_AND_SUBSYSTEMS_REPORT.md:130,412,493` — "393 core tests
(all passing / 100% passing)". Actual: `tests/cpp/*.cpp` = **402 files**
(file count ≠ test count, so this alone doesn't disprove 393 tests, but no
passing-run artifact at HEAD was cited, and the translated READMEs claim a
different number, 369/369). This pass did not execute the suite, so the
pass rate is **unconfirmed**, and two different totals circulate. `:414` —
"17 automated workflows" vs **21** files in `.github/workflows/`
(including `ai-experiments-ci.yml`, `bench.yml` as claimed, plus 19
others). `:121` — `tools/cli/ai_tools/` vs actual `tools/cli/ai/` (no
`ai_tools` directory; `canonfs_tools/` and `core/` do exist). `:198` —
`ffi_dispatcher.cpp (15KB)` vs actual 28,570 bytes (~28 KB). Most other
size claims check out (vm.cpp ~310 KB vs claimed 301 KB, decoder ~62 KB vs
63 KB, tensor_helpers ~63 KB vs 64 KB, canonfs_interchange ~35 KB vs
36 KB) — this is staleness, not fabrication, but the FFI figure is ~2× off.
`:150` — `experimental/ai/ (47 items)` vs 13 top-level entries / 55 files
recursively — "items" is undefined, so the number is unverifiable as
written. `:411` — "1,200+ files" vs 4,045 files (excluding `.git`) — a
stale undercount; harmless direction, but confirms the inventory predates
major growth.
Fix: regenerate the inventory table from the tree at a stated commit,
define "test" vs "test file", and cite the CI run (workflow + commit)
behind any "100% passing" claim.

**C-M5 — MEDIUM — "Production-ready" labels on all five core components
overstate the governance register.**
Evidence: `SYSTEMS_AND_SUBSYSTEMS_REPORT.md:28,39,50,61,72` — TISC ISA,
T81VM, Axion, CanonFS, T81Lang all "Maturity: Production-ready"; `:15` —
DCP is "Production-ready, frozen, and verified". Contradicting sources:
`README.md` §3 prefaces the same table with "reported implementation
scope, **not a blanket production-readiness certification**", and §6 says
T81 is not presently a commercial desktop/server/general-purpose OS.
`docs/governance/GOVERNANCE_SURFACE_REGISTER.md:67–72` classifies T81Lang
frontend/toolchain as **"Governed non-DCP… partial deterministic emission
evidence only"**, not DCP, and notes "broader acceleration is outside this
class" for the VM. The systems report's own `:458` calls CSI "the missing
subsystem" while `vm/csi_integration.cpp` and `experimental/ai/csi/` (CLI,
sampling, provenance) already exist — wording that is at best stale.
Fix: downgrade T81Lang to "Stable (governed non-DCP)" per the register,
scope "production-ready" to the verified DCP surfaces only, and update the
CSI paragraph to describe what exists vs what is missing.

**C-M6 — MEDIUM — README's Wasmtime/OPA comparison cites a file that
doesn't contain it.**
Evidence: `README.md` §5 ("Governed execution") — "A conventional
Wasmtime/Open Policy Agent comparison reportedly achieved the same tested
zero-breach outcome and evidence auditability with less host-code
complexity", citing `docs/research/consolidation/README.md` and RFC-00D4.
Check: grep of `docs/research/consolidation/README.md` finds no mention of
Wasmtime, OPA, or zero-breach. The closest in-repo nuance is
`docs/research/consolidation/ARCHITECTURE_DECISION.md:78` — comparators
"are bounded comparators, not full OPA/Postgres/Wasmtime deployment
studies". RFC-00D4 (`:16`) only says an evidence bundle *may* be produced
by Wasmtime among other hosts — a portability statement, not a comparison
result.
Fix: point the citation at the actual comparison artifact (file + section
+ measured "host-code complexity" metric), or soften to "a bounded
comparator study (see ARCHITECTURE_DECISION.md:78) reported…" so the
claim's weight matches its evidence.

**C-M7 — MEDIUM — Matvec baseline in README/audit is ambiguous: both FP32
and INT8 are reported as "17.15 GFLOP/s".**
Evidence: `README.md:156` — "approximately 12.91 GFLOP/s, versus 17.15
GFLOP/s for an INT8 AVX2 comparison on the documented workload".
Complication: `TERNARY_COMPARATIVE_CLAIMS_AUDIT.md:127–128` lists *both*
"FP32 AVX2 (M=512, K=512): reported 17.15, re-executed 17.86" *and* "INT8
AVX2 (M=512, K=512): reported 17.15, re-executed 18.74" — the same reported
figure for two different baselines, with re-execution 4–9% off. The
underlying `docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md:17,28–31`
gives yet another workload picture (12.97 GFLOP/s; INT8 19.35 GFLOP/s on
4096×4096 DRAM-bound). The *direction* (packed ternary slower than INT8)
is consistent everywhere and the experiment code exists
(`experiments/packed_ternary_matvec/main.cpp`), but the single "17.15"
number in the README is not workload-anchored and cannot be reproduced
exactly from either doc.
Fix: in `README.md:156`, name the workload (M×K, host class) and use the
report's own figures (12.97 vs the matching INT8 figure for that workload),
or label 12.91/17.15 as "Investigation 05 as-reported, re-execution within
~9%".

**C-M8 — MEDIUM — Cognitive tiers T81→T19683 presented as architecture,
but only experimental tier scaffolding exists.**
Evidence: `T81_CAPABILITIES_REPORT.md:24` (tiers as core architecture),
`:188` — "Tier 6 (T19683): Continuum cognition with strict containment".
Code: tier material is experimental-only: `experimental/tiers/` (README +
cog + test stub), `include/t81/experimental/cog/tier.hpp`,
`include/t81/experimental/cog/tier6/distributed_monad.hpp`. No Tier 4–6
cognition dispatch was found in `vm/vm.cpp` (AI dispatch present is phase-1
ATTN/QMATMUL/EMBED/WLOAD/GATHER/SCATTER, `vm/vm.cpp:5691–5818`). "Strict
containment" rests on Axion policy mediation plus the `AXHALT` path
(`vm/vm.cpp:7511–7518`, ethics/capability halt reasons) — real code, but a
far cry from demonstrated continuum cognition.
Fix: move tiers to the experimental-frontier section with an explicit "no
Tier 4–6 VM implementation" note, or cite the tier tests that demonstrate
each tier's claimed capability.

**C-L1 — LOW — Audit doc's own C-05 citation points at a file that no
longer exists at that path.**
Evidence: `TERNARY_COMPARATIVE_CLAIMS_AUDIT.md:39` (C-05) — "Implemented
in `src/t81int.cpp`, tested in `tests/cpp/test_t81int.cpp`". Check:
`src/t81int.cpp` does **not** exist at HEAD; the BigInt implementation now
lives at `include/t81/bigint.hpp` (test file `tests/cpp/test_t81int.cpp`
does exist, plus `t81int_properties_test.cpp`,
`test_t81int_generic_div.cpp`, etc.). The arithmetic itself (27-trit limbs
≈ 42.79 bits → ~(1.5)² ≈ 2.25× schoolbook tax) is internally coherent;
only the citation path is stale.
Fix: update the C-05 evidence column to `include/t81/bigint.hpp`.

**C-L2 — LOW — Audit's "Verified & Reproduced" overstates exactness; two
percentage wordings are unexplained.**
Evidence: `TERNARY_COMPARATIVE_CLAIMS_AUDIT.md:21,40,145` calls the matvec
numbers "Verified & Reproduced" while its own table (`:127–131`) shows
re-executed INT8 at 18.74 vs reported 17.15 (+9.3%) and FP32 at 17.86 vs
17.15 (+4.1%). Direction and magnitude reproduce; exact figures do not.
The same doc gives trit-on-binary storage redundancy as "33.3%" (executive
summary) and "25% of the 2-bit state space" (§2.1) without noting these are
different denominators (state-count vs bits); both are arithmetically
defensible but a reader will see a contradiction.
Fix: change the label to "Reproduced within ~10% (direction confirmed)"
and add one parenthetical defining each percentage's denominator.

**C-L3 — LOW — Broken cross-reference: determinism registry points to a
README section that isn't there.**
Evidence: `docs/governance/DETERMINISM_SURFACE_REGISTRY.md:37` defines
"supported architectures" as "the platforms listed in the root README under
Compatibility for the current major version" — but the English `README.md`
has no Compatibility section (grep finds none). The systems report's `:421`
claim "Cross-platform: Linux, macOS, Windows, ARM64" therefore rests on an
undefined platform list, while `README.md` §7 itself warns "Some
experimental targets and dependencies are platform-specific."
Fix: add the Compatibility section to the English README or repoint the
registry at the actual platform list (e.g. CI matrix in
`.github/workflows/cross-platform-determinism.yml`).

**C-L4 — LOW — "SIMD 10–15 Gops/s" and "single PSHUFB (1-cycle)" are
unanchored generalities.**
Evidence: `T81_CAPABILITIES_REPORT.md:200–201` presents these as general
SIMD performance. Archived results do show numbers in/around that range
for *specific* kernels (`benchmarks/results/archive/benchmark_results_release.txt:23`
— negation at 34.74 Gops/s; `benchmarks/results/inference_comparison.md` —
1–19 Gops/s depending on kernel/size), but no kernel, size, or host is
cited, and the same `VPSHUFB` instruction the capabilities report praises
for negation is the instruction the audit blames for the 25–30% matvec
decode tax (different uses, but the report never distinguishes them).
Fix: attach kernel + workload + host to each figure, or drop the range in
favor of the matvec report's workload-specific table.

**Verified as stated (claims area spot-checks that held up):**
`README.md` §3 component paths all exist, as do §7 CanonFS example files
and `t81 code build` (`tools/cli/core/main.cpp:448,453`). Investigation 07
numbers in `README.md:172` match
`experiments/ternary_hardware_feasibility/FINDINGS_REPORT.md:112–114`
exactly (36,547.5 / 6,564.5 / 5,980.5 at N=64; ~1.10×), and the README
correctly bounds the claim. Investigation 08 is handled honestly in
`README.md` §4–5 ("INCONCLUSIVE", baselines not functionally equivalent).
Hanoi evidence quoted in the audit matches the artifact in substance
(`docs/research/continuity/evidence/hanoi-arithmetic-results.json` — 547
addition cases / 422 failures, 201 / 140, 27 / 0). The 27-program
conformance suite is corroborated outside the capabilities report
(`spec/rfcs/index.md` RFC-0002 entry;
`docs/status/CI_GATE_STATUS.md:174`).

### D. Dead references repo-wide (docs/, spec/, READMEs; docs/ai-context/ excluded)

Method: `git ls-files` (4,080 tracked files) + filesystem existence checks,
resolving every markdown link relative to its citing file. PR numbers were
ignored (never treated as dead references); a SHA/PR outside the shallow
window was not treated as missing. Root-relative backtick shorthand (e.g.
`spec/rfcs/index.md` written inside `spec/rfcs/README.md`) was **not**
counted as dead — only clickable links and explicitly relative (`../…`)
references were. Counts: 881 scope files, 1,750 internal markdown links
checked, **71 dead link rows / 41 distinct dead targets (~4% of internal
links)**, grouped into 14 findings. Most dead rows share just two root
causes: a stale `core/`/`runtime/`/`src/canonfs/` layout, and off-by-one
`../` counts.

**D-H1 — HIGH — `core/vm/vm.cpp` does not exist; the VM is `vm/vm.cpp`.**
Evidence: the `core/` tree does not exist at repo root at all
(`git ls-files | grep '^core/'` = empty except `src/core/CMakeLists.txt`).
The interpreter is `vm/vm.cpp` (`class Interpreter`, line 281) and
`vm/internal/` exists. Occurrences:
`docs/architecture/cross-cutting/execution-model.md:53`,
`docs/architecture/cross-cutting/governance-axion.md:55`;
`docs/architecture/layers/axion.md:26,70`,
`docs/architecture/layers/experimental-tiers.md:62`,
`docs/architecture/layers/tisc-isa.md:63`,
`docs/architecture/layers/vm.md:31,124`; contributor guide
`docs/developer-guide/internals/vm-opcodes.md:33,146,153`. Check run:
`git ls-files vm/vm.cpp` = exists; `git ls-files core/vm/vm.cpp` = missing.
Fix: global replace `core/vm/vm.cpp` → `vm/vm.cpp` and `core/vm/internal/`
→ `vm/internal/` in the files above (and in
`docs/architecture/layers/vm.md:33`).

**D-H2 — HIGH — CLI user manual (the support contract) has 4 dead links,
all one `../` too many.**
Evidence: `docs/user-guide/reference/cli-user-manual.md` sits 3 levels deep
(`docs/user-guide/reference/`), so `../../../` escapes to repo root, where
`product/`, `governance/`, `reference/` do not exist. All four targets
exist under `docs/`. `:35`
`../../../product/CLI_JSON_SCHEMA_CONTRACTS.md` →
`../../product/CLI_JSON_SCHEMA_CONTRACTS.md`; `:55`
`../../../governance/DETERMINISM_THREAT_MODEL.md` →
`../../governance/DETERMINISM_THREAT_MODEL.md`; `:56`
`../../../reference/CAPABILITY_CONTRACT.md` →
`../../reference/CAPABILITY_CONTRACT.md`; `:57`
`../../../reference/REPRODUCIBILITY.md` →
`../../reference/REPRODUCIBILITY.md`. Check run: all four
`docs/product/…`, `docs/governance/…`, `docs/reference/…` files confirmed
present via `git ls-files --error-unmatch`.

**D-H3 — HIGH — `ternaryos/docs/README.md` has 7 dead links, all one
`../` too many.**
Evidence: from `ternaryos/docs/` (2 levels), `../../../` escapes above the
repo root. Every target exists with one fewer `../`. `:11`
`../../../docs/research/ternary_os_roadmap.md` →
`../../docs/research/ternary_os_roadmap.md` (file confirmed present);
`:12–17` `../../../spec/rfcs/RFC-00B0-hal-spec.md`,
`RFC-00B1-ternary-mmu.md`, `RFC-00B2-device-drivers.md`,
`RFC-00B3-axion-kernel-architecture.md`,
`RFC-00B6-minimal-syscall-capability-boundary.md`,
`RFC-00B5-governed-event-interrupt-model.md` → same filenames under
`../../spec/rfcs/` (all confirmed present in `spec/rfcs/`).

**D-H4 — HIGH — Architecture layers: `core/isa`, `core/types`,
`runtime/jit`, `src/canonfs` are all wrong paths.**
Evidence: same stale-layout root cause as D-H1, in load-bearing
architecture docs. Correct locations verified by filename search:
`docs/architecture/layers/tisc-isa.md:24,62` — `../../../core/isa`,
`../../../core/isa/README.md` → `isa/` and `isa/README.md` (both exist;
`include/t81/isa/` is the header side).
`docs/architecture/layers/data-types.md:20,62` —
`../../../core/types/README.md` → `isa/types/README.md` (exists).
`docs/architecture/cross-cutting/execution-model.md:55,56` —
`../../../runtime/jit/jit_compiler.cpp`, `../../../runtime/jit/README.md` →
`vm/jit/jit_compiler.cpp`, `vm/jit/README.md` (both exist).
`docs/architecture/layers/canonfs.md:26,73,74` —
`../../../src/canonfs/in_memory_driver.cpp`,
`../../../src/canonfs/persistent_driver.cpp` →
`fs/in_memory_driver.cpp`, `fs/persistent_driver.cpp` (both exist;
`src/canonfs/` never existed). Same table prose in
`docs/architecture/OVERVIEW.md:40–44` (`core/types/`, `core/isa/`,
`core/vm/`, `src/canonfs/`) needs the same four replacements.

**D-H5 — HIGH — Governance / product scope tables repeat the stale
`core/` + `runtime/` layout.**
Evidence: these define frozen/DCP scope, so a wrong path here misstates
what is governed: `docs/governance/GOVERNANCE_SURFACE_REGISTER.md:67–71,74`
— `core/types/`, `core/isa/`, `core/vm/`, `src/canonfs/` → `isa/types/`,
`isa/`, `vm/`, `fs/` respectively.
`docs/governance/FREEZE_ENFORCEMENT.md:24,33` — same `core/types/`,
`core/isa/`, `core/vm/` replacements.
`docs/governance/MODULE_OWNERSHIP_CHARTER.md:17–22,38–43,63–64` — same,
plus `runtime/jit/` → `vm/jit/` and `runtime/tracing/` → `vm/tracing/`
(next check: confirm tracing ownership is `vm/tracing/` vs
`include/t81/tracing/` before editing that one line).
`docs/governance/SPEC_AUTHORITY_MODEL.md:123–125`,
`docs/product/DETERMINISTIC_CORE_PROFILE.md:27–29,40` — same `core/` →
`isa/`/`vm/` and `runtime/jit/` → `vm/jit/` replacements. Check run: no
`core/` directory exists; `isa/`, `vm/`, `fs/`, `vm/jit/` all exist.

**D-M1 — MEDIUM — Quickstart INSTALL: two root-relative references that
fail as written.**
Evidence: `docs/user-guide/quickstart/INSTALL.md:115` — link
`[Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)`
resolves to `docs/user-guide/quickstart/docs/governance/…` (missing).
Fix: `../../governance/DETERMINISM_SURFACE_REGISTRY.md` (file exists).
`docs/user-guide/quickstart/INSTALL.md:113` — backtick
`examples/consumer_cmake/` resolves to
`docs/user-guide/quickstart/examples/…` (missing).
Fix: `../../../examples/compiler-and-ffi/consumer_cmake/` (confirmed
present: `CMakeLists.txt`, `README.md`, `main.cpp`).

**D-M2 — MEDIUM — Process roadmap: 5 dead links, all wrong depth.**
Evidence: `docs/process/roadmaps-plans/ROADMAP.md:338–342` (file is 3 levels
deep, so `../../` lands in `docs/`, not repo root, and `../` lands in
`docs/process/`): `../../spec/README.md` → `../../../spec/README.md`;
`../architecture/OVERVIEW.md` → `../../architecture/OVERVIEW.md`;
`../status/PROJECT_CONTROL_CENTER.md` →
`../../status/PROJECT_CONTROL_CENTER.md`;
`../governance/DETERMINISM_SURFACE_REGISTRY.md` →
`../../governance/DETERMINISM_SURFACE_REGISTRY.md`;
`../status/IMPLEMENTATION_MATRIX.md` →
`../../status/IMPLEMENTATION_MATRIX.md`. Check run: all five targets
confirmed present at the corrected paths; `docs/process/architecture/` and
`docs/process/status/` do not exist.

**D-M3 — MEDIUM — Stdlib promotion snapshot points at a moved standards
file (11 rows, 1 target).**
Evidence:
`docs/records/status-history/STDLIB_PROMOTION_SNAPSHOT_2026-03-14.md:11–21`
— all 11 evidence links use `../standards/standard-library.md`, resolving
to `docs/records/standards/standard-library.md` (missing;
`docs/records/standards/` does not exist).
Fix: `../../standards/standard-library.md` →
`docs/standards/standard-library.md` (exists). Anchor check (`#stdcore`,
`#stdmath`, …) was not verified — confirm anchors exist in that file
before closing.

**D-M4 — MEDIUM — March 2026 proposals point at pre-relocation paths (now
wrong-path, not historically wrong).**
Evidence:
`docs/process/proposals/MISSION_ALIGNMENT_EXECUTION_PLAN_2026-03-19.md:105,121`
— `/experimental/ternaryos` → `/ternaryos` (tree now at repo-root
`ternaryos/`, confirmed). Same file `:182,183,270,271` —
`/tooling/cli/main.cpp`, `/tooling/cli/driver.cpp`,
`/tooling/cli/ai/ai_cli_shared.cpp` → `/tools/cli/core/main.cpp`,
`/tools/cli/core/driver.cpp`, `/tools/cli/ai/ai_cli_shared.cpp` (all
confirmed present).
`docs/process/proposals/TERNARYOS_RELOCATION_PROPOSAL_2026-03-19.md:37,50,51,106,109,142,160`
— `/experimental/ternaryos`,
`/experimental/ternaryos/docs/kernel_execution_plan.md`,
`/experimental/ternaryos/docs/virtualbox_x86_64_handoff.md`,
`/experimental/ternaryos/docs/README.md`,
`/experimental/ternaryos/tests/scheduler_test.cpp` → the same names under
`/ternaryos/…` (all confirmed present, including
`ternaryos/tests/scheduler_test.cpp`).
Fix: retarget, or add a one-line "relocated to `ternaryos/`" note — do not
silently rewrite the historical proposal's reasoning; the links themselves
are what fail today.

**D-M5 — MEDIUM — RFC catalog gap (reverse dead reference): `RFC-00D4`
file exists but is absent from the index.**
Evidence: `spec/rfcs/RFC-00D4-accountable-decision-bundle.md` exists, but
`grep '00D4' spec/rfcs/index.md` returns nothing, while the index's own
rule (`spec/rfcs/README.md`: "index.md must remain a complete catalog") is
violated.
Fix: add the RFC-00D4 catalog row. Related note: RFC-00D3 exists only as
four `RFC-00D3-*.json` schemas with no `.md` primary — confirm whether it
needs a catalog entry or a primary document; it was not counted as a dead
link. Conversely, **no in-scope RFC identifier was found dead**: every
`RFC-XXXX` cited in `docs/` (excl. ai-context), `spec/`, and READMEs
resolves to a file in `spec/rfcs/`. The `RFC-0035` numbering gap is not
cited anywhere in scope, so it is not a dead reference; `RFC-0057`/
`RFC-00E9` appear only in the excluded `docs/ai-context/` pack and are out
of scope for this area.

**D-M6 — MEDIUM — Explanation architecture table cites non-existent spec
directories.**
Evidence: `docs/explanation/ARCHITECTURE.md:289,290,292,294` —
`../../spec/tisc/` (exists as a directory, OK), but `../../spec/vm/`,
`../../spec/canonfs/`, `../../spec/numerics/` do not exist.
Fix: `spec/vm/` → `spec/t81vm-spec.md`, `spec/canonfs/` →
`spec/supplemental/canonfs-spec.md`, `spec/numerics/` →
`spec/t81-data-types.md`; the `core/`/`src/canonfs/` columns in the same
rows need the D-H4/D-H5 replacements.

**D-L1 — LOW — Module README "See Also" off-by-one (relative, not
shorthand).**
Evidence: `codec/README.md:15`, `crypto/README.md:19` — `` `../../docs/explanation/ARCHITECTURE.md` ``
from a top-level directory escapes the repo.
Fix: `../docs/explanation/ARCHITECTURE.md` (file exists).
`experimental/tiers/cog/README.md:15` — same template, one level short.
Fix: `../../../docs/explanation/ARCHITECTURE.md`.
`experimental/tiers/cog/tier1/README.md:22` — `` `../../docs/explanation/cognitive_tiers.md` ``
is doubly dead: the depth is wrong *and*
`docs/explanation/cognitive_tiers.md` exists nowhere (filename search found
only `spec/cognitive-tiers.md` and an archived
`docs/records/archive/reference/cognitive_tiers.md`).
Fix: point at `spec/cognitive-tiers.md` (with corrected depth) or remove.

**D-L2 — LOW — Archived temporal-guides Axion README (archive, low
priority).**
Evidence:
`docs/records/archive/temporal-guides/guides/axion/README.md:6,7,8,11,12,13,16`
— `../axion-trace.md`, `../axion-policy-manual.md`,
`../axion-tracing-manual.md` resolve to
`docs/records/archive/temporal-guides/guides/axion-*.md` (missing; the live
guides are `docs/developer-guide/internals/axion-trace.md` and
`docs/developer-guide/internals/axion-tracing-manual.md`);
`../../spec/axion-kernel.md` resolves to
`docs/records/archive/temporal-guides/spec/axion-kernel.md` (missing;
correct is `spec/axion-kernel.md`); `../../navigation.md` and the
`docs/navigation.md` / `docs/guides/README.md` backticks similarly miss
(`docs/navigation.md` exists, 4 levels up).
Fix: retarget to the developer-guide files and `spec/axion-kernel.md`, or
leave archived as-is with an archive banner — do not spend load-bearing
effort here.

**D-L3 — LOW — DPE benchmark evidence link points at a build artifact, not
a tracked file.**
Evidence:
`docs/records/status-history/DPE_COMPARATIVE_BENCHMARK_EVIDENCE_2026-03-19.md:34`
— `[console.txt](/build/dpe-bench/console.txt)`; no such tracked file
exists (`build/` is generated).
Next check, not a deletion: confirm whether this console log was ever meant
to be committed; if it is build output only, annotate the link as such
rather than treating it as a repo dead reference.

**D-L4 — LOW — Experimental README inventory path is wrong-path.**
Evidence: `experimental/README.md:76` — `` `../docs/status/EXPERIMENTAL_SURFACE_INVENTORY.md` ``
resolves to `docs/status/EXPERIMENTAL_SURFACE_INVENTORY.md` (missing).
Fix: `docs/records/status-history/EXPERIMENTAL_SURFACE_INVENTORY.md`
(confirmed present).

**Explicitly NOT dead (checked, to prevent false positives):** root
`README.md`, `docs/README.md`, `docs/index.md`, `docs/navigation.md`,
`spec/README.md`, and `spec/index.md` contain no dead markdown links;
`docs/rfcs/` holds only a title `README.md` (the canonical RFC home is
`spec/rfcs/`); `spec/rfcs/index.md` covers every `.md` RFC file except
RFC-00D4 (D-M5); IETF `RFC 2119` in `spec/README.md:9` is an external
reference, not a repo RFC. `docs/HANDOFF.md` link *display text* sometimes
says `docs/…` while the actual link target is the correct relative path —
clickable targets resolve, so those were not counted.

### E. Build and test health (bounded)

Environment: cmake 3.28.3, g++ 13.3.0 (meets README's GCC 13+ / CMake 3.25+),
Python 3.12.3, 4 CPUs, ~1.2 GB RAM free. **No `ninja`, no `clang++`, no
system GTest** — relevant to E-F4. Read-only throughout — `git status
--porcelain` clean at the end. All build output went to `/tmp/t81-build`,
never into the repo. No full-repo build was attempted (bounded task).

Stage results: `cmake --preset default` FAILED (no Ninja binary in this
environment — not a repo defect — plus consequent `CMAKE_C_COMPILER not
set` errors; presets hard-require Ninja, see E-F4). Makefiles fallback
configure SUCCEEDED (`cmake -S <repo> -B /tmp/t81-build -G "Unix
Makefiles" -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON
-DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF`, 78.4s, exit 0;
TernaryOS and DPE left ON, matching the `ci` preset's intent). Two small
named targets built (`t81_packed_trit_vector_test`,
`t81_determinism_primitives_test`) and `ctest -R` on them PASSED 2/2
(0.09 s). Full build NOT RUN (1,626 make targets; committed `build.log`
shows a 1,106-step Ninja build). Full `ctest` NOT RUN (depends on the full
build; 410 registered tests inventoried, not executed).

**E-F1 — HIGH — 26 sub-directory tests build but are invisible to `ctest`,
including the tests CI's epoch gate runs.**
Evidence: root `CMakeLists.txt:27` (`add_subdirectory(src)`) and `:98`
(`add_subdirectory(ternaryos)`) run **before** the repo's only main-tree
`enable_testing()` at `CMakeLists.txt:1355`. CMake does not retroactively
register `add_test()` calls from already-processed subdirectories.
Consequences, reproduced in `/tmp/t81-build`: `ctest -N --test-dir
/tmp/t81-build/ternaryos` → `Total Tests: 0`; no
`/tmp/t81-build/ternaryos/CTestTestfile.cmake` exists, although target
dirs like `t81_ternaryos_hal_boot_test.dir` exist (binaries build fine).
`ctest -N` for the whole tree contains **0** matches for `t81_ternaryos_*`
or `t81_dpe_*`, despite 17 `add_test` calls in
`ternaryos/CMakeLists.txt:1044–1139` (HAL boot, context switch, MMU,
scheduler, IPC, device driver, shell, userenv, epoch
submission/audit/history/timeout/policy/syscall) and 8 in
`src/vm/CMakeLists.txt:51–98` (`t81_dpe_task_graph_test`,
`t81_dpe_task_runner_test`, `t81_dpe_epoch_commit_test`,
`t81_dpe_epoch_dag_test`, `t81_dpe_epoch_parallel_test`,
`t81_dpe_thread_pool_test`, `t81_dpe_float_reduction_test`,
`t81_dpe_include_surface_test`), plus 1 in `src/tools/CMakeLists.txt:106`
(`tui_snapshot_test`). **CI impact:**
`.github/workflows/ci.yml` job `axion-epoch-determinism` (~line 1015)
builds `t81_ternaryos_epoch_submission_test` +
`t81_ternaryos_epoch_audit_test`, then runs `ctest --test-dir build -R
't81_ternaryos_epoch_(submission|audit)_test'` with no `--no-tests`
fallback. With zero matching tests, ctest reports "No tests were found"
and exits non-zero — so this gate is expected to fail (or pass vacuously
only if some step masks it). This also explains why
`docs/research/CAPABILITY_BASELINE.md`'s `t81_dpe_*` /
`t81_ternaryos_*` test names appear "missing": the targets exist, the
registrations don't.
Proposed fix: move `enable_testing()` to the top of the root
`CMakeLists.txt`, before any `add_subdirectory()` that contains tests (or
add `enable_testing()` at the top of `src/CMakeLists.txt` and
`ternaryos/CMakeLists.txt`). Then verify: `ctest -N` must list the 26
tests, and re-run the `axion-epoch-determinism` job. Caveat: reproduced
with the Unix Makefiles generator (CI uses Ninja); the `enable_testing`
ordering semantics are generator-independent, but confirm with one Ninja
configure since that is the CI path.

**E-F2 — MEDIUM — 26 test sources in `tests/cpp/` are referenced by no
`CMakeLists.txt` at all.**
Evidence: name search over all `CMakeLists.txt` (no `GLOB` is used for
tests) finds these unbuilt: `ai_phase3_jit_optimization_test.cpp`,
`axion_segment_trace_test.cpp`, `canonfs_enhanced_errors_simple_test.cpp`,
`canonfs_enhanced_errors_test.cpp`, `canonfs_interchange_engine_test.cpp`,
`canonfs_negative_tests.cpp`, `cli_integration_test.cpp`,
`cognitive_tier_framework_test.cpp`, `cognitive_tier_minimal_test.cpp`,
`cognitive_tier_simple_test.cpp`, `comprehensive_test.cpp`,
`controlled_ai_backend_test.cpp`, `ffi_basic_test.cpp`,
`frontend_ir_generator_relation_test.cpp`, `lang_literal_pool_test.cpp`,
`test_axion_recursion_limit_ext.cpp`, `test_complex_determinism.cpp`,
`test_complex_serialization.cpp`, `test_frontend_bitwise.cpp`,
`test_property_float.cpp`, `test_property_invariants.cpp`,
`test_reflective_opcodes.cpp`, `test_remediation.cpp`,
`tier4_loop_closure_test.cpp`, `vm_gc_test.cpp`, `vm_reflection_test.cpp`.
None is referenced from any `.txt`/`.cmake`/`.py`/`.yml`/`.md` outside the
file tree (except two in stale
`docs/records/inventories/repo_tree_expanded.txt` snapshots).
Proposed fix: for each file, either wire it into `CMakeLists.txt` (if it
compiles and is wanted) or delete it. Add a CI guard (e.g. extend the
existing "removed non-compiling test references" pattern) that fails on
`tests/cpp/*.cpp` files with no target, or maintain an explicit exclusion
list so dead tests are a deliberate decision, not drift. Note CI already
asserts one legacy file stays absent (`ci.yml:287–290`,
`test_axion_evidence_loop_closure.cpp` — verified absent, and the four
deterministic-profile files it requires all exist), so an exclusion-list
mechanism already fits the repo's conventions.

**E-F3 — MEDIUM — Governance/capability docs cite test names that are not
registered tests.**
Evidence (after correcting for the `t81_` prefix and for sources under
`ternaryos/tests/` and `tests/cpp/dpe/`): most raw mismatches dissolve —
e.g. docs' `canonfs_driver_test` is registered as
`t81_canonfs_driver_test`; `hal_boot_test.cpp` exists under
`ternaryos/tests/`. Residual genuine gaps include:
`docs/governance/ENFORCEMENT_MATRIX.md` citing
`vm_stubbed_async_network_opcode_fail_closed_test` /
`vm_stubbed_privileged_opcode_fail_closed_test` (only the generic
`t81_vm_stubbed_opcode_fail_closed_test` exists, `CMakeLists.txt:729`);
`docs/developer-guide/internals/vm-opcodes.md` citing `t81_vm_divmod_test`,
`vm_my_opcode_test`; `docs/process/policies/EVIDENCE.md` citing
`error_determinism_test`, `strict_profile_test`, `vm_trap_test`,
`symbol_content_address_test`;
`docs/research/CAPABILITY_BASELINE.md`'s `t81_weights_quantize_test` and
`t81_ternaryos_epoch_timeout_test` (the latter exists as a target but is
unregistered per E-F1). Proposal/poc docs (`c-frontend-mlir-poc`,
`python/rust-frontend-mlir-poc`, `TERNARYOS_RELOCATION_PROPOSAL`) citing
frontend/MLIR smoke tests are aspirational and expected to gap while
`T81_ENABLE_LLVM/MLIR` default OFF — low concern on their own.
Proposed fix: after fixing E-F1, re-run the doc↔ctest cross-check and
either rename doc references to the registered names or add the missing
tests — starting with `ENFORCEMENT_MATRIX.md`, since it presents the
stubbed-opcode tests as enforcement evidence. Do not count proposal-doc
names as coverage claims.

**E-F4 — MEDIUM — The documented newcomer build path has no fallback when
Ninja is absent.**
Evidence: `README.md:208–223` requires "CMake 3.25+ and Ninja" and shows
`cmake -B build -G Ninja …`; every configure preset in `CMakePresets.json`
(`default`, `dev`, `ci`, `asan`, `warn-strict`) sets `"generator":
"Ninja"`. On a box with a compliant compiler but no Ninja, the documented
path fails outright (stage table above). CI is unaffected (it `apt-get
install`s `ninja-build`, `ci.yml` Linux deps step).
Proposed fix: add a `default-make` (Unix Makefiles) preset, or document the
Makefiles fallback command in README §7. Also note presets write into the
source tree (`build/`, `build-dev/`, `build-asan/`), which conflicts with
out-of-tree hygiene; consider `${sourceDir}/../` or documenting a scratch
dir.

**E-F5 — LOW — Configure noise and stale artifacts that can mislead a
newcomer.**
Evidence: (a) `ternaryos/CMakeLists.txt:834` warns on Linux that "QEMU/EFI
guest targets are disabled because Homebrew clang was not found" with a
`/opt/homebrew/opt/llvm/bin/clang` example path — a macOS-specific hint
surfacing on Linux. (b) `Could NOT find Doxygen` warning (docs build only,
harmless). (c) A gitignored but present `build.log` in the repo root
records a 1,106-step Ninja build — stale evidence that could be mistaken
for a current build record. (d) `third_party/llama.cpp` submodule is
uninitialized (`git submodule status` shows `-3769fe6…`); harmless because
`T81_ENABLE_LLAMA_CPP` defaults OFF (`cmake/Options.cmake`), but the
research-regressions path that needs it should fail with a clear message
rather than a cryptic one.
Proposed fix: gate the Homebrew hint to Apple platforms; remove the stale
`build.log` (it's gitignored, so deletion is safe); keep the llama.cpp
submodule note in README §7's prerequisites for the opt-in regressions
slice.

**Inventory (this configuration: Release, tests ON, examples OFF,
benchmarks OFF):** registered ctest tests: **410** (31
determinism-named, 6 ternary/trit-named by substring, plus
spec-conformance `tisc_*` tests from `spec/conformance`, added after
`enable_testing()` — which is why they register while ternaryos/DPE tests
don't). Make targets: **1,626**. Root `CMakeLists.txt`: 120 `add_test`
lines, 386 `add_executable(…test…)` lines. Test sources: `tests/cpp/` 415
files (394 matching `*_test.cpp`/`test_*.cpp`), `tests/determinism/` 11
files, `tests/python/` 8 files, `tests/cpp/dpe/` plus `ternaryos/tests/`
16 files. The 26 E-F2 orphans are ~6% of `tests/cpp` sources. CI surface:
19 workflow files; `ci.yml` `build-and-test` matrix covers linux-x86_64
(gcc-14, clang-18), linux-arm64 (clang-18), macOS arm64/x86_64, Windows
(MSVC, clang-cl), 40-min timeout — a broader matrix than this single
Linux/GCC-13 run can validate, so these findings are scoped to the
Linux/GCC path.

## Recommended next checks (in order)

1. Move `enable_testing()` and re-run `ctest -N` plus the
   `axion-epoch-determinism` job (E-F1).
2. One full Ninja build + full `ctest` on a CI-like image (gcc-14/clang-18,
   Ninja installed) to validate the other 408 registered tests.
3. Fix A-H1–A-H3, B-H1–B-H2, and C-H1–C-H2, then re-audit the JIT, policy
   boundary, and ISA contract against the fixes.
4. Resolve the open needs-check items: A-NC1–A-NC5, B-NC1–B-NC6 (including
   confirming the B-M1-governed TLOADHASH verdict against the VM path),
   the D-area next checks (tracing ownership in D-H5, anchors in D-M3,
   DPE console-log intent in D-L3), and the E-F3 doc↔ctest cross-check
   after E-F1.

---

*Submitted for the maintainer's review — findings only, no code changes in
this PR.*
