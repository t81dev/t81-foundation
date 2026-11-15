# Operational Capability and Agent Skills Audit

**Date:** 2026-10-27. **Role:** implementation engineer/auditor (Codex; exact
underlying model identity unrecorded). **Pinned main:**
`fabc7aa90f70ead227ec257a1ae6833fa16ced40` (merged PR608). This is a bounded
operational investigation, not a security certification or independent scientific
replication. No production code, skill, accepted spec, governance activation,
canonical research outcome, PR609 or PR610 was changed.

The 12 skill packages describe a much broader API than their implemented
adapters and current CLI support. **Do not expose the current shell wrappers to
untrusted external-agent arguments.** Two high-severity boundary findings were
reproduced with benign fixtures: shell expansion before CLI policy processing,
and requested CLI policy not reliably binding execution. These findings coexist
with passing VM/CanonFS tests on different call paths.

## Inventory and evidence interpretation

The [command matrix](AGENT_SKILLS_COMPATIBILITY_MATRIX.md) enumerates every
advertised command, syntax variant, wrapper and intended handler. The
[machine-readable inventory](evidence/operational_capabilities.json) includes
permissions, side effects, specs, classifications, tests, evidence refs, full Git
pins and source SHA-256 values. Counts are deliberately nonexclusive:

| Observation | Count and exact scope |
| --- | --- |
| Skill definitions inspected | 12 |
| Advertised package/command names | 69; 71 syntax variants because breakpoint has three forms |
| Shell files inspected and syntax checked | 66; 12 main wrappers and 54 delegators |
| Missing advertised wrapper entries | 3: import-batch/list in canonfs-import; validate-batch in bundle-validator |
| Intended CLI paths with matching registered handlers | 8/69, traced in source and exercised as missing-input negative probes |
| Intended CLI paths without matching handlers | 61/69; no functional implementation inferred from adjacent code |
| Successful direct CLI operation types | 11 on benign scratch fixtures, plus two wrapper invocations of overlapping import/build operations |
| Real wrapper samples that succeeded | Bare import and bare build only, on macOS Bash3.2; neither is approved for untrusted arguments |
| Advertised contracts labeled BROKEN | 68/69, including partially working build with incompatible flags/JSON promises; bare import remains unsafe rather than classified absent |
| Wrapper capabilities without a successful real operation sample | 67/69, labeled UNVERIFIED for operational success; not disjoint from BROKEN |
| Existing selected CTests | Five passed; one failed at missing-fixture precondition |
| External output-schema checks | Model-import sample passes; all five retained CanonFS result samples fail their companion RFC schema |
| Scientific replication / comparative advantage established | 0 |

Labels SPECIFIED, IMPLEMENTED, TESTED, MEASURED, COMPARATIVELY_VALIDATED,
EXPERIMENTAL, STUBBED, BROKEN and UNVERIFIED are evidence properties, not a
maturity scale. No measured/comparatively validated advantage is assigned.
TESTED means the retained local cases ran; fake-backend success is never counted
as a real operation. The collector scripts return zero on successful **collection**,
including collection of negative findings.

## Authority and investigation boundary

Inspected AGENTS, README/handoff/roadmap/buildable-next-steps/profile, both research
charters, the continuity register, CAPABILITY_BASELINE, RESEARCH_GAPS, governance
surface/determinism registers, implementation matrix, skills README, all 12 skill
definitions/66 scripts, actual main/AI dispatch, related implementations, tests,
schemas and CI references. Source hashes and line locators are in the inventory.

The prior capability/gaps reports are historical source claims, not automatically
validated findings. For example, a registered network opcode is not networking:
current blocked async/network cases return **SecurityFault with a denial log**, not
functional network execution. The selected fail-closed test passed. A neural
blocked handler and a synthetic inference fallback also exist; source inspection
of them does not demonstrate model accuracy or real generation. The latter
explicitly labels its output as synthetic.

The canonical H01–H08 register is unchanged (SHA-256
`af8ca65dcef136f294eb33010b5a6621875a52db4f3e60b94a4e28db5318a1c1`). H07
retains B/C evidence and no new security certification; H08 retains C/E evidence
and untested comparative benefit in its dated 2026-10-25 record. Numbered
investigations and operational observations do not redefine those hypotheses.

