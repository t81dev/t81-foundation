# RFC-00D2: Deterministic Artificial Intelligence Operating System Target Architecture and Sequencing

**Status:** draft
**Type:** informational
**Applies-To:** cross-layer architecture planning, sequencing, stable-vs-deferred boundary setting
**Created:** 2026-04-02
**Updated:** 2026-04-02
**Author:** @t81dev
**Discussion:** internal architecture and sequencing note

---

## 1. Summary

This RFC defines the target architecture for T81 as a Deterministic Artificial
Intelligence Operating System (DAIOS) and the intended sequencing for building
toward that goal.

Its purpose is architectural clarity, not surface expansion.

This RFC does not promote new stable APIs, CLI verbs, schema ids, or runtime
subsystems. It gives the project a shared map for deciding:

- what belongs in the DAIOS direction,
- what is already real today,
- what remains explicitly deferred,
- and what order future work should follow.

The governing rule is:

- design ahead,
- implement behind proof.

## 2. Why This RFC Exists

T81 already contains enough real surfaces that a long-horizon systems direction
needs to be described more carefully than "build more until it feels like an
OS."

Today the strongest usable story is narrower:

- deterministic execution,
- policy enforcement before side effects,
- immutable CanonFS identity and provenance,
- and a small bounded AI OS-object family.

At the same time, the longer-term goal remains larger:

- a deterministic artificial intelligence operating system.

Without an explicit target architecture, those two truths can drift into each
other in unhelpful ways:

- implementation can jump ahead into broad subsystem count,
- planning can over-claim what exists today,
- and contributors can mistake a future aspiration for a current build target.

This RFC exists to keep those boundaries explicit.

## 3. Goals

- Define the major architectural layers of a credible DAIOS direction.
- Name which layers are materially real today versus still planned.
- Provide an implementation order that preserves determinism, governance, and
  provenance.
- Give contributors a way to reject off-path work without relying on maintainer
  memory.

## 4. Non-Goals

- Promoting T81's current external category claim from runtime-first to OS-first.
- Authorizing new broad implementation lanes such as orchestration, agents,
  generalized services, or hardware backend integration.
- Freezing a new public ABI, wire format, or CLI contract.
- Replacing RFC-00D1 or RFC-00D0 as the current practical work lanes.

## 5. Architectural Target Layers

The target DAIOS architecture is organized into six layers.

### 5.1 Deterministic Execution Substrate

This layer includes:

- deterministic VM/runtime execution,
- canonical data forms,
- reproducible state transitions,
- and cross-platform determinism enforcement.

This layer is materially real today.

### 5.2 Policy and Admission Layer

This layer includes:

- Axion pre-side-effect mediation,
- policy-gated admission of operations and artifacts,
- fail-closed behavior,
- and auditable governance decisions.

This layer is materially real today, though specific policy surfaces remain at
different maturity levels.

### 5.3 Immutable Artifact and Provenance Layer

This layer includes:

- CanonFS content-addressed identity,
- immutable artifacts,
- provenance records,
- and replay/evidence linkage.

This layer is materially real today and is one of T81's clearest build-against
surfaces.

### 5.4 AI Object Layer

This layer includes:

- result artifacts,
- provenance artifacts,
- downstream typed records,
- and canonical bundles as top-level governed objects.

This layer is materially real today, but only for the admitted bounded family:

- `assess-fixed`
- `route-fixed`
- `classify-fixed`

This is not yet a general AI object runtime.

### 5.5 AI Runtime Layer

This layer includes:

- bounded AI task families,
- model/runtime admission contracts,
- reusable governed execution patterns for AI-native work,
- and broader deterministic AI execution beyond the current bounded family.

This layer is partially real today.

Its currently validated surface is narrow and should be described that way.

### 5.6 System Layer

This layer includes, over time:

- services,
- schedulers,
- device and backend integration,
- operator surfaces,
- and any future hardware acceleration seams.

This layer is mixed.

