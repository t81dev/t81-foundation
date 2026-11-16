# Re-lock preparation evidence — 2026-11-16

Source-only review proposal. Same-agent relationship to earlier harness/proposals
is disclosed. No independent scientific validation, installation, interpreter or
artifact code execution, qualification probes, proof checking or A–F execution.

Read accepted dependency-lock.json/requirements.lock and strict boundary source,
LQ01 RELOCK/interpreter/two architecture profiles and LQ01-R1 dependency review.
Their historical files are unchanged; local uncommitted proposal identities are
explicitly labelled as such in source-pins.json, rather than invented Git pins.
The current preparation branch starts at accepted0afbc339. Only this new packet
is changed; the full experiments/rfc_00d7_f01 tree remains byte-identical.

## Fresh static artifact inspection

Executed only the data inspection script, using existing preparation CPython3.14.3:

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B docs/research/proposals/RFC-00D7-F01-LINUX-RELOCK/inspect_artifacts.py --fetch --prior-proposals /Users/t81dev/Code/t81-foundation/docs/research/proposals/RFC-00D7-F01-LQ01
```

It fetched five bounded official PyPI release JSON responses and six exact
wheel archives; compared metadata and prior filename/hash/size/URL identities;
verified every source RECORD member; inspected WHEEL/METADATA and native ELF
section/symbol/version data. It downloaded the exact official CPython source
archive, hashed its bytes and streamed source Lib/ members and critical build/
path configuration files into identity inventories. No extraction to a package
or executable directory, import, native loading, build hook, resolver or installer.
Archives were transient process data and are not retained; independent byte
verification requires retrieving those exact URLs/hashes again, not trusting this
report. Raw release responses and member observations are retained.

Observed exit0 output:

```json
{"artifact_code_executed":false,"host_qualified":false,"packages_installed":0,"source_archive_bytes":23778568,"source_lib_files":2405,"wheel_archive_bytes":948967,"wheel_archives":6}
```

Actual bytes hashed:24,727,535 archive bytes, plus retained registry responses.
This is input-byte accounting, not a benchmark or a measurement of a Linux host.
No wall/CPU/resource qualification is inferred. Retrieval caps are explicit in
inspect_artifacts.py; the packet is capped4MiB. Source TAR reads use extractfile
as a streaming read interface, never extract/extractall filesystem operations.
ELF parsing reads bytes only; no ldd/readelf/native loader was executed.

## Offline checks and review limitations

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B docs/research/proposals/RFC-00D7-F01-LINUX-RELOCK/validate_packet.py
/private/tmp/t81-pr795-parity-venv/bin/python -B tools/research_commons/commons.py --validate-contribution docs/research/proposals/RFC-00D7-F01-LINUX-RELOCK/proposal.json
/private/tmp/t81-pr795-parity-venv/bin/python -B experiments/rfc_00d7_f01/static_check.py
git diff --cached --check
```

The packet checker is offline: source blob/hash pin comparisons, observed-data
consistency, hash manifest, proposed profile/config/requirements consistency,
AST/compile of preparation scripts, boundary-source inspection and unchanged
active harness tree. It does not call interpreter_boundary, inspect a Linux
host, import experimental modules or recreate the observations on archive bytes.
The accepted harness checker performs its existing metadata/AST/hash work only;
its Linux no-site branch is not entered in this macOS preparation environment.
Commons validation checks proposal shape/references, never authority or findings.
No formatter, resolver or dependency was installed.

The first attempted worktree creation was denied by the filesystem sandbox;
the authorized isolated branch was then created with sandbox escalation. No
protected branch was edited. No data inspection failed and no unfavorable result
was deleted. Source/installed runtime fields that cannot be observed remain null.
Those missing identities and review decisions block activation independently of
all passing static checks. The active linux_relock_required flag remains true.

Actual validator outcomes are appended below after execution. SHA256SUMS covers
the final packet except itself; Git covers that manifest and the containing SHA.

Observed offline packet validator (exit0):

```json
{"active_harness_unchanged":true,"artifact_or_harness_execution":false,"compiled_preparation_modules":2,"host_qualified":false,"linux_relock_required":true,"native_machines":[62,183],"packet_hashes":20,"profiles":2,"record_member_observations":143,"source_pins":15,"stdlib_source_files":2405,"wheel_records":6}
```

Commons proposal validator (exit0): valid_proposal_shape, execution unavailable,
with the explicit limitation that shape/reference checks establish neither
human authorization authenticity nor scientific acceptance.

Accepted harness static checker (exit0):11 modules compiled,14 source pins,
5 active protocol artifacts,3 schema definitions,54 cross-module signatures,
8 interpretation keys,6 fixed cap bounds and16 protected baseline artifacts;
run.py exception/unchanged experiment AST and F254/256 checked. Candidate/oracle
imports/invocations and experimental states enumerated:0. Execution qualified:false.

Whitespace checks passed. The packet validator was repeated after this evidence
append and SHA256SUMS refresh, with the same successful counters. This repetition
checks changed evidence identities; it is not an experimental rerun.

The inspection script's final review interface also accepts --fetch without the
local --prior-proposals path. It then reads the two retained proposed profiles,
whose exact selections are checked offline against the same observed artifacts.
This makes the packet self-contained for an independent static byte recheck.
The earlier explicit legacy-path command above remains supported and records
the actual invocation. This interface adjustment and assertions-enabled guard
were compiled/checked only; no second artifact download or inspection run was
performed. Future rechecks should use a fresh review checkout because --fetch
refreshes inspection/metadata files and intentionally invalidates old packet sums.
