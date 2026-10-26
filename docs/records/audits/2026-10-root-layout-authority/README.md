# Root layout authority investigation

Investigated 2026-10-26 (America/New_York). Outcome: **mixed stale checks,
incomplete governance migration, and tracked output artifacts; complete root
authority remains unresolved. No validator correction is implemented.**

The request explicitly says to stop rather than implement a speculative change
if authority remains ambiguous. This report records that stopping point. It does
not approve every committed entry, delete research evidence, or propose changing
any scientific conclusion. This work belongs to newcomer clarity and boring CI,
using the existing CMake/runtime and governance surfaces.

## Exact baseline

`git fetch origin` succeeded. Read-only GitHub pull-request API queries confirmed
all four PRs open and unmerged, with the supplied exact heads:

| PR | Head | Base |
| --- | --- | --- |
| #574 | `10df367102151b087ee64b1fbb737c471f4c87fb` | `01c85019319fd020922402ba356878e392cb2231` |
| #575 | `b06f8520d02fd68301a5ae06777beb03b73c8eb5` | #574 head |
| #576 | `41fa126cc82df6ed54b095d8dd67bfe066252066` | #575 head |
| #577 | `b6a20706b3b37eebbfb5ab0293a88306a3dae29a` | #576 head |

Created branch `corrective/root-layout-authority` in an isolated worktree at
exact #577 head. Both before and after the inventory, tracked/untracked status
was empty. The original workspace and its untracked `build-bench/` were not
changed. The worktree's `.git` is Git administration, not a tracked root entry.

[baseline.json](baseline.json) contains the SHA, clean status, complete physical
root listing, tracked root modes/object IDs, untracked listing, platform, and
original validator diagnostics. Every one of the **19 unexpected entries is
tracked**. Neither an untracked local build nor a dirty checkout explains this
failure. The validator exited **1**, requiring the two absent npm manifests and
rejecting the entries classified below.

## Authority and migration history

The authority hierarchy is `/spec/**`, then `docs/architecture/OVERVIEW.md`, then
other docs. `docs/governance/SPEC_AUTHORITY_MODEL.md` §4F explicitly prohibits
new root files without unanimous maintainer approval. Its directory ownership
table still names `core/types/`, `core/isa/`, and `core/vm/`; the architecture
overview also retains some of these paths. None provides a complete, current,
typed root-layout contract or a disposition for the disputed output artifacts.
The normative deterministic-core profile and RFC-0048 classify semantic
surfaces; they do not supply that missing root inventory.

Key history, preserved with patches/statistics in
[authority-evidence.json](authority-evidence.json):

- `529a9486` (2026-02-26) introduced this exact-set check, including `src/`
  and both npm manifests. It is an operational snapshot, not evidence of
  independent approval for every entry.
- `c087907d` (2026-03-05) explicitly added `experiments/`.
- `cce005f5` (2026-03-21) reorganized source into OS-layer directories.
- `3ec0fea8` (2026-03-30), the most recent ancestor modification of this
  validator, replaced old directories with layered paths and removed `src/`
  and `experiments/`. Those removals cannot govern today's build: root CMake
  now calls `add_subdirectory(src)` and refers to multiple experiments.
- `c36ccaff` (2026-04-01) refactored CMake layout without changing build
  behavior. `src/` currently contains real CMake modules and Direct API source,
  not merely residue from an abandoned directory migration.
- `8d56582d` (2026-04-04) added the organization guide, report, commercial/demo
  directories and development-test grouping. Their deliberate commit establishes
  lineage, but does not establish unanimous root approval or validate their
  maturity claims.
