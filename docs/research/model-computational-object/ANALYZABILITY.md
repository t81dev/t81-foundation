# Independent reconstruction requirement

A conforming future implementation must let an external auditor reconstruct
what the system could authorize without trusting the model's rationale or the
producer's `decision=P` assertion. This is an acceptance obligation independent
of whether A and B agree. Ghidra integration is possible future analysis work,
not a dependency, implemented component or acceptance shortcut.

## Minimum audit packet

Publish the frozen corpus revision/manifest, raw request and response bytes,
exact schemas/wire profile, initial authoritative context and its provenance,
all model/provider/contract pins, evidence bytes plus required source/predicate/
epoch bindings, the event order, and independently observed final owner state.
Every dynamic record must contain:

- case/fixture ref and sequence number, event and stopping stage;
- before/after policy, replay, remaining-unit, row and owner-sequence state;
- raw request/output refs and approved versus observed model/provider bindings;
- the observed evidence pair and validity result for each required ref;
- decision and ordered diagnostic codes **with subjects**, including deferred
  reasons and typed model-failure cause;
- broker-probe count separately from protected dispatches, committed rows,
  receipt ref (or explicit absence), and any sink fault;
- verifier build/configuration identity and which inputs the observer actually
  saw, leaving unobserved provider internals explicitly unknown.

`corpus/cases.jsonl` and `event-permutations.jsonl` contain expected records,
not independent observed execution journals. A future implementation must emit
its own observed records in addition to these expectations. An unsigned journal
and a valid hash establish internal consistency, not truthful observation or
who produced it. Bind trusted context/authority versions to a trusted observer;
for adversarial hosts a separately protected witness/anchor would be required.
That stronger threat model is not claimed by this single-owner specification.

## Reconstruction algorithm for an external auditor

1. Verify file and per-fixture hashes; decode the exact bytes, checking the typed
   hash domains. Verify how the auditor obtained the trusted manifest/context.
2. Independently apply the wire profile and structural schemas. Do not normalize
   a message before hashing it, accept duplicate-key last-wins behavior, or use
   machine-float underflow to change mathematical integer validity.
3. Reconstruct bindings from caller intent and independently observed provider,
   not from echo fields or the rationale. Enumerate every required observation;
   distinguish missing entry, valid 00, valid 01, valid 11 and invalid record.
4. Use ORACLE_SPEC.md's direct conjunction and diagnostic classification, not a
   model summary or candidate combiner. Check both permission and reason fidelity.
5. Validate the entire event order, rejecting invalid plans without processing
   a prefix. Replay reachable records against the declared owner transition:
   re-evaluate live facts at each A/B; check each before/after delta, receipt and
   final row/sequence state. Reconcile the state with an independent observation
   of the owned sink, not just the implementation's claimed counter.
6. Mark any missing input/event/authority provenance as unverified. Do not infer
   that a row proves real-world test success, a hosted model revision or physical
   side effect. Record divergences and the earliest failed obligation.

Worked expectations: F10-C02 initially has P with an active grant. R changes
no state; V changes policy_state to revoked; A must then report D with
POLICY_REVOKED and leave replay fresh, remaining units 2, rows/sequence 0. The
old P does not justify a receipt. In F09-C02, all response shapes and echoes
can be valid while observed B differs from approved A: structural validity
cannot repair ResponseBinding/D/BINDING. Neither example depends on whether the
rationale explains, denies or apologizes for the violation.

## Analyzability falsifiers and future binary inspection

Fail the auditability claim if required context is hidden, a rejection cannot be
attributed to a typed check/subject, a receipt cannot be bound to an observed
commit, or the only explanation is model-generated prose. A/B agreement cannot
compensate for an unreconstructible authority decision.

If a compiled implementation is later built, an external binary auditor could
use ordinary disassembly, call-graph and data-flow tools to inspect the final
policy/model/replay checks and sole effect boundary. Ghidra may help inspect
those conventional binaries; no processor module, ISA change or Ghidra-specific
runtime is needed or authorized here. Symbols alone do not prove enforcement;
reconstruction still needs the actual configuration, observed transitions and
trusted inputs. No missing property was found that ordinary typed services,
reference monitors and suitably protected observers cannot express; deployment
trust, opaque-provider observability and host rollback remain limitations, not
reasons to manufacture a new OS primitive.
