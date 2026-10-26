# Trust boundary and adversarial conditions

Status: research threat model. None of the proposed attacks below has been
executed in this investigation. Existing source/report evidence is classified
in [RESEARCH_QUESTION.md](RESEARCH_QUESTION.md).

## Boundary and authority

The model, its response, retrieved text and rationale are untrusted. They may
attempt prompt injection, invent evidence, ask for tools or impersonate an
administrator. The trusted caller separately supplies intended operation,
immutable pins and a bounded grant. The model receives no bearer secret, writable
policy, authority-store handle or effect-sink reference. Only the verifier/sink
owner can admit the one defined effect. Unknown fields and invalid outputs fail
closed; a model-written PERMIT string is not an authorization credential.

The trusted computing base for the fixture consists of:

- the caller's intent/grant binding and approved model/provider configuration;
- bounded parser/schema interpretation, digest implementation, verifier,
  guard definitions and independent oracle used to assess the experiment;
- the preloaded observation registry and authority/replay/budget state;
- the single owner of the in-memory sink and its final admission transition;
- host language runtime, compiler/interpreter, memory protection and the OS/hardware
  supporting those components, plus the channel identifying any model worker.

The model is excluded from the **effect-safety** TCB. A genuinely enforced
untrusted local worker also needs existing process isolation: placing malicious
native inference code in the verifier process would put it inside the TCB.
A Python/C++ interface or effect annotation alone is not a process boundary.
The first experiment replays inert bytes and therefore does not test isolation
of arbitrary native code. Hosted models add provider/channel trust for statements
about execution, even while local effect authorization remains external.

There is no unrestricted system control: no shell execution, file deletion,
network tool dispatch, arbitrary plugin loading or model-controlled recursion.
The only candidate effect is an append to an in-memory fixture ledger. Later
provider experiments would need explicit admission of that different boundary;
no such execution is part of this packet.

## Enforceable guarantees and limits

A monitor can prevent a mediated disallowed effect; it cannot make an untrusted
model honest or guarantee a useful answer. Input/output shape can be checked
independently, and evidence refs can be resolved against an independent registry.
Factual correctness is limited to the registry's declared predicate and trust.
The [reference-monitor result](https://ecommons.cornell.edu/entities/publication/73f44364-f45e-40b5-b581-301a42619b0d)
and [restricted-agent design patterns](https://arxiv.org/html/2506.08837v1)
provide precedents, not proof that this proposed implementation is safe.

| Condition | Required independent check | Residual boundary |
| --- | --- | --- |
| Prompt injection in rationale or source material | Rationale is never evaluated as policy or code; only bounded typed fields participate in guards | Injection can still cause bad advice, denial or deferral; no semantic helpfulness guarantee |
| Malformed/ambiguous output | Reject extra fields, duplicate keys, invalid encoding/numbers, trailing data, excessive size/depth | Parser defects can undermine every subsequent guard |
| Conflicting instructions / forged authorization | Caller grant and current policy determine authority; response cannot change either | A compromised caller can authorize harmful but permitted actions |
| Unauthorized tool request | No tool field or model-held effect capability; any extra request is invalid | Any bypassing tool path outside the sole sink breaks mediation |
| Replay / stale policy | Bind request/model/output/policy refs and check live freshness at final admission | Reinitializing or rolling back the owner erases this in-session protection |
| Model substitution | Host independently pins provider/local artifact; grant binds model ref; reject stale echoes | A hosted provider can conceal substitution; matching echoed IDs are not model attestation |
| Evidence laundering or contradiction | Validate ref/source/artifact/predicate/epoch; retain support/refutation pair; conflict denies | Trusted source can be wrong; hashes do not prove its observations |
| Exhaustion / cancellation | Bound bytes, parsing, number of guards/refs and logical budget; capture timeout/cancel as terminal events | Logical units do not prove CPU/GPU time or financial bounds at an opaque provider |
| Revocation race | Final policy check and sink admission are serialized with authority updates | A check followed by an independently mutable external effect is a TOCTOU gap |
| False receipt / interrupted sink | Distinguish decision from committed row; receipt names verifier/owner and observations | An unsigned local record is not independently authenticated; no crash durability or physical-effect proof |

Confused-deputy and token-audience mistakes are relevant established failure
modes, not problems solved by asking a model to follow policy.
[MCP's security guidance](https://modelcontextprotocol.io/docs/2025-11-25/tutorials/security/security_best_practices)
illustrates those protocol boundaries; this experiment does not implement MCP or
OAuth. No model response may delegate or forward authority it does not possess.

## Adversary and non-goals

The adversary controls all response bytes, can supply contradictory references,
replay old messages, ask for larger budgets, reorder messages and race authority
changes in a finite event schedule. It can provide an unavailable or lying hosted
model. It does not control the fixture oracle, preloaded truth, final sink owner
or host memory in the primary threat model. Host compromise, malicious kernel,
side channels and storage rollback are explicit extensions, not secretly solved.

Specific failure counterexamples that the proposal must expose:

1. Validate model A, then swap the callable to B before invocation: a digest-only
   descriptor check is insufficient unless it binds the actual invoked provider.
2. Check a policy, then revoke it before a separately owned sink action: a valid
   proposal cannot repair the missing atomic admission boundary.
3. Supply a correctly hashed but false test report: identity verification cannot
   replace independently observed test results.
4. Collapse both missing and contradictory evidence into a retryable DEFER:
   safety may still block effects, but retry/liveness and audit semantics are lost.
5. Restore a previous replay store: uniqueness in one continuous owner session
   does not establish rollback-resistant or distributed exactly-once execution.

Existing CanonFS implementations have differing capability/hook behavior and
legacy no-capability-file behavior; the proposed mandatory grant must be tested
explicitly rather than assumed from storing an object there. Likewise existing
witness code has a signing-helper/test-secret TCB; a receipt checked with a shared
secret cannot prove an independent observer or correctness of LLM reasoning.
These are source-level boundaries, not new exploitation results.