- `2c185fbd` and `cc74c53e` (April 4–5) added OpenClaw scripts and subsequent
  integration artifacts. Later research commits actively extend `experiments/`,
  through `01c85019` (#569).

`PROJECT_STRUCTURE.md` describes only a subset of the present tree and uses
promotional maturity language. It cannot independently authorize all root
entries. Conversely, the validator cannot invalidate live CMake inputs merely
by omitting their names. This is a partially reconciled governance migration,
not evidence that the source migration should be reversed.

## Missing npm manifests

`cb54c175` (2026-04-04), titled “cleanup: remove experimental CLI tools and
temporary files”, explicitly deleted both manifests. Before deletion,
`package.json` was a private metadata/docs manifest with one script,
`docs:search: node docs/site/search/build_index.js`, and no dependency section.
The lockfile described the root package with no external package inventory.

The current documentation workflow runs npm **inside `docs/site/search`**:
it uses a local lockfile if present, otherwise installs pinned Cheerio and Lunr
versions, then invokes `node build_index.js` directly. It even comments that no
search/root lockfile is checked in. This is not a root-manifest consumer.
The VS Code extension has its own manifest under `tools/vscode-t81/`.
Root CMake, Docker publication and Python wheel packaging do not consume a root
npm manifest; Python uses `pyproject.toml` and CMake. The captured searches cover
workflows, scripts, CMake, Dockerfile and Python packaging.

Evidence therefore supports removing these **root** manifest requirements in a
future correction. It does not support restoring placeholder files or claiming
that JavaScript tooling has disappeared.

## Every unexpected entry

Classifications describe observed use, not new authority or maturity guarantees.
Full tracked file lists, addition histories and last changes are in
[entry-history.json](entry-history.json). History abbreviations below resolve
against the full-history baseline checkout.

| Entry | Classification | Evidence and limits |
| --- | --- | --- |
| `src/` | Required build input; active production source | Root CMake unconditionally adds it; its modules define core/VM/FFI/CLI libraries, and `src/direct_api/t81_direct_api.cpp` is live. Also contains older development programs; do not infer all 19 files are production. |
| `experiments/` | Active research or experimental source | Root CMake has architecture/provenance targets and radix, packed-matvec and H04-R02 subdirectories; 131 tracked files include protocols, raw evidence and tests. Latest change is #569. Preserve negative results. |
| `development_tests/` | Required build input, mixed with generated artifacts | `src/direct_api/CMakeLists.txt` builds `test_real_direct_api.cpp` unconditionally. The 12 tracked files also include executables and `ctest.log`; retain source, resolve outputs separately. Added by `8d56582d`. |
| `skills/` | Active research or experimental source/documentation | 79 tracked OpenClaw skill/script files from `2c185fbd`, extended through `6222c821`; scripts use CLI operations. Not a CMake dependency or proof all advertised operations work. |
| `PROJECT_STRUCTURE.md` | Active documentation with stale scope | Added by `8d56582d`; describes the April grouping but omits much of the runtime/research tree. Descriptive, not a complete authority contract. |
| `SYSTEMS_AND_SUBSYSTEMS_REPORT.md` | Historical report; retention intent unresolved | Dated April 4 and added by `8d56582d`; contains analysis and maturity claims, not an executable layout contract. Do not silently promote its claims. |
| `customer_tools/` | Research/demo support source | Four ROI calculator Python files in the April organization commit. Presence establishes tooling, not validated financial or clinical claims. |
| `market-demos/` | Research/demo source, mixed with generated output | April commit groups demo C++ programs and a tracked `customer_demo_system` binary. No evidence establishes a core production dependency. |
| `execution_reports/` | Historical documentation; retention intent unresolved | Four verification/weekly reports grouped by `8d56582d`. No evidence supports discarding them as reproducibility-irrelevant. |
| `market_analysis/` | Historical documentation; retention intent unresolved | Two roadmap documents from `8d56582d`. Not build/packaging input. |
| `strategic_docs/` | Historical documentation; retention intent unresolved | Three strategy documents grouped by `8d56582d`. Not a normative root contract. |
| `build.log` | Generated artifact candidate that should not be tracked absent evidence-retention designation | Ninja build transcript; addition history includes `53efb64b`, last changed by `4299504f`. No curated evidence contract identified. Do not delete solely on this classification. |
| `import.apl` | Unresolved: sample policy versus scratch input | Wildcard model policy added by `cc74c53e`; no demonstrated requirement for this exact root path. Content is not evidence of a production policy default. |
| `tiny-model-policy.apl` | Unresolved: sample policy versus scratch input | One explicit model hash, same April integration commit. No demonstrated current consumer requiring root placement. |
| `record_hash.txt` | Unresolved: generated provenance output versus retained evidence | Contains a `sha3-256:` reference, added by `cc74c53e`. Deleting it without establishing its referenced artifact could lose lineage. |
| `symbolic_algebraic.tisc` | Unresolved: compiled fixture versus generated experiment output | Binary bytecode from `cc74c53e`; symbolic/algebraic strings identify its experiment, not its retention policy. |
| `symbolic_algebraic_exported.tisc` | Unresolved: exported evidence versus redundant output | Byte-identical to the preceding file (SHA-256 `50b832c009157eb87659a2d675289f54996228ef398ffaa43281e0ca37908097`). An original/export pair can be intentional evidence, so equality alone does not authorize deletion. |
| `test_input.txt` | Unresolved: fixture versus scratch input | Contains `What is 2+2?`; added by `cc74c53e`. No established exact-path consumer. |
| `test_record.json` | Unresolved: fixture versus scratch record | Example host-action record with test references and `TEST_EXAMPLE`, added by `cc74c53e`. Not demonstrated production provenance. |

These categories answer the initial alternatives: stale validator **yes**;
partially completed governance migration **yes**; conflicting descriptive and
operational contracts **yes**; tracked generated output **yes**; untracked-root
explanation **no**. Actual unauthorized additions or accidental commits remain
possible but are not established merely by their names or presence in history.

An additional control problem is outside the unexpected list: the allowlisted
root `Makefile` begins “CMAKE generated file: DO NOT EDIT!” and contains old
source paths and machine-specific build references. Its last change is
`f5e81bae`. Thus passing this allowlist would not prove generated-artifact hygiene.
The separate artifact checker only rejects Python caches, OS debris and bytecode
suffixes; its passing result does not settle logs, binaries or research outputs.

## Competing remedies and decision

| Criterion | A: update validator alone | B: relocate/remove rejected entries | C: reconcile the layout contract first |
| --- | --- | --- | --- |
| Architectural correctness | Justified for deleted manifests and required build paths; unsafe for every remaining entry | Moving `src/` or required tests breaks CMake; deletion requires stronger evidence | Document required build paths and distinguish historical/research retention explicitly |
| Scientific reproducibility | Blanket acceptance obscures fixture/output intent | Highest risk for raw evidence and original/export pairs | Preserve bytes/hashes and lineage pending disposition |
| Data/provenance risk | No immediate loss, but legitimizes potentially accidental outputs | High without a reference/retention audit | Lowest immediate risk; no relocation yet |
| Maintenance | Small patch, continued ambiguity | Many path/documentation changes | Small initial decision record, then narrow validator patch |
| CI effectiveness | Could become green by accepting accidental state | Can remain strict but break live consumers | Exact typed contract with reviewed exceptions can remain strict |
| Documentation compatibility | Leaves contradictory organization/ownership documents | Requires broad updates | Update only the conflicting layout statements |
| Negative-test sensitivity | Exact names help, but current checker misses type drift | Must test missing inputs and moved consumers | Add explicit rejection cases after authority is settled |

**Select C as the next approach, but do not implement speculative dispositions.**
A complete justified correction requires a recorded decision about which April
reports/demo groupings and integration outputs are intentionally retained at
root, which are preserved fixtures elsewhere, and which are disposable outputs.
The decision should also reconcile stale directory ownership with actual CMake
inputs. A Git commit or README description alone cannot settle this.

Once that decision exists, the minimum implementation is an explicit typed root
contract, removal of the obsolete root npm requirements, and narrowly supported
changes for the approved entries. Preserve required `CMakeLists.txt`,
`CMakePresets.json`, `pyproject.toml`, source and policy/spec inputs; do not add
wildcards or broaden the architecture. Any relocation must retain hashes and
update verified consumers. No runtime, ISA, numerical contract or result change
is needed.

The future regression suite must accept an independently specified minimal
valid layout and reject unknown files/directories, missing required inputs,
file/directory and symlink substitutions, generated outputs without explicit
evidence exceptions, and reachable CMake-input drift. Include real temporary Git
repositories and committed mutations: today's validator reads **HEAD**, not the
index or filesystem, and compares names only. An uncommitted add/remove test
would not exercise its contract. This also explains why changing files locally
cannot establish a successful committed-layout result.

No positive/negative implementation tests are added now: Phase 6 is conditional
on a justified correction, and the request's ambiguity stop condition applies.

## Complete inventory and limits

All **27 commands** were independently executed from clean exact #577 source,
with exit codes and full stdout/stderr in [inventory.json](inventory.json).
[inventory.md](inventory.md) compares every command with the prior inventory:
**18 passing / 9 failing**, exactly reproduced. No skip or timeout is a pass.
The tree stayed clean through those executions; metrics generated only ignored
outputs. Audit documentation was written afterward.

There is no 19/8 result because no corrective implementation was made. The
required gate remains red. Full-history translation staleness reproduces the
recorded 70-commit lag; shallow hosted history can differ. No new hosted checks,
build success, cross-platform execution, merge, push, PR rewrite, lock
regeneration, protection change or scientific conclusion is claimed. The
original workspace is unchanged; this branch contains only this investigation.