## Findings by severity

### OP-SEC-01 — HIGH: shell interpretation precedes Axion

All 12 main wrappers construct a string and call `eval`; the other 54 scripts
exec those wrappers. [import.sh](../../skills/canonfs-import/import.sh#L72) inserts a
caller-controlled filename inside double quotes, then evaluates the resulting
shell program. Unquoted values also exist, for example the bundle creator's
execution-result list. Ordinary Bash3.2 execution of an existing scratch file
literally named `input$(printf AUDIT_CANARY)` passed `inputAUDIT_CANARY` to a fake
`t81`. Thus command substitution was interpreted rather than preserved as data,
before any CLI/Axion authorization. Only harmless `printf` was used.

The separate conditional-source probe demonstrates the same eval behavior under
an explicitly different errexit context; it is not substituted for the ordinary
entrypoint evidence. No destructive payload, remote service, user file or real
model was involved. This is host-shell exposure, not a demonstrated weakness in
CanonFS hashing or VM policy evaluation. See `eval-ordinary-execution` and
`eval-conditional-source` in the wrapper receipt.

### OP-SEC-02 — HIGH: requested execution policy is not reliably bound

[Global argument parsing](../../tools/cli/core/main.cpp#L2380) consumes `--policy`
into `Args::policy`. [run_vm_command](../../tools/cli/core/main.cpp#L6967) initializes
a separate empty `policy_path` and only checks remaining command arguments; it
does not initialize it from `args.policy`. In the retained probes, both a missing
policy and an existing invalid policy were ignored by `t81 vm run`: the benign
program printed `47` and exited 0 without a policy error.

The separate [run_tisc](../../tools/cli/core/driver.cpp#L1408) branch logs an unreadable
policy but continues loading/running the program. `t81 code run` with a missing
policy printed the error, then `47`, and exited 0. These tests used only a print
program, not a privileged operation. They establish that callers cannot rely on
these CLI options as the intended policy boundary; they do **not** establish a
universal VM/Axion bypass. The VM pre-dispatch deny-logging test passed because it
installs and exercises policy through another path.

### OP-COMP-01 — MEDIUM: delegators erase the selected operation

The 54 alternate entry scripts use `exec path/to/main.sh "$@"`. Their main scripts
dispatch on `basename "$0"`, which now identifies the main script. For example,
[disasm.sh](../../skills/t81-compiler/disasm.sh) selects build, and
[demo-policy.sh](../../skills/governance-demo/demo-policy.sh) selects governance.
All six zero-argument governance probes reached `demo governance --json` in the
fake backend; five were the wrong advertised mode. This is not a symlink-based
mode-preserving dispatch. Fixing quoting alone would not fix it.

### OP-COMP-02 — MEDIUM: missing routes and mismatched syntax

The 61 unsupported intended paths include every advertised generic bundle,
model-loader, debugger-session, tensor-operation and AI task route. Real bounded
AI tasks are named `assess-fixed`, `route-fixed`, `classify-fixed` and require
explicit model/policy/input options; `assess` is not an alias. Actual model
intake is `model import`/`diff`, not Hugging Face download/security-scan/conversion.
Actual tensor CLI is canonize/hash/inspect, not standalone tensor math/reshape.

`code build --optimize 2` and `code disasm --format binary` fail with multiple-input
errors; `canonfs export --verify` is rejected. Native `axion simulate` runs a
`.tisc` program, so it must not be exposed as the advertised no-effect policy
simulation. The three missing wrapper files are retained as missing findings.
All 69 intended routes were probed without input to avoid executing arbitrary
programs; their errors and source traces are retained separately from successes.

### OP-SEC-03 — MEDIUM: host mutation is not an agent capability boundary

Import/model/export/policy/bundle-validation wrappers call `mkdir -p` independently
of backend policy. The missing-policy import-wrapper probe created its scratch
store directory before rejecting the policy file, without invoking the fake CLI.
Paths are caller-selected; no workspace containment, OS sandbox or host resource
allowlist is established by these scripts. Path traversal/symlink attacks outside
scratch were not attempted.

Native CanonFS `deny-all` correctly refused sampled payload import and export
materialization, but each denial still wrote one provenance object. The receipt
identifies those references, and [import_path](../../fs/canonfs_interchange_ops.cpp#L395)
and export's provenance tail explain the writes. Do not equate payload denial
with an entirely read-only filesystem operation. These receipts are not silently
called unauthorized payload writes, and no CanonFS identity contract is amended.

`T81_POLICY_PATH` is advertised in the README but not consumed by these wrappers.
Native root discovery also differs by surface: main.cpp's helper searches cwd/repo,
while some driver paths consult the environment. Set explicit allowed roots on
operations that support them; an environment variable is not a global sandbox.

### OP-COMP-03 — MEDIUM: shell/runtime portability defects

All five VM runner entrypoints aborted on the unset `RECURSIVE` variable before
calling the fake CLI. The main wrapper reads it under `set -u` without default
initialization. The ordinary wrapper probes used Apple's Bash 3.2.57; 61 other
entrypoints reached the fake CLI, which is not proof of their real compatibility.

All main parsers also increment zero using `((i++))` under `set -e`. Current GNU
Bash arithmetic/errexit semantics make that nonzero result an earlier exit risk.
This is a source/manual-based portability inference, not an executed Linux
result. See the [GNU Bash set builtin](https://www.gnu.org/software/bash/manual/html_node/The-Set-Builtin.html)
and [conditional constructs](https://www.gnu.org/software/bash/manual/html_node/Conditional-Constructs.html).
Linux, Windows/WSL and other shell versions were not run. The skills themselves
advertise only darwin/linux. No portability pass is inferred from C++ CI alone.

### OP-CLAIM-01 — MEDIUM: advertised assurance/output exceeds the tested surface

Eleven main parsers do not accept a caller's `--json` option although several
examples advertise it; most append a JSON flag internally. Bare compiler build
returns human-readable success text even with its internal flag. No wrapper
validates the advertised JSON output templates. Duplicate `--output` cases in
model-loader/bundle-creator shadow later fields. AI wrapper output filenames are
incorrectly checked as pre-existing inputs. Model quarantine/security scanning,
complete provenance/authenticity and session APIs are not established by these
wrappers. The VM skill's temporal-predictability claim does not follow from
bit-exact execution; wall-clock timing is out of deterministic scope.

### OP-CONTRACT-01 — MEDIUM: runtime and companion JSON Schemas disagree

External JSON Schema validation checked all six retained documents with matching
RFC schemas: the model-import sample passes, but all five CanonFS success/error
samples fail. The RFC import/export schemas reject emitted `provenance_schema`
and `manifest_schema` fields under `additionalProperties: false`; error entries
also reject emitted `reason`. The runtime's own validators accept these documents.
Passing an internal check is therefore insufficient evidence of companion-schema
conformance. The full outcomes are retained in
[schema-validation.json](evidence/operational-audit/schema-validation.json).
No output, accepted contract or schema was altered to hide the mismatch. The
first audit-test attempt exposed this error; the evidence regression check now
replays the recorded failures, rather than asserting that the contracts conform.

### OP-REP-01 — LOW: a missing fixture blocks existing CLI contract coverage

`t81_cli_contract_test_runner` failed at
[cli_contract_test.cpp:491](../../tests/cpp/cli_contract_test.cpp#L491):
`models/tiny-random-llama-exported.safetensors` is absent in the clean checkout;
`models/` is ignored. Its assertions after that precondition did not run. No
replacement fixture was inserted into production test paths to make it green.
Manual CLI parity passed (24 help topics), but that checker does not cover skills.

## Actually exercised native paths

These eleven operations succeeded on self-authored scratch fixtures:

| Public operation | Actual path / observed scope |
| --- | --- |
| `code build` | normalize_domain_command → t81::cli::compile → frontend/program build → save_program; writes `.tisc`; success output text |
| `code disasm` | normalization → disasm_tisc → load_program; text disassembly, not the wrapper's format options |
| `vm run` | run_vm_command → run_tisc → interpreter; benign print only, explicit policy caveat above |
| `vm trace` | same execution path plus trace file; execution, not read-only discovery |
| `vm state` | run_vm_command state path; `t81.vm-state.v1` on one benign artifact; not a generic safe untrusted-code inspector |
| `policy validate` | run_policy → run_policy_validate → policy parser; `t81.policy-validate.v1` |
| `canonfs import` | canonfs_import → import_path; payload, manifest and provenance writes; `t81.canonfs-import.v1` |
| `canonfs stat` | canonfs_stat → content-addressed metadata; existing scratch store, no sampled new writes |
| `canonfs verify` | canonfs_verify → object integrity read; no authenticity claim |
| `canonfs export` | canonfs_export → export_ref; host materialization and provenance; `t81.canonfs-export.v1` |
| `model import` | run_model_import → tools/model intake; minimal local SafeTensors metadata, `t81.model-import.v1`; not model execution |

The two actual wrapper samples are bare compiler build and CanonFS import on
Bash3.2. Fake-CLI captures, errors, intended routes and actual operations have
separate IDs in the raw receipts; an exit-zero negative policy probe is recorded
as a defect, not counted as a successful secured operation.

## Scientific adapter integration: current versus proposed

- **Current:** T81Lang compilation emits TISC; native VM execution/trace paths
  exist; CanonFS interchange links payload/manifest/provenance; fixed-task
  implementation checks policy and stores typed results. Source and selected
  tests support those scoped paths, with the CLI policy-binding defect above.
- **Current, separate draft:** Research Commons PR609 at
  `43df109b01fc88f67375f18cd41ff563982830b3` exposes offline pinned discovery.
  `--validate` and a fresh H08 lookup ran successfully during this audit. This
  does not put Commons on main or independently validate research claims. Its
  canonical C/E H08 result still says comparative benefit is untested.
- **Not implemented:** No wrapper-to-Commons scientific experiment execution
  adapter, common experiment receipt protocol, generic safe capability transport,
  MCP server, host permission broker, or automatic independent reviewer exists
  through these skills. Their filenames and templates are not such integration.
- **Proposed only:** Reuse explicit argv-based native inspection after review;
  link inputs/outputs to commit/hash manifests and separately authorized CanonFS
  storage; bind requested policy before execution; record exact environment,
  checks and uncertainty. Keep discovery separate from privileged execution.
  A receipt proves recorded bytes/check outcomes within scope, not truth or
  independent scientific replication. No proposal here grants execution rights.

## Recommendation and smallest next step

First expose **reviewed offline read-only evidence discovery** (after its draft
acceptance), followed by bounded metadata inspection of allowlisted local files,
such as native `model import` without output flags and existing-store stat/verify.
Reconcile the CanonFS companion-schema mismatch before claiming schema-conformant
interchange output.
Use process-level no-network/write restrictions and resource limits; these are
requirements for a future adapter, not capabilities supplied by current scripts.
Do not expose the generic skill directory as a trusted execution API.

Remediation priorities, each requiring separate implementation authorization:

1. Bind CLI execution policy correctly and fail closed on unreadable/invalid
   requested policy; add matching CLI-boundary tests, not only interpreter tests.
2. Select one read-only metadata adapter; use an argument array, explicit mode,
   constrained files, limits and validated structured output. Test hostile-but-
   harmless arguments, invalid flags and shell/platform differences before release.
3. For a separately approved mutating adapter, make store/output permissions and
   denial-metadata writes explicit. Then repair one wrapper's dispatch/parser,
   not all twelve at once. Narrow/remove unsupported command promises.
4. Reconcile runtime/companion CanonFS schemas under the existing contract review
   process; preserve the failing documents as negative fixtures.
5. Supply the CLI contract fixture reproducibly with a pinned license/identity,
   or make its precondition an explicit supported baseline; rerun the blocked suite.

The smallest safe next implementation is a reviewed **single metadata adapter
and its boundary tests**, after resolving the two high-severity exposure findings
for any execution path it would use. This audit implements none of that work.
Independent human acceptance must assess severity, evidence completeness and
remaining uncertainty before any external-agent exposure or merge.

Severity tally: two HIGH, six MEDIUM and one LOW findings. The MEDIUM/LOW rows
include operational, compatibility and reproducibility findings, not nine proven
security exploits. Observation snapshots track scratch filenames/content hashes;
they do not instrument transient writes, same-byte overwrites or all OS calls.
