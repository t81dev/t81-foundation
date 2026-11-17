# Proposed PR #811 applicability — NON-NORMATIVE

**CANDIDATE AMENDMENT — NOT ADOPTED.** This document proposes replacement
assurance/applicability language only. It does not edit or adopt PR #811's
operative candidate plan or authorize any target activity.

Pinned plan: `d0edeb486993860d733abc04b2ce740b35442874`.
Decision memo: `473770c7a4aaf83327eaaededd1de14bad177e4c`.
Proposed model: F01-HOST-BOLQ-v1; language is contingent on future human adoption.

## P1 — Proposed assurance applicability text (NON-NORMATIVE)

> PR #811's provisioning timeline and controls are a distinct host activity,
> not the A–F experiment. If a future revision expressly adopts F01-HOST-BOLQ-v1,
> its shared-leaf survival and termination/reaping intervals become prospectively
> declared observed criteria for named finite fault scenarios, not universal
> prevention or response guarantees. Source inspection, policy settings and
> accepted arithmetic are not target enforcement measurements. Missing required
> admission/coverage/evidence still blocks the requested scope. The original
> UNSUPPORTED assessment remains valid for its unchanged full-guarantee claim;
> this different proposed claim does not refute or rewrite that finding.

## P2 — Proposed cumulative timeline interpretation (NON-NORMATIVE)

| Phase | Phase wall/CPU | Cumulative wall/CPU from original origin |
|---|---|---|
| P0, including all custom startup |4s/3s |4s/3s |
| P1 |10s/10s |14s/13s |
| P2 |14s/14s |28s/27s |
| P3 |13s/13s |41s/40s |
| P4 |3s/3s |44s/43s |
| FINAL, including stopping/sealing/exit |10s/5s |54s/48s |

> Retain CLOCK_MONOTONIC T0 before any custom work, fresh absolute D/cpu.stat
> origin, actual active-enter Ta and0<=Ta-T0<=1s. The55s active-runtime manager
> expiry therefore falls in[T0+55,T0+56]. Planned54s quiescence leaves at least
> 1s to earliest expiry, conditional on the declared premises; it is not observed
> safety. Preserve stop/write cutoffs min(F+9,T0+53) wall and min(U_F+4,47) CPU,
> FINAL bounds min(F+10,T0+54) / min(U_F+5,48), and observed1s wall/CPU response
> and2s manager-expiry-to-quiescence comparisons. Required latest fallback58s
> remains a measured criterion, not an assumed host property. All startup,
> observation, in-flight stop, reaping, persistence and control-process exit costs
> consume the same absolute envelope. Late detection shortens it. No reset/retry.

Candidate acceptance still requires60s whole-operation wall/50s cumulative CPU,
256MiB aggregate,128MiB helper,64MiB total controller/guards,zero swap,one CPU,
and25MiB total evidence. Proposed provisioning storage remains128MiB inputs+
512MiB payload+256MiB staging+25MiB evidence=921MiB within1GiB. This is not an
A–F storage increase. The current1MiB manager-record export reserve remains
inside25MiB, not a post-kill extra allowance. No quota or phase number is changed.

## P3 — Proposed stopping/finalization applicability (NON-NORMATIVE)

> The controller is the only finalizer after confirmed helper termination/reaping.
> A separate guard in the64MiB leaf does not establish OOM/reclaim/scheduling
> independence. Lost control/rights/progress stops admission and invokes only
> remaining authorized stop paths. Actual deadline/CPU/containment violations
> are FAILED; unknown attempted completion is INCOMPLETE; unexercised coverage
> is NOT VERIFIED. No new phase/observer/CPU/privilege/leaf or fresh FINAL allowance
> is granted. An expected abort test can PASS only its predeclared assertion with
> complete evidence, not the interrupted installation. Manager termination does
> not promise a receipt or allow a replacement finalizer. Retain confirmed
> checkpoints, partial data and any ordinary existing manager evidence, with
> actual observation and persistence scope. Do not describe detection as prevention.

## P4 — A–F separation and adoption boundary

Provisioning remains offline materialization/inventory with distinct permissions.
It grants no model enumeration, oracle/proof calls, native load/stress probe or
A–F authority. The qualification vocabulary in A4 and the receipt dictionary
cannot replace scientific F01 result labels or make an incomplete search pass.

A–F retains its area wall caps5/5/10/10/10/10s, visited-state ceilings,128MiB
runner,49,408 suite states,60s/50s,256MiB,one CPU and25MiB. #809's source guard
values58s/48s and finalization59s/49s remain untouched; do not copy this plan's
53/47/54/48 into it. Host metadata is not modeled logical time, and host fsync/
reaping probes are not the simulated Area E journal or any new experimental path.

A future amendment to the research protocol is necessary before A–F may rely on
this new host-assurance model. It needs complete new protocol bytes/hash and
reviewed receipt/gate/source/identity migration, not a reinterpreted old approval.
Source acceptance for #809 and input-profile acceptance for #810 remain bounded
to their actual bytes and scope. The #811 design and #812 negative conclusion
remain historical evidence, not silently upgraded to qualification.

Current GO is documentation review only. Provisioning, compatibility diagnostics,
lock activation, qualification and A–F remain STOP pending separate decisions,
actual inputs/tooling/host evidence and applicable exact identities. The present
drafting authorization is not any of those decisions; linux_relock_required=true.
