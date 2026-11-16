# Proposed Linux qualification and authorization procedure

**Procedure only. Not run. No qualification or A–F execution is authorized.**

1. Independently review the clarified protocol, harness diff, candidate/oracle
   separation, control expectations, admissibility table and state reservations.
   Resolve preparation defects before pinning a reviewed commit. Do not resolve
   a disputed RFC meaning through code; retain both interpretations with their
   provenance or report `UNDERDETERMINED`. Re-review changes to interpretations.
2. In a separate preparation authorization, pin a Linux CPython interpreter,
   compatible dependency wheels/source hashes and exact installed files. Replace
   the explicitly macOS-only preparation lock and set `linux_relock_required`
   false only after independent review of the new lock. Regenerate source
   identities, repeat static-only checks and pin the resulting harness SHA.
   Installing dependencies or probing resource controls is not an A–F run and
   must not be presented as evidence about RFC contracts.
3. Obtain explicit human authorization for resource qualification probes before
   performing any. Use a pre-existing delegated cgroup-v2 subtree, never sudo,
   production cgroups or global control changes. Qualification must not call
   the A–F code or enumerate its cases. Bound qualification probes separately
   within at most the original 60-second/50-CPU-second/256-MiB ceilings; no
   repeated fresh allowances or implicit probe authorization follows here.
4. Configure a fresh aggregate cgroup with `memory.max=268435456`,
   `memory.swap.max=0`, `cpu.max` at no more than one CPU, and inherited affinity
   to one online CPU. Include the trusted supervisor and its single runner.
   Give the `runner` child cgroup `memory.max=134217728`, `memory.swap.max=0`.
   No other processes may enter these groups. Record ancestor limits,
   `cgroup.controllers`, `cgroup.subtree_control`, membership, memory/CPU files,
   kernel release, boot ID and interpreter identity. If delegation is absent,
   stop `BLOCKED`; no privilege escalation is included.
5. A separate qualified outer service-manager deadline must bound supervisor
   gate/setup/finalization as well as the child. Demonstrate bounded harmless
   allocation/CPU/deadline probes and cleanup, without A–F imports. Show that
   aggregate memory includes supervisor/runner, swap is disabled, an over-limit
   disposable allocation is killed, one active runner is enforced, and no child
   remains. Reset or account for pre-run `memory.peak` and CPU baselines. Qualify
   the proposed 58-second wall / 48-second aggregate-CPU guard margins and
   worker/supervisor RLIMIT_CPU guards (44/45 and 2/3 seconds soft/hard).
   OS scheduling is not real-time: record maximum observed delivery/cleanup
   overshoot and refuse qualification if ceilings cannot be respected. Passing
   finite probes does not prove universal host timing or malicious-host safety.
6. Review an attributable qualification receipt under the evidence schema,
   naming the reviewed harness SHA, dependency-lock hash, aggregate cgroup,
   kernel/boot identity, approver and raw qualification-evidence hash. A JSON
   boolean is not proof that limits were demonstrated. The gate checks shape,
   current identities and current cgroup files; human authenticity and receipt
   truth remain the operator's responsibility. Qualification is not authorization
   for the contract experiment. No receipt is supplied in this preparation.
7. Request a **separate explicit authorization for one A–F run**. It must name
   the reviewed harness SHA and protocol hash, the already reviewed source and
   dependency pins, allowed interpretations, all original ceilings, exact
   scratch output directory and current qualification receipt. Do not create
   an `authorized` record on a human's behalf. No broader service implementation,
   native worker, real journal, networking, credentials, sandbox test or merge
   is included. A–F must run sequentially with one runner only.
8. Only after that decision may the operator invoke the reviewed supervisor
   with authorization/qualification records. A supplied record is not an access
   control boundary against its owner. The runner consumes one shared allowance,
   including controls and rotations. Stop on failed controls, missing identity,
   cap exhaustion or enforcement failure. Preserve partial ledgers and all
   negative/inconclusive outcomes. Never retry automatically or adjust domains
   to obtain a favorable verdict. Missing terminal metadata means incomplete.
9. Independently review raw cases and summary counters before any claim. Neither
   qualification nor successful synthetic results authorize production work,
   normative promotion, Protocol 001, RFC acceptance or release/merge changes.

macOS and Windows are unsupported execution environments for this harness.
Linux x86_64 and ARM64 are both unqualified. No platform or containment result
is inferred from the preparation machine being macOS ARM64.

## FA01 qualification prerequisites

Any future receipt must name F-structural-corpus-v1 and the adopted protocol,
corpus, amendment and newly reviewed harness identities. A prior receipt cannot
authorize a changed plan. Explicitly review the five proof assumptions and
provide an attributable review reference; disputed assumptions yield a conditional
underdetermined proof, not an admission claim. The preparation macOS lock remains
unchanged and blocks execution. No probes have run or become authorized here.

## LQ01-R1 source correction supersession

The original step4 placement and step5 baseline wording above are historical.
The new implementation requires empty aggregate D and sibling supervisor/runner
leaves, fresh zero-origin accounting before launch, and G0/G1/GO startup.
See LQ01_R1_IMPLEMENTATION.md for exact preconditions and blockers. No CPU reset
or later baseline, inherited hard3 before fork, or populated internal D is an
approved substitute. Qualification and execution remain unauthorized.