Some adjacent work exists in accepted RFCs and in-progress freestanding lanes,
but T81 is not yet at the point where the system layer is the clearest adoption
story or the safest default implementation lane.

## 6. Current Reality Map

### 6.1 Strongest Current Truth

The strongest current truth is:

T81 is a deterministic, policy-gated runtime for auditable computation with
immutable provenance and a small validated bounded AI OS-object family.

### 6.2 What Is Real Enough To Build Against

Contributors should currently prefer:

- deterministic runtime and CLI stability,
- CanonFS interchange hardening under RFC-00D1,
- newcomer clarity,
- and bounded-family boringness.

### 6.3 What Remains Deferred

This RFC explicitly does not treat the following as current default build lanes:

- generalized AI orchestration,
- agent platform breadth,
- broad OS-first positioning,
- generalized hardware/backend abstraction,
- or acceleration-driven repo convergence.

## 7. Sequencing Rule

Work should be sequenced in this order:

1. deterministic substrate,
2. policy and admission clarity,
3. immutable artifact/provenance integrity,
4. bounded AI object production,
5. broader AI runtime coherence,
6. system-layer expansion.

This sequencing is normative for planning, not for marketing.

It means future work should generally prefer:

- making current layers more legible and buildable,
- proving a narrow surface completely,
- and only then widening upward into the next layer.

## 8. Decision Rule For New Work

Any significant new proposal should answer all of these questions:

1. Which target layer does this strengthen?
2. Is that layer materially real today or still planned?
3. Does the proposal strengthen an existing proof surface, or does it skip
   ahead into a broader layer?
4. Does it preserve:
   - determinism,
   - policy-before-side-effects,
   - and immutable provenance?
5. Can it be implemented without increasing subsystem count casually?

If a proposal cannot answer these clearly, it should be narrowed or deferred.

## 9. Relationship To Current RFC Lanes

### 9.1 RFC-00D1

RFC-00D1 remains the best current draft-to-code bridge in the repo.

Use RFC-00D1 when the work is about:

- CanonFS import/export,
- provenance and manifest contracts,
- policy-profile depth,
- contributor-facing examples,
- and contract hardening.

RFC-00D2 does not replace that practical lane.

### 9.2 RFC-00D0

RFC-00D0 remains design-led.

RFC-00D2 does not authorize broad TCP/IP implementation. It reinforces the
existing guidance to narrow the resolver/descriptor surface first.

### 9.3 Bounded AI OS-Object Family

The bounded AI OS-object family remains a protected subsystem.

RFC-00D2 does not authorize:

- a fourth composition by default,
- a new helper family,
- rule execution,
- orchestration,
- or broader AI runtime claims.

## 10. Relationship To Acceleration and Parallel Tracks

Related efforts such as `ternary-fabric` may inform future design work, but
they do not change T81's current stable runtime surface.

If deeper convergence is revisited later, it should occur only through a narrow
experimental backend seam behind the runtime, not by merging current repo
identities or stable public interfaces.

## 11. What This RFC Does Not Permit

This RFC must not be used as justification for:

- adding a new subsystem because it appears in the target architecture,
- broadening public claims faster than proof,
- importing interposer, scheduler, FPGA, or MLIR stacks into current stable
  T81 surfaces,
- or replacing the current runtime-first adoption wedge with an OS-first story.

## 12. Acceptance Signal

This RFC is successful if it helps contributors do all of the following:

- describe what DAIOS means in T81 without overstating the present,
- place new work in the right architectural layer,
- reject off-path expansion earlier,
- and preserve the current practical priority order:
  - RFC-00D1,
  - newcomer clarity,
  - deterministic runtime stability,
  - bounded-family boringness.

## 13. Implementation Posture

The intended posture after this RFC is added is:

- architecture ahead,
- implementation behind proof.

That means the target architecture may be discussed broadly, but coding should
continue to prefer the smallest finishable increment in a real current lane.
