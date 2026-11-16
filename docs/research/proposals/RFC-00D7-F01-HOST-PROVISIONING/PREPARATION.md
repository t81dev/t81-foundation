# Planning preparation record — 2026-11-16

The initial record below describes reviewed a2f49d7b. Its60/50 phase arithmetic
is superseded by the targeted timeline correction appended below; the underlying
observed historical validation output is retained rather than rewritten.

Planning/documentation only. No Linux host/account/AMI was inspected through an
API, selected, launched or modified. No package/archive was downloaded or
installed, interpreter built/executed, Linux probe or metadata diagnostic run,
bootstrap/supervisor/runner invoked, native code loaded or A–F/proof/fixture
executed. No active lock, accepted source/profile, schema, semantic or hold changed.
Local existing documentation validators are used only to inspect this proposal;
no target CPython/provider code is executed. All target/tool/approval observations
remain null and linux_relock_required remains true.

Read the accepted #809 source/strict import boundary and #810 exact profiles,
requirements/config/source records. Repository release workflow labels supply
context only, not CI results or observed host capabilities. Consulted official
AWS general-purpose instance specifications, Canonical image-selection guidance
and systemd255 API/version history; linked in PLAN.md. These are candidate
selection inputs, not target inventory or pricing/availability guarantees. No
cloud credentials or service APIs were used. The current agent also authored
those earlier artifacts; no independent scientific validation is claimed.

Created a separate documentation-only worktree from exact accepted profile head
0a369b8c7ce00979c970389e02e0da8d89d6ab0a. Proposed X1/A1 configurations remain
unselected; actual image/account/host/builder/installer/tool identities and every
approval/receipt are deliberately unfilled. Provider/build acquisition and the
single bounded offline operation are separate scopes, not hidden setup costs.
A metadata-only future diagnostic is described, not implemented or run.

Validation procedure uses only existing preparation tooling: parse JSON with
duplicate-key rejection; verify every pinned Git blob/SHA-256 and current source
identity; compare both accepted trees unchanged; check resource/phase/evidence
arithmetic and false/unfilled authority templates; verify packet SHA256SUMS;
run the Commons proposal shape/reference validator and staged whitespace check.
No new test, installer, probe, hashing implementation or diagnostic is supplied.
These checks do not establish runtime resource fit or capability compatibility.

Validation outcomes are recorded below after execution. SHA256SUMS covers all
packet files except itself; Git covers the final manifest and this record.

Observed planning validation (exit0):

```json
{"accepted_profiles_unchanged":true,"accepted_source_unchanged":true,"evidence_mib":25,"linux_relock_required":true,"phase_cpu_seconds":50,"phase_wall_seconds":60,"proposal_json_files":4,"source_pins_verified":24,"storage_mib":921,"target_actions_executed":false,"unfilled_receipt_templates":4}
```

The inline local data check compared24 git-show bytes and blob IDs with pinned
hashes and current files, parsed four JSON files with duplicate rejection, checked
60/50 phase sums,25MiB evidence reservations and921MiB proposed storage within
1GiB, required false/unfilled authority fields, and compared both accepted source
trees unchanged. It did not call a target interpreter or host/contract function.

Commons command (existing documentation tool/environment; exit0):

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B tools/research_commons/commons.py --validate-contribution docs/research/proposals/RFC-00D7-F01-HOST-PROVISIONING/proposal.json
```

Result: valid_proposal_shape; execution unavailable; shape/reference checks
establish neither human authorization authenticity nor scientific acceptance.
After this record and the approval-order clarification were written, final packet
hashes were regenerated and verified with shasum -a256 -c SHA256SUMS. Staged
whitespace checking uses git diff --cached --check. No Linux capability, native
ABI, installation fit, runtime limit or cleanup behavior is verified by those
checks. All current receipts remain templates, not observed outcomes.

One documentation append initially used a repository-relative path from inside
the packet directory and failed before writing. Existing hashes still verified;
the append was then applied using the correct local paths and hashes refreshed.
This was a drafting/path failure, not a host or provisioning observation.

## Targeted timeline/enforcement correction — static documentation only

Review baseline: a2f49d7b4a6d89c743a7192dabaf95d727cc5a20. P0–P4 now total
44s wall/43s CPU, FINAL reserves10s/5s, planned quiescence is54s/48s, and the
60s/50s outer ceilings remain. T0 includes startup; Ta-T0<=1s is required.
The55s active-runtime timer expires55–56s after T0, so54s quiescence leaves at
least1s. Required expiry-to-quiescence<=2s gives latest58s, with2s remaining
outer wall margin. The final write cutoff53s/47s leaves1s/1s guard-response
reservation inside FINAL; all quantities are candidate limits, not evidence of
enforcement or fit. Actual earlier FINAL entry shortens, never resets, bounds.

Source-inspected systemd v255 service_running_timeout and unit_arm_timer and
its service timeout documentation to distinguish active-enter/monotonic origins
and repeated stop/extension paths. Exact target manager remains unselected; no
source fact is claimed as host-observed behavior. The source/function references
are linked in PLAN.md. No source copy, provider/interpreter download, host API,
probe, install, build, experimental function or diagnostic was executed.

The plan explicitly requires a reviewed independent CPU/wall guard able to stop
a blocked/dead finalizer. cpu.max, per-process RLIMIT and RuntimeMax alone do
not demonstrate cumulative50s CPU enforcement. Guard source and target latency
receipts remain null. Controller-only FINAL, helper stop/reaping first, pre-kill
checkpoints, bounded existing-manager record export and absence of any guaranteed
post-kill terminal receipt are now explicit. No extra observer or post-budget
finalizer is introduced. No resource or existing evidence schema changes.

Validation uses existing local documentation/data tools only: source-pin/blob
checks, duplicate-key JSON parsing, candidate arithmetic, unchanged accepted
source/profile comparisons, false/null template checks, Commons shape validation,
packet hashes and staged whitespace. No target capability, timing or persistence
claim follows. Corrected results are appended after checks.

Corrected local JSON/source/arithmetic validation (exit0):

```json
{"FINAL_wall_cpu":[10,5],"accepted_source_profiles_unchanged":true,"enforcement_implemented_or_tested":false,"evidence_mib":25,"linux_relock_required":true,"memory_leaf_mib":[256,128,64],"minimum_manager_wall_margin":1,"outer_wall_cpu":[60,50],"planned_quiescence_wall_cpu":[54,48],"required_latest_manager_quiescence":58,"source_pins_verified":24,"storage_mib":921,"unfilled_receipt_templates":4,"work_wall_cpu":[44,43]}
```

Checked every phase/prefix sum,53/47 write cutoff and1/1 response reserve,
activation+runtime55–56 and required2s fallback arithmetic, unchanged memory/
swap/affinity/evidence/storage values, null enforcement/guard/authority evidence,
24 pinned source/blob hashes, unchanged #809/#810 trees and retained active
linux_relock_required=true. These assertions check proposed numbers and source
identity, not any implementation, host, timer, CPU guard, kill, fsync or orphan.

Existing Commons validation again returned valid_proposal_shape with execution
unavailable and its authority/acceptance limitation. Packet SHA256SUMS was
refreshed and all seven entries verified. Staged whitespace checking passed.
The original numerical record above remains historical evidence of the reviewed
plan, not the corrected operative budget. No new executable file or test/probe
was written or run, and no existing accepted source/lock/schema was modified.
